// ==========================================
// AskalPlayerBalance - Sistema de Balance do Player
// Gerenciamento de moedas virtuais via JSON
// ==========================================

// Estrutura JSON do player
class AskalPlayerData
{
	string FirstLogin;
	ref map<string, int> Balance; // Ex: {"Askal_Coin": 1234567}
	int CreditCard;
	int PremiumStatusExpireInHours;
	ref map<string, int> Permissions;
	
	void AskalPlayerData()
	{
		FirstLogin = "";
		Balance = new map<string, int>;
		CreditCard = 0;
		PremiumStatusExpireInHours = 0;
		Permissions = new map<string, int>;
	}
}

// Gerenciador de balance
class AskalPlayerBalance
{
	// ITER-1: Per-player locks, reservations, cache, and outbox for async persistence
	private static ref map<string, bool> s_PlayerLocks = new map<string, bool>();
	private static ref map<string, ref map<string, int>> s_Reservations = new map<string, ref map<string, int>>();
	private static ref map<string, ref AskalPlayerData> s_Cache = new map<string, ref AskalPlayerData>();
	private static ref map<string, float> s_CacheTimestamps = new map<string, float>();
	private static ref map<string, ref AskalPlayerData> s_OutboxQueue = new map<string, ref AskalPlayerData>>();
	private static const float CACHE_TTL = 30.0;
	private static const float LOCK_TIMEOUT_MS = 500.0; // Max 500ms wait
	private static const float OUTBOX_FLUSH_INTERVAL = 5.0; // Flush every 5 seconds
	private static float s_LastOutboxFlush = 0.0;

	private static bool s_Initialized = false;
	private static bool s_MarketConfigLoaded = false;
	private static ref AskalMarketConfig s_MarketConfig;
	
	// Player config validation state (must be validated before any transaction)
	private static ref map<string, bool> s_PlayerConfigValid = new map<string, bool>();

	// ITER-1: Simple lock mechanism (non-blocking with timeout simulation)
	private static bool TryLockPlayer(string steamId)
	{
		if (!s_PlayerLocks.Contains(steamId))
		{
			s_PlayerLocks.Set(steamId, true);
			return true;
		}

		if (s_PlayerLocks.Get(steamId))
			return false; // Already locked

		s_PlayerLocks.Set(steamId, true);
		return true;
	}

	private static void UnlockPlayer(string steamId)
	{
		s_PlayerLocks.Set(steamId, false);
	}

	// ITER-1: Get cached data or load from disk (guarantees file creation if missing)
	private static AskalPlayerData GetCachedOrLoad(string steamId)
	{
		if (!steamId || steamId == "")
		{
			Print("[AskalBalance] ❌ GetCachedOrLoad: steamId vazio!");
			return NULL;
		}
		
		// ALWAYS ensure file exists first - don't trust cache
		EnsurePlayerFileExists(steamId);
		
		// Declare filePath once at function scope
		string filePath = GetPlayerFilePath(steamId);
		
		float now = GetGame().GetTime();
		if (s_Cache.Contains(steamId))
		{
			float cacheTime = s_CacheTimestamps.Get(steamId);
			if (now - cacheTime < CACHE_TTL)
			{
				// Verify file still exists before returning cached data
				if (FileExist(filePath))
				{
					return s_Cache.Get(steamId);
				}
				else
				{
					Print("[AskalBalance] ⚠️ GetCachedOrLoad: Cache existe mas arquivo não, recarregando...");
					s_Cache.Remove(steamId);
					s_CacheTimestamps.Remove(steamId);
				}
			}
			else
			{
				s_Cache.Remove(steamId);
				s_CacheTimestamps.Remove(steamId);
			}
		}

		// LoadPlayerData will create file if it doesn't exist
		Print("[AskalBalance] 🔍 GetCachedOrLoad: Carregando dados do player: " + steamId);
		AskalPlayerData data = LoadPlayerData(steamId);
		if (data)
		{
			// Verify file exists before caching
			if (FileExist(filePath))
			{
				s_Cache.Set(steamId, data);
				s_CacheTimestamps.Set(steamId, now);
				Print("[AskalBalance] ✅ GetCachedOrLoad: Dados carregados e cache atualizado para: " + steamId);
			}
			else
			{
				Print("[AskalBalance] ⚠️ GetCachedOrLoad: Dados carregados mas arquivo não existe, recriando...");
				EnsurePlayerFileExists(steamId);
				filePath = GetPlayerFilePath(steamId); // Update path
				data = LoadPlayerData(steamId);
				if (data && FileExist(filePath))
				{
					s_Cache.Set(steamId, data);
					s_CacheTimestamps.Set(steamId, now);
				}
			}
		}
		else
		{
			// If LoadPlayerData failed, try to create default data
			Print("[AskalBalance] ⚠️ GetCachedOrLoad: LoadPlayerData retornou NULL, tentando criar dados padrão para: " + steamId);
			EnsurePlayerFileExists(steamId);
			filePath = GetPlayerFilePath(steamId); // Update path
			data = LoadPlayerData(steamId);
			if (data)
			{
				if (FileExist(filePath))
				{
					s_Cache.Set(steamId, data);
					s_CacheTimestamps.Set(steamId, now);
					Print("[AskalBalance] ✅ GetCachedOrLoad: Dados padrão criados e salvos para: " + steamId);
				}
			}
		}
		return data;
	}

	static void Init()
	{
		if (s_Initialized)
			return;

		string baseDir = "$profile:Askal";
		if (!FileExist(baseDir))
		{
			MakeDirectory(baseDir);
			Print("[AskalBalance] 📁 Pasta criada: " + baseDir);
		}

		string dbDir = baseDir + "/Database";
		if (!FileExist(dbDir))
		{
			MakeDirectory(dbDir);
			Print("[AskalBalance] 📁 Pasta criada: " + dbDir);
		}

		string playersDir = dbDir + "/Players";
		if (!FileExist(playersDir))
		{
			MakeDirectory(playersDir);
			Print("[AskalBalance] 📁 Pasta criada: " + playersDir);
		}

		// ITER-1: Initialize outbox flush timer
		s_LastOutboxFlush = GetGame().GetTime();

		Print("[AskalBalance] Sistema de balance inicializado (persistência em tempo real)");
		Print("[AskalBalance] 📂 Diretório de players: " + playersDir);
		s_Initialized = true;
	}

	static void EnsureMarketConfigLoaded()
	{
		if (s_MarketConfigLoaded)
			return;

		s_MarketConfig = AskalMarketConfig.GetInstance();
		if (!s_MarketConfig)
		{
			Print("[AskalBalance] ❌ EnsureMarketConfigLoaded: Falha ao carregar MarketConfig (GetInstance retornou NULL)");
		}
		else if (!s_MarketConfig.Currencies)
		{
			Print("[AskalBalance] ⚠️ EnsureMarketConfigLoaded: MarketConfig sem moedas configuradas");
		}

		s_MarketConfigLoaded = true;
	}
	
	// Obter caminho do arquivo JSON do player
	static string GetPlayerFilePath(string steamId)
	{
		if (!steamId || steamId == "")
			return "";
		
		return "$profile:Askal/Database/Players/" + steamId + ".json";
	}
	
	// Migrate player data to ensure all currencies are present
	private static bool MigratePlayerData(string steamId, string filePath, AskalPlayerData playerData, string originalContent)
	{
		if (!playerData || !filePath || filePath == "")
			return false;
		
		EnsureMarketConfigLoaded();
		if (!s_MarketConfig || !s_MarketConfig.Currencies)
		{
			Print("[AskalJsonLoader] ERROR: MarketConfig not available, skipping player currency migration for: " + steamId);
			return false;
		}
		
		bool needsMigration = false;
		
		// Ensure Balance exists and is an object
		if (!playerData.Balance)
		{
			playerData.Balance = new map<string, int>;
			needsMigration = true;
			Print("[AskalJsonLoader] Balance missing => creating object for player " + steamId);
		}
		
		// First, migrate old balance format (item class names) to new format (currency IDs)
		// This might add/modify entries, so check if it made changes
		int balanceCountBefore = playerData.Balance.Count();
		MigrateBalanceFormat(playerData);
		if (playerData.Balance.Count() != balanceCountBefore)
		{
			needsMigration = true;
		}
		
		// Migrate: ensure all currencies are present
		for (int currencyIdx = 0; currencyIdx < s_MarketConfig.Currencies.Count(); currencyIdx++)
		{
			string currencyId = s_MarketConfig.Currencies.GetKey(currencyIdx);
			AskalCurrencyConfig currencyConfig = s_MarketConfig.Currencies.GetElement(currencyIdx);
			if (!currencyConfig)
				continue;
			
			if (!playerData.Balance.Contains(currencyId))
			{
				needsMigration = true;
				int seedValue = 0;
				
				if (currencyConfig.Mode == AskalMarketConstants.CURRENCY_MODE_DISABLED)
				{
					seedValue = 0;
					Print("[AskalJsonLoader] MIGRATE: currency " + currencyId + " is disabled (Mode 0); seeding 0");
				}
				else
				{
					seedValue = currencyConfig.StartCurrency;
					if (seedValue < 0)
						seedValue = 0;
				}
				
				playerData.Balance.Set(currencyId, seedValue);
				Print("[AskalJsonLoader] MIGRATE: Added missing currency \"" + currencyId + "\" = " + seedValue + " to player " + steamId);
			}
		}
		
		// If migration was needed, save with backup
		if (needsMigration)
		{
			Print("[AskalJsonLoader] Player file saved (migrated): " + filePath);
			bool saveSuccess = AskalJsonLoader<AskalPlayerData>.SaveToFileWithBackup(filePath, playerData, originalContent);
			
			// Update in-memory cache after successful save
			if (saveSuccess)
			{
				s_Cache.Set(steamId, playerData);
				s_CacheTimestamps.Set(steamId, GetGame().GetTime());
			}
			
			return saveSuccess;
		}
		
		return true;
	}
	
	// Validate and load player config using synchronous loader (MUST be called before any transaction)
	// Returns true if config is valid and loaded, false otherwise
	static bool ValidateAndLoadPlayerConfig(string steamId)
	{
		// Use the new synchronous loader
		AskalPlayerData playerData;
		bool success;
		string reason;
		
		AskalPlayerConfigLoader.LoadOrCreatePlayerConfigSync(steamId, playerData, success, reason);
		
		if (success && playerData)
		{
			// Initialize in-memory balances
			s_Cache.Set(steamId, playerData);
			s_CacheTimestamps.Set(steamId, GetGame().GetTime());
			
			// Update internal validation flag
			// Note: PlayerMeta is set from 4_World context (AskalPurchaseModule)
			s_PlayerConfigValid.Set(steamId, true);
			
			return true;
		}
		else
		{
			// Mark as invalid
			// Note: PlayerMeta is set from 4_World context (AskalPurchaseModule)
			s_PlayerConfigValid.Set(steamId, false);
			
			// Clear in-memory balances
			if (s_Cache.Contains(steamId))
				s_Cache.Remove(steamId);
			if (s_CacheTimestamps.Contains(steamId))
				s_CacheTimestamps.Remove(steamId);
			
			return false;
		}
	}
	
	// Legacy ValidateAndLoadPlayerConfig - kept for compatibility but now uses new loader
	static bool ValidateAndLoadPlayerConfigLegacy(string steamId)
	{
		if (!steamId || steamId == "")
		{
			Print("[AskalJsonLoader] ERROR: ValidateAndLoadPlayerConfig called with empty steamId");
			s_PlayerConfigValid.Set(steamId, false);
			return false;
		}
		
		// Check if already validated and cached
		if (s_PlayerConfigValid.Contains(steamId) && s_PlayerConfigValid.Get(steamId))
		{
			// Verify file still exists
			string cachedFilePath = GetPlayerFilePath(steamId);
			if (FileExist(cachedFilePath))
				return true;
			// File was deleted, need to revalidate
			s_PlayerConfigValid.Set(steamId, false);
		}
		
		Init();
		EnsureMarketConfigLoaded();
		
		// Check if MarketConfig is available
		if (!s_MarketConfig || !s_MarketConfig.Currencies)
		{
			Print("[AskalJsonLoader] ERROR: MarketConfig not available — cannot migrate/seed balances");
			s_PlayerConfigValid.Set(steamId, false);
			return false;
		}
		
		string configFilePath = GetPlayerFilePath(steamId);
		if (!configFilePath || configFilePath == "")
		{
			Print("[AskalJsonLoader] ERROR: Invalid file path for player: " + steamId);
			s_PlayerConfigValid.Set(steamId, false);
			return false;
		}
		
		Print("[AskalJsonLoader] Loading player file: " + configFilePath);
		
		AskalPlayerData playerData = new AskalPlayerData();
		string originalContent = "";
		bool fileExists = FileExist(configFilePath);
		bool parseSuccess = false;
		bool needsMigrationFlag = false;
		
		if (fileExists)
		{
			// Try to load and parse file
			FileHandle fh = OpenFile(configFilePath, FileMode.READ);
			if (fh)
			{
				string rawContent = "";
				string line;
				while (FGets(fh, line) > 0)
				{
					rawContent = rawContent + "\n" + line;
				}
				CloseFile(fh);
				
				if (rawContent != "")
				{
					originalContent = rawContent;
					
					// Check if Balance is legacy numeric format (simple heuristic: look for "Balance":<number>)
					// This is a basic check - full JSON parsing would be needed for robust detection
					bool hasLegacyNumericBalance = false;
					int legacyBalanceValue = 0;
					
					// Try to detect legacy format: "Balance":<number> (not "Balance":{...})
					int balancePos = rawContent.IndexOf("\"Balance\"");
					if (balancePos >= 0)
					{
						// Find colon after "Balance"
						string afterBalance = rawContent.Substring(balancePos, rawContent.Length() - balancePos);
						int colonPos = afterBalance.IndexOf(":");
						if (colonPos >= 0)
						{
							string afterColon = afterBalance.Substring(colonPos + 1, afterBalance.Length() - colonPos - 1);
							afterColon.Trim();
							// Check if it starts with a digit (numeric) instead of "{" (object)
							if (afterColon.Length() > 0)
							{
								string firstChar = afterColon.Substring(0, 1);
								// Check if first character is a digit by comparing with "0" and "9"
								if (firstChar >= "0" && firstChar <= "9")
								{
									// Extract number (simple extraction)
									string numberStr = "";
									for (int numIdx = 0; numIdx < afterColon.Length(); numIdx++)
									{
										string charStr = afterColon.Substring(numIdx, 1);
										// Check if character is a digit
										if (charStr >= "0" && charStr <= "9")
											numberStr += charStr;
										else if (charStr == "," || charStr == "}" || charStr == "\n" || charStr == "\r") // comma, }, newline
											break;
									}
									if (numberStr != "")
									{
										legacyBalanceValue = numberStr.ToInt();
										hasLegacyNumericBalance = true;
										Print("[AskalJsonLoader] Detected legacy numeric Balance: " + legacyBalanceValue);
									}
								}
							}
						}
					}
					
					// Try to parse JSON
					string error;
					JsonSerializer serializer = new JsonSerializer();
					parseSuccess = serializer.ReadFromString(playerData, rawContent, error);
					
					if (!parseSuccess || error != string.Empty)
					{
						Print("[AskalJsonLoader] ERROR: Failed to parse player JSON: " + error);
						Print("[AskalJsonLoader] Creating corrupt backup and replacing with default file");
						
						// Create corrupt backup
						if (!AskalJsonLoader<AskalPlayerData>.CreateCorruptBackup(configFilePath, rawContent))
						{
							Print("[AskalJsonLoader] ERROR: Failed to create corrupt backup, aborting");
							s_PlayerConfigValid.Set(steamId, false);
							return false;
						}
						
						// Reset playerData for default creation
						playerData = new AskalPlayerData();
						originalContent = "";
					}
					else if (hasLegacyNumericBalance)
					{
						// Balance was numeric, convert to object format
						Print("[AskalJsonLoader] Converting legacy numeric Balance to object format");
						playerData.Balance = new map<string, int>();
						
						// Get default currency ID
						string defaultCurrencyId = s_MarketConfig.GetDefaultCurrencyId();
						if (defaultCurrencyId == "")
							defaultCurrencyId = AskalMarketConstants.DEFAULT_CURRENCY_ID;
						
						playerData.Balance.Set(defaultCurrencyId, legacyBalanceValue);
						Print("[AskalJsonLoader] MIGRATE: Converted legacy Balance " + legacyBalanceValue + " to {" + defaultCurrencyId + ": " + legacyBalanceValue + "}");
						
						// Mark for migration save
						needsMigrationFlag = true;
					}
				}
			}
		}
		else
		{
			Print("[AskalJsonLoader] Player file not found: " + configFilePath + " — creating default");
		}
		
		// If file doesn't exist or parse failed, create default
		if (!fileExists || !parseSuccess)
		{
			playerData = CreateDefaultPlayerData();
			if (!playerData)
			{
				Print("[AskalJsonLoader] ERROR: Failed to create default player data");
				s_PlayerConfigValid.Set(steamId, false);
				return false;
			}
			
			// Set FirstLogin timestamp if creating new file
			if (!fileExists)
			{
				int year, month, day, hour, minute, second;
				GetYearMonthDay(year, month, day);
				GetHourMinuteSecond(hour, minute, second);
				
				string firstLogin = year.ToString() + "-";
				if (month < 10) firstLogin += "0";
				firstLogin += month.ToString() + "-";
				if (day < 10) firstLogin += "0";
				firstLogin += day.ToString() + "T";
				if (hour < 10) firstLogin += "0";
				firstLogin += hour.ToString() + ":";
				if (minute < 10) firstLogin += "0";
				firstLogin += minute.ToString() + ":";
				if (second < 10) firstLogin += "0";
				firstLogin += second.ToString() + "Z";
				playerData.FirstLogin = firstLogin;
			}
			
			// Log currencies being created
			string currencyList = "";
			if (s_MarketConfig && s_MarketConfig.Currencies)
			{
				for (int listIdx = 0; listIdx < s_MarketConfig.Currencies.Count(); listIdx++)
				{
					string listCurrencyId = s_MarketConfig.Currencies.GetKey(listIdx);
					if (currencyList != "")
						currencyList += ", ";
					currencyList += listCurrencyId;
				}
			}
			Print("[AskalJsonLoader] Created new player JSON for " + steamId + " with currencies: " + currencyList);
			
			// Save new file atomically
			if (!AskalJsonLoader<AskalPlayerData>.SaveToFile(configFilePath, playerData))
			{
				Print("[AskalJsonLoader] ERROR: Failed to save new player file: " + configFilePath);
				s_PlayerConfigValid.Set(steamId, false);
				return false;
			}
			
			// Verify file was created
			if (!FileExist(configFilePath))
			{
				Print("[AskalJsonLoader] ERROR: File was not created after save: " + configFilePath);
				s_PlayerConfigValid.Set(steamId, false);
				return false;
			}
		}
		else
		{
			// File exists and parsed successfully, migrate if needed
			// Ensure Balance is an object (not legacy numeric)
			if (!playerData.Balance)
			{
				playerData.Balance = new map<string, int>();
			}
			
			// If legacy conversion was detected, needsMigrationFlag is already set
			// Otherwise, check for missing currencies
			if (!needsMigrationFlag)
			{
				needsMigrationFlag = false; // Will be set to true if currencies are missing
			}
			
			// Migrate: ensure all currencies exist
			for (int migrateIdx = 0; migrateIdx < s_MarketConfig.Currencies.Count(); migrateIdx++)
			{
				string migrateCurrencyId = s_MarketConfig.Currencies.GetKey(migrateIdx);
				AskalCurrencyConfig migrateCurrencyConfig = s_MarketConfig.Currencies.GetElement(migrateIdx);
				if (!migrateCurrencyConfig)
					continue;
				
				if (!playerData.Balance.Contains(migrateCurrencyId))
				{
					needsMigrationFlag = true;
					int seedValue = 0;
					
					if (migrateCurrencyConfig.Mode == AskalMarketConstants.CURRENCY_MODE_DISABLED)
					{
						seedValue = 0;
					}
					else
					{
						seedValue = migrateCurrencyConfig.StartCurrency;
						if (seedValue < 0)
							seedValue = 0;
					}
					
					playerData.Balance.Set(migrateCurrencyId, seedValue);
					Print("[AskalJsonLoader] MIGRATE: Added missing currency \"" + migrateCurrencyId + "\"=" + seedValue + " to player " + steamId);
				}
			}
			
			// If migration was needed, save with backup
			if (needsMigrationFlag)
			{
				// Save migrated data atomically with backup
				if (!AskalJsonLoader<AskalPlayerData>.SaveToFileWithBackup(configFilePath, playerData, originalContent))
				{
					Print("[AskalJsonLoader] ERROR: Failed to save migrated player file");
					s_PlayerConfigValid.Set(steamId, false);
					return false;
				}
				
				Print("[AskalJsonLoader] Player file saved (migrated): " + configFilePath);
			}
		}
		
		// Populate in-memory balance structures
		s_Cache.Set(steamId, playerData);
		s_CacheTimestamps.Set(steamId, GetGame().GetTime());
		
		// Mark as valid
		s_PlayerConfigValid.Set(steamId, true);
		Print("[AskalJsonLoader] Player config validated and loaded for: " + steamId);
		
		return true;
	}
	
	// Check if player config is valid (must be called before transactions)
	// Public method for external validation checks
	// Note: This is called from 3_Game context, PlayerMeta check is done in 4_World
	static bool IsPlayerConfigValid(string steamId)
	{
		if (!steamId || steamId == "")
			return false;
		
		// Check internal validation flag
		// Note: PlayerMeta is checked in 4_World context (AskalPurchaseModule)
		if (!s_PlayerConfigValid.Contains(steamId))
			return false;
		
		return s_PlayerConfigValid.Get(steamId);
	}
	
	// Initialize in-memory balances from loaded player config
	static void InitializePlayerBalances(string steamId, AskalPlayerData playerData)
	{
		if (!steamId || steamId == "" || !playerData)
			return;
		
		s_Cache.Set(steamId, playerData);
		s_CacheTimestamps.Set(steamId, GetGame().GetTime());
		s_PlayerConfigValid.Set(steamId, true);
	}
	
	// Set internal validation flag (called from 4_World context)
	static void SetPlayerConfigValidInternal(string steamId, bool valid)
	{
		if (!steamId || steamId == "")
			return;
		
		s_PlayerConfigValid.Set(steamId, valid);
	}
	
	// Carregar dados do player (lê sempre do disco)
	static AskalPlayerData LoadPlayerData(string steamId)
	{
		if (!steamId || steamId == "")
		{
			Print("[AskalBalance] ❌ LoadPlayerData: steamId vazio!");
			return NULL;
		}

		Init();

		string filePath = GetPlayerFilePath(steamId);
		if (!filePath || filePath == "")
		{
			Print("[AskalBalance] ❌ LoadPlayerData: Caminho do arquivo inválido para: " + steamId);
			return NULL;
		}
		
		Print("[AskalBalance] 🔍 LoadPlayerData: Carregando dados do player: " + steamId + " -> " + filePath);

		AskalPlayerData playerData = new AskalPlayerData();
		string originalContent = "";

		if (FileExist(filePath))
		{
			Print("[AskalBalance] 🔍 LoadPlayerData: Arquivo existe, tentando carregar...");
			if (AskalJsonLoader<AskalPlayerData>.LoadFromFileWithOriginal(filePath, playerData, originalContent, false))
			{
				// Garante estruturas válidas mesmo se JSON foi editado manualmente
				if (!playerData.Balance)
					playerData.Balance = new map<string, int>;
				if (!playerData.Permissions)
					playerData.Permissions = new map<string, int>;
				
				Print("[AskalBalance] ✅ LoadPlayerData: Arquivo carregado com sucesso, migrando...");
				// Migrate player data (ensure all currencies present, create backup if needed)
				MigratePlayerData(steamId, filePath, playerData, originalContent);
				
				return playerData;
			}
			Print("[AskalBalance] ⚠️ LoadPlayerData: Erro ao carregar dados do player, recriando: " + steamId);
		}
		else
		{
			Print("[AskalBalance] ⚠️ LoadPlayerData: Arquivo do player NÃO encontrado, criando novo: " + steamId);
		}

		Print("[AskalBalance] 🔧 LoadPlayerData: Criando dados padrão...");
		playerData = CreateDefaultPlayerData();
		if (!playerData)
		{
			Print("[AskalBalance] ❌ LoadPlayerData: CreateDefaultPlayerData retornou NULL!");
			return NULL;
		}
		
		// Log currencies being created
		string currencyList = "";
		if (s_MarketConfig && s_MarketConfig.Currencies)
		{
			for (int i = 0; i < s_MarketConfig.Currencies.Count(); i++)
			{
				string currencyId = s_MarketConfig.Currencies.GetKey(i);
				if (currencyList != "")
					currencyList += ", ";
				currencyList += currencyId;
			}
		}
		Print("[AskalBalance] 💰 LoadPlayerData: Criando JSON para " + steamId + " com moedas: " + currencyList);
		
		// Ensure directories exist before saving
		Init();
		
		// Verify directory exists
		string playersDir = "$profile:Askal/Database/Players";
		if (!FileExist(playersDir))
		{
			Print("[AskalBalance] ⚠️ LoadPlayerData: Diretório não existe, criando: " + playersDir);
			MakeDirectory(playersDir);
		}
		
		// Save the new player data
		Print("[AskalBalance] 💾 LoadPlayerData: Salvando novo arquivo do player...");
		if (SavePlayerData(steamId, playerData))
		{
			// Verify file was created
			if (FileExist(filePath))
			{
				Print("[AskalBalance] ✅✅✅ LoadPlayerData: Arquivo JSON do player criado e verificado: " + filePath);
			}
			else
			{
				Print("[AskalBalance] ❌❌❌ LoadPlayerData: ERRO - Arquivo NÃO foi criado: " + filePath);
			}
		}
		else
		{
			Print("[AskalBalance] ❌❌❌ LoadPlayerData: ERRO CRÍTICO - Falha ao salvar arquivo JSON do player: " + filePath);
		}
		
		return playerData;
	}

	static AskalPlayerData CreateDefaultPlayerData()
	{
		Print("[AskalBalance] 🔧 CreateDefaultPlayerData: Iniciando criação de dados padrão");
		AskalPlayerData playerData = new AskalPlayerData();
		playerData.FirstLogin = "0";
		EnsureMarketConfigLoaded();

		if (s_MarketConfig && s_MarketConfig.Currencies)
		{
			Print("[AskalBalance] 🔧 CreateDefaultPlayerData: MarketConfig encontrado, " + s_MarketConfig.Currencies.Count() + " moedas configuradas");
			for (int currencyIdx = 0; currencyIdx < s_MarketConfig.Currencies.Count(); currencyIdx++)
			{
				string currencyId = s_MarketConfig.Currencies.GetKey(currencyIdx);
				AskalCurrencyConfig currencyConfig = s_MarketConfig.Currencies.GetElement(currencyIdx);
				if (!currencyConfig)
				{
					Print("[AskalBalance] ⚠️ CreateDefaultPlayerData: currencyConfig NULL para " + currencyId);
					continue;
				}

				// Seed all currencies (including disabled ones with 0)
				int seedValue = 0;
				if (currencyConfig.Mode == AskalMarketConstants.CURRENCY_MODE_DISABLED)
				{
					seedValue = 0;
					Print("[AskalBalance] 🔧 CreateDefaultPlayerData: " + currencyId + " está desabilitada (Mode 0), seed = 0");
				}
				else
				{
					seedValue = currencyConfig.StartCurrency;
					if (seedValue < 0)
						seedValue = 0;
					Print("[AskalBalance] 🔧 CreateDefaultPlayerData: " + currencyId + " (Mode " + currencyConfig.Mode + "), StartCurrency = " + currencyConfig.StartCurrency + ", seed = " + seedValue);
				}

				// Use currency ID as key (not item class name)
				playerData.Balance.Set(currencyId, seedValue);
				if (seedValue > 0)
					Print("[AskalBalance] 💰 StartCurrency aplicado: " + currencyId + " = " + seedValue);
			}
			Print("[AskalBalance] 🔧 CreateDefaultPlayerData: Total de moedas no balance: " + playerData.Balance.Count());
		}
		else
		{
			// Fallback: ensure at least one balance entry if config not available
			Print("[AskalBalance] ⚠️ CreateDefaultPlayerData: MarketConfig não encontrado ou sem moedas, usando fallback 'Askal_Money'");
			playerData.Balance.Set("Askal_Money", 0);
		}
		
		Print("[AskalBalance] ✅ CreateDefaultPlayerData: Dados padrão criados com sucesso");
		return playerData;
	}
	
	// Salvar dados do player
	static bool SavePlayerData(string steamId, AskalPlayerData playerData)
	{
		if (!steamId || steamId == "" || !playerData)
		{
			string playerDataStatus = "NULL";
			if (playerData)
				playerDataStatus = "OK";
			Print("[AskalBalance] ❌ SavePlayerData: Parâmetros inválidos - steamId=" + steamId + " playerData=" + playerDataStatus);
			return false;
		}
		
		// Ensure directories exist FIRST
		Init();
		
		string filePath = GetPlayerFilePath(steamId);
		if (!filePath || filePath == "")
		{
			Print("[AskalBalance] ❌ SavePlayerData: Caminho do arquivo inválido para: " + steamId);
			return false;
		}
		
		Print("[AskalBalance] 💾 SavePlayerData: Salvando dados do player: " + steamId);
		Print("[AskalBalance] 💾 SavePlayerData: Caminho completo: " + filePath);
		
		// Verify directory exists before saving - CREATE IT IF NEEDED
		string playersDir = "$profile:Askal/Database/Players";
		if (!FileExist(playersDir))
		{
			Print("[AskalBalance] ⚠️ SavePlayerData: Diretório não existe, criando: " + playersDir);
			MakeDirectory(playersDir);
			// Verify it was created
			if (!FileExist(playersDir))
			{
				Print("[AskalBalance] ❌❌❌ SavePlayerData: ERRO CRÍTICO - Não foi possível criar diretório: " + playersDir);
				return false;
			}
			Print("[AskalBalance] ✅ SavePlayerData: Diretório criado com sucesso: " + playersDir);
		}
		
		Print("[AskalBalance] 💾 SavePlayerData: Chamando AskalJsonLoader.SaveToFile...");
		bool success = AskalJsonLoader<AskalPlayerData>.SaveToFile(filePath, playerData);
		
		// CRITICAL: Verify file was actually created
		if (success)
		{
			// Wait a tiny bit and verify
			if (FileExist(filePath))
			{
				Print("[AskalBalance] ✅✅✅ SavePlayerData: Arquivo criado e verificado com sucesso: " + filePath);
				// Update cache
				s_Cache.Set(steamId, playerData);
				s_CacheTimestamps.Set(steamId, GetGame().GetTime());
			}
			else
			{
				Print("[AskalBalance] ❌❌❌ SavePlayerData: ERRO CRÍTICO - SaveToFile retornou TRUE mas arquivo NÃO existe!");
				Print("[AskalBalance] ❌ Caminho esperado: " + filePath);
				Print("[AskalBalance] ❌ Verifique permissões do servidor e caminho!");
				success = false; // Mark as failed since file doesn't exist
			}
		}
		else
		{
			Print("[AskalBalance] ❌❌❌ SavePlayerData: ERRO - SaveToFile retornou FALSE!");
			Print("[AskalBalance] ❌ Caminho: " + filePath);
		}
		
		return success;
	}
	
	// ITER-1: Enqueue to outbox for async batch persistence
	private static void EnqueueToOutbox(string steamId, AskalPlayerData playerData)
	{
		s_OutboxQueue.Set(steamId, playerData);
		Print("[AskalBalance] ENQUEUE_PERSIST steamId=" + steamId);
		
		// Auto-flush if interval exceeded
		float now = GetGame().GetTime();
		if (now - s_LastOutboxFlush >= OUTBOX_FLUSH_INTERVAL)
		{
			FlushOutbox();
		}
	}
	
	// ITER-1: Flush outbox queue to disk
	static void FlushOutbox()
	{
		int count = 0;
		for (int i = 0; i < s_OutboxQueue.Count(); i++)
		{
			string steamId = s_OutboxQueue.GetKey(i);
			AskalPlayerData data = s_OutboxQueue.GetElement(i);
			if (data)
			{
				SavePlayerData(steamId, data);
				count++;
			}
		}
		s_OutboxQueue.Clear();
		s_LastOutboxFlush = GetGame().GetTime();
		Print("[AskalBalance] Flushed " + count + " pending saves to disk");
	}
	
	// Migrate old balance format (item class names) to new format (currency IDs)
	private static void MigrateBalanceFormat(AskalPlayerData playerData)
	{
		if (!playerData || !playerData.Balance)
			return;
		
		EnsureMarketConfigLoaded();
		if (!s_MarketConfig || !s_MarketConfig.Currencies)
			return;
		
		// Create a map of old item class names to currency IDs
		ref map<string, string> itemClassToCurrencyId = new map<string, string>();
		for (int i = 0; i < s_MarketConfig.Currencies.Count(); i++)
		{
			string currencyId = s_MarketConfig.Currencies.GetKey(i);
			AskalCurrencyConfig currencyConfig = s_MarketConfig.Currencies.GetElement(i);
			if (!currencyConfig || !currencyConfig.Values)
				continue;
			
			// Map each item class name to its currency ID
			for (int j = 0; j < currencyConfig.Values.Count(); j++)
			{
				AskalCurrencyValueConfig valueConfig = currencyConfig.Values.Get(j);
				if (valueConfig && valueConfig.Name && valueConfig.Name != "")
				{
					itemClassToCurrencyId.Set(valueConfig.Name, currencyId);
				}
			}
		}
		
		// Check if migration is needed (if any key is an item class name, not currency ID)
		bool needsMigration = false;
		ref map<string, int> oldBalances = new map<string, int>();
		
		for (int balanceIdx = 0; balanceIdx < playerData.Balance.Count(); balanceIdx++)
		{
			string key = playerData.Balance.GetKey(balanceIdx);
			int value = playerData.Balance.GetElement(balanceIdx);
			
			// If key is not a currency ID (not in Currencies map), it's likely an old item class name
			if (!s_MarketConfig.Currencies.Contains(key))
			{
				needsMigration = true;
				oldBalances.Set(key, value);
			}
		}
		
		if (!needsMigration)
			return; // Already in new format
		
		Print("[AskalBalance] 🔄 Migrando formato de balance (item class -> currency ID)");
		
		// Migrate: convert item class names to currency IDs
		for (int oldIdx = 0; oldIdx < oldBalances.Count(); oldIdx++)
		{
			string oldKey = oldBalances.GetKey(oldIdx);
			int oldValue = oldBalances.GetElement(oldIdx);
			
			string mappedCurrencyId = "";
			if (itemClassToCurrencyId.Find(oldKey, mappedCurrencyId))
			{
				// Add to currency ID balance (merge if already exists)
				int existingBalance1 = 0;
				if (playerData.Balance.Contains(mappedCurrencyId))
					existingBalance1 = playerData.Balance.Get(mappedCurrencyId);
				
				playerData.Balance.Set(mappedCurrencyId, existingBalance1 + oldValue);
				playerData.Balance.Remove(oldKey); // Remove old key
				Print("[AskalBalance] 🔄 Migrado: " + oldKey + " (" + oldValue + ") -> " + mappedCurrencyId);
			}
			else
			{
				// Unknown item class, try to map to default currency
				string defaultCurrencyId = s_MarketConfig.GetDefaultCurrencyId();
				int existingBalance2 = 0;
				if (playerData.Balance.Contains(defaultCurrencyId))
					existingBalance2 = playerData.Balance.Get(defaultCurrencyId);
				
				playerData.Balance.Set(defaultCurrencyId, existingBalance2 + oldValue);
				playerData.Balance.Remove(oldKey);
				Print("[AskalBalance] 🔄 Migrado (fallback): " + oldKey + " (" + oldValue + ") -> " + defaultCurrencyId);
			}
		}
		
		// Ensure all currencies have balance entries (even if 0)
		for (int currencyIdx = 0; currencyIdx < s_MarketConfig.Currencies.Count(); currencyIdx++)
		{
			string finalCurrencyId = s_MarketConfig.Currencies.GetKey(currencyIdx);
			AskalCurrencyConfig finalCurrencyConfig = s_MarketConfig.Currencies.GetElement(currencyIdx);
			if (!finalCurrencyConfig || finalCurrencyConfig.Mode == AskalMarketConstants.CURRENCY_MODE_DISABLED)
				continue;
			
			if (!playerData.Balance.Contains(finalCurrencyId))
				playerData.Balance.Set(finalCurrencyId, 0);
		}
	}
	
	// Ensure player file exists (called on first access)
	static void EnsurePlayerFileExists(string steamId)
	{
		if (!steamId || steamId == "")
		{
			Print("[AskalBalance] ❌ EnsurePlayerFileExists: steamId vazio!");
			return;
		}
		
		// ALWAYS check if file exists physically - don't trust cache
		string filePath = GetPlayerFilePath(steamId);
		if (!filePath || filePath == "")
		{
			Print("[AskalBalance] ❌ EnsurePlayerFileExists: Caminho do arquivo inválido para: " + steamId);
			return;
		}
		
		Print("[AskalBalance] 🔍 EnsurePlayerFileExists: Verificando arquivo: " + filePath);
		
		if (FileExist(filePath))
		{
			Print("[AskalBalance] ✅ EnsurePlayerFileExists: Arquivo já existe: " + filePath);
			return; // File already exists
		}
		
		// File doesn't exist, create it NOW
		Print("[AskalBalance] 🔄 EnsurePlayerFileExists: Arquivo NÃO existe, criando AGORA: " + steamId);
		Print("[AskalBalance] 🔄 Caminho completo: " + filePath);
		
		// Ensure directories exist FIRST
		Print("[AskalBalance] 🔧 EnsurePlayerFileExists: Garantindo que diretórios existam");
		Init();
		
		// Verify directory exists
		string playersDir = "$profile:Askal/Database/Players";
		if (!FileExist(playersDir))
		{
			Print("[AskalBalance] ⚠️ Diretório não existe, criando: " + playersDir);
			MakeDirectory(playersDir);
			if (!FileExist(playersDir))
			{
				Print("[AskalBalance] ❌ ERRO CRÍTICO: Não foi possível criar diretório: " + playersDir);
				return;
			}
			Print("[AskalBalance] ✅ Diretório criado com sucesso: " + playersDir);
		}
		
		Print("[AskalBalance] 🔧 EnsurePlayerFileExists: Criando dados padrão do player");
		AskalPlayerData playerData = CreateDefaultPlayerData();
		if (!playerData)
		{
			Print("[AskalBalance] ❌ ERRO CRÍTICO: CreateDefaultPlayerData retornou NULL para: " + steamId);
			return;
		}
		
		Print("[AskalBalance] 🔧 EnsurePlayerFileExists: Dados padrão criados, salvando arquivo");
		Print("[AskalBalance] 🔧 EnsurePlayerFileExists: Tentando salvar em: " + filePath);
		
		// Try to save multiple times if needed
		bool saveSuccess = false;
		for (int retryCount = 0; retryCount < 3; retryCount++)
		{
			if (retryCount > 0)
			{
				Print("[AskalBalance] 🔧 EnsurePlayerFileExists: Tentativa " + (retryCount + 1) + " de salvar arquivo...");
			}
			
			saveSuccess = SavePlayerData(steamId, playerData);
			if (saveSuccess)
			{
				// Verify file was actually created
				if (FileExist(filePath))
				{
					Print("[AskalBalance] ✅✅✅ ARQUIVO CRIADO E VERIFICADO COM SUCESSO: " + filePath);
					// Update cache
					s_Cache.Set(steamId, playerData);
					s_CacheTimestamps.Set(steamId, GetGame().GetTime());
					return; // Success!
				}
				else
				{
					Print("[AskalBalance] ⚠️ SavePlayerData retornou TRUE mas arquivo não existe, tentando novamente...");
					saveSuccess = false; // Mark as failed to retry
				}
			}
			else
			{
				Print("[AskalBalance] ⚠️ SavePlayerData retornou FALSE, tentando novamente...");
			}
		}
		
		// If we get here, all retries failed
		Print("[AskalBalance] ❌❌❌ ERRO CRÍTICO: Falha ao criar arquivo após 3 tentativas!");
		Print("[AskalBalance] ❌ Caminho esperado: " + filePath);
		Print("[AskalBalance] ❌ Verifique permissões e caminho do servidor!");
		string dirStatus = "NÃO";
		if (FileExist(playersDir))
			dirStatus = "SIM";
		Print("[AskalBalance] ❌ Diretório existe? " + dirStatus);
	}
	
	// Obter balance de uma moeda específica (usando currency ID)
	static int GetBalance(string steamId, string currencyId = "")
	{
		Print("[AskalBalance] 🔍 GetBalance: Chamado para steamId=" + steamId + " currencyId=" + currencyId);
		
		if (!steamId || steamId == "")
		{
			Print("[AskalBalance] ❌ GetBalance: steamId vazio!");
			return 0;
		}
		
		// Validate and load player config BEFORE any operation
		if (!ValidateAndLoadPlayerConfig(steamId))
		{
			Print("[AskalBalance] ❌ GetBalance: Player config validation failed for: " + steamId);
			return 0;
		}
		
		// Resolve currency ID (use default if not provided)
		if (!currencyId || currencyId == "")
		{
			EnsureMarketConfigLoaded();
			if (s_MarketConfig)
				currencyId = s_MarketConfig.GetDefaultCurrencyId();
			else
				currencyId = "Askal_Money";
		}
		
		// ITER-1: Use cache instead of always reading from disk
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		if (!playerData || !playerData.Balance)
			return 0;
		
		if (playerData.Balance.Contains(currencyId))
			return playerData.Balance.Get(currencyId);
		
		return 0;
	}
	
	// ITER-1: Reserve funds atomically (prevents double-spend)
	static bool ReserveFunds(string steamId, int amount, string currencyId = "")
	{
		Print("[AskalBalance] 🔍 ReserveFunds: Chamado para steamId=" + steamId + " amount=" + amount + " currencyId=" + currencyId);
		
		if (amount <= 0)
		{
			Print("[AskalBalance] ❌ ReserveFunds: amount inválido: " + amount);
			return false;
		}
		
		if (!steamId || steamId == "")
		{
			Print("[AskalBalance] RESERVE_FAIL steamId vazio");
			return false;
		}
		
		// Validate and load player config BEFORE any transaction
		if (!ValidateAndLoadPlayerConfig(steamId))
		{
			Print("[AskalBalance] RESERVE_FAIL steamId=" + steamId + " reason=config_invalid");
			return false;
		}
		
		// Resolve currency ID
		if (!currencyId || currencyId == "")
		{
			EnsureMarketConfigLoaded();
			if (s_MarketConfig)
				currencyId = s_MarketConfig.GetDefaultCurrencyId();
			else
				currencyId = "Askal_Money";
		}
		
		// Try to acquire lock (non-blocking)
		if (!TryLockPlayer(steamId))
		{
			Print("[AskalBalance] RESERVE_FAIL steamId=" + steamId + " reason=lock_timeout");
			return false;
		}
		
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		if (!playerData || !playerData.Balance)
		{
			UnlockPlayer(steamId);
			Print("[AskalBalance] RESERVE_FAIL steamId=" + steamId + " reason=no_player_data");
			return false;
		}
		
		int currentBalance = 0;
		if (playerData.Balance.Contains(currencyId))
			currentBalance = playerData.Balance.Get(currencyId);
		
		// Check available balance (current - reserved)
		int reserved = GetReservedAmount(steamId, currencyId);
		int available = currentBalance - reserved;
		
		if (available < amount)
		{
			UnlockPlayer(steamId);
			Print("[AskalBalance] RESERVE_FAIL steamId=" + steamId + " amount=" + amount + " currency=" + currencyId + " available=" + available + " reserved=" + reserved);
			return false;
		}
		
		// Add to reservations
		if (!s_Reservations.Contains(steamId))
			s_Reservations.Set(steamId, new map<string, int>());
		
		map<string, int> playerReservations = s_Reservations.Get(steamId);
		int currentReserved = 0;
		if (playerReservations.Contains(currencyId))
			currentReserved = playerReservations.Get(currencyId);
		
		playerReservations.Set(currencyId, currentReserved + amount);
		
		UnlockPlayer(steamId);
		Print("[AskalBalance] RESERVE_OK steamId=" + steamId + " amount=" + amount + " currency=" + currencyId + " available_after=" + (available - amount));
		return true;
	}
	
	// ITER-1: Get reserved amount for a currency
	private static int GetReservedAmount(string steamId, string currencyId)
	{
		if (!s_Reservations.Contains(steamId))
			return 0;
		
		map<string, int> playerReservations = s_Reservations.Get(steamId);
		if (!playerReservations.Contains(currencyId))
			return 0;
		
		return playerReservations.Get(currencyId);
	}
	
	// ITER-1: Confirm reservation (actually deduct balance)
	static bool ConfirmReservation(string steamId, int amount, string currencyId = "")
	{
		// Resolve currency ID
		if (!currencyId || currencyId == "")
		{
			EnsureMarketConfigLoaded();
			if (s_MarketConfig)
				currencyId = s_MarketConfig.GetDefaultCurrencyId();
			else
				currencyId = "Askal_Money";
		}
		
		if (!TryLockPlayer(steamId))
			return false;
		
		// Verify reservation exists
		if (!s_Reservations.Contains(steamId))
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		map<string, int> playerReservations = s_Reservations.Get(steamId);
		if (!playerReservations.Contains(currencyId))
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		int reserved = playerReservations.Get(currencyId);
		if (reserved < amount)
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		// Remove from reservations
		playerReservations.Set(currencyId, reserved - amount);
		if (playerReservations.Get(currencyId) == 0)
			playerReservations.Remove(currencyId);
		
		// Actually deduct balance
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		if (!playerData || !playerData.Balance)
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		int currentBalance = 0;
		if (playerData.Balance.Contains(currencyId))
			currentBalance = playerData.Balance.Get(currencyId);
		
		playerData.Balance.Set(currencyId, currentBalance - amount);
		
		// ITER-1: Enqueue to outbox instead of immediate write
		EnqueueToOutbox(steamId, playerData);
		
		UnlockPlayer(steamId);
		return true;
	}
	
	// ITER-1: Release reservation (rollback)
	static bool ReleaseReservation(string steamId, int amount, string currencyId = "")
	{
		// Resolve currency ID
		if (!currencyId || currencyId == "")
		{
			EnsureMarketConfigLoaded();
			if (s_MarketConfig)
				currencyId = s_MarketConfig.GetDefaultCurrencyId();
			else
				currencyId = "Askal_Money";
		}
		
		if (!TryLockPlayer(steamId))
			return false;
		
		if (!s_Reservations.Contains(steamId))
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		map<string, int> playerReservations = s_Reservations.Get(steamId);
		if (!playerReservations.Contains(currencyId))
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		int reserved = playerReservations.Get(currencyId);
		if (reserved < amount)
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		playerReservations.Set(currencyId, reserved - amount);
		if (playerReservations.Get(currencyId) == 0)
			playerReservations.Remove(currencyId);
		
		UnlockPlayer(steamId);
		return true;
	}
	
	// Adicionar balance
	static bool AddBalance(string steamId, int amount, string currencyId = "")
	{
		if (amount <= 0)
			return false;
		
		// Resolve currency ID
		if (!currencyId || currencyId == "")
		{
			EnsureMarketConfigLoaded();
			if (s_MarketConfig)
				currencyId = s_MarketConfig.GetDefaultCurrencyId();
			else
				currencyId = "Askal_Money";
		}
		
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		if (!playerData)
			return false;
		
		if (!playerData.Balance)
			playerData.Balance = new map<string, int>;
		
		int currentBalance = 0;
		if (playerData.Balance.Contains(currencyId))
			currentBalance = playerData.Balance.Get(currencyId);
		
		int newBalance = currentBalance + amount;
		playerData.Balance.Set(currencyId, newBalance);
		
		// ITER-1: Enqueue to outbox
		EnqueueToOutbox(steamId, playerData);
		return SavePlayerData(steamId, playerData);
	}
	
	// Remover balance (para compras)
	static bool RemoveBalance(string steamId, int amount, string currencyId = "")
	{
		if (amount <= 0)
			return false;
		
		// Resolve currency ID
		if (!currencyId || currencyId == "")
		{
			EnsureMarketConfigLoaded();
			if (s_MarketConfig)
				currencyId = s_MarketConfig.GetDefaultCurrencyId();
			else
				currencyId = "Askal_Money";
		}
		
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		if (!playerData)
			return false;
		
		if (!playerData.Balance)
			return false;
		
		int currentBalance = 0;
		if (playerData.Balance.Contains(currencyId))
			currentBalance = playerData.Balance.Get(currencyId);
		
		if (currentBalance < amount)
		{
			Print("[AskalBalance] ❌ Balance insuficiente: " + currentBalance + " < " + amount + " currency=" + currencyId);
			return false;
		}
		
		int newBalance = currentBalance - amount;
		playerData.Balance.Set(currencyId, newBalance);
		
		return SavePlayerData(steamId, playerData);
	}
	
	// Verificar se tem balance suficiente
	static bool HasEnoughBalance(string steamId, int amount, string currencyId = "")
	{
		int balance = GetBalance(steamId, currencyId);
		return balance >= amount;
	}
	
	// Limpar cache (útil para reload)
	static void ClearCache(string steamId = "")
	{
		// ITER-1: Actually clear cache
		if (!steamId || steamId == "")
		{
			s_Cache.Clear();
			s_CacheTimestamps.Clear();
		}
		else
		{
			s_Cache.Remove(steamId);
			s_CacheTimestamps.Remove(steamId);
		}
	}
	
	// ITER-1: Flush all pending saves (call on server shutdown)
	static void FlushAllPendingSaves()
	{
		FlushOutbox();
	}
}

