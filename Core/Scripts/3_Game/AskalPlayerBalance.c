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

	// ITER-1: Get cached data or load from disk
	private static AskalPlayerData GetCachedOrLoad(string steamId)
	{
		float now = GetGame().GetTime();
		if (s_Cache.Contains(steamId))
		{
			float cacheTime = s_CacheTimestamps.Get(steamId);
			if (now - cacheTime < CACHE_TTL)
			{
				return s_Cache.Get(steamId);
			}
			s_Cache.Remove(steamId);
			s_CacheTimestamps.Remove(steamId);
		}

		AskalPlayerData data = LoadPlayerData(steamId);
		if (data)
		{
			s_Cache.Set(steamId, data);
			s_CacheTimestamps.Set(steamId, now);
		}
		return data;
	}

	static void Init()
	{
		if (s_Initialized)
			return;

		string baseDir = "$profile:Askal";
		if (!FileExist(baseDir))
			MakeDirectory(baseDir);

		string dbDir = baseDir + "/Database";
		if (!FileExist(dbDir))
			MakeDirectory(dbDir);

		string playersDir = dbDir + "/Players";
		if (!FileExist(playersDir))
			MakeDirectory(playersDir);

		// ITER-1: Initialize outbox flush timer
		s_LastOutboxFlush = GetGame().GetTime();

		Print("[AskalBalance] Sistema de balance inicializado (persistência em tempo real)");
		s_Initialized = true;
	}

	static void EnsureMarketConfigLoaded()
	{
		if (s_MarketConfigLoaded)
			return;

		s_MarketConfig = AskalMarketConfig.GetInstance();

		s_MarketConfigLoaded = true;
	}
	
	// Obter caminho do arquivo JSON do player
	static string GetPlayerFilePath(string steamId)
	{
		if (!steamId || steamId == "")
			return "";
		
		return "$profile:Askal/Database/Players/" + steamId + ".json";
	}
	
	// Carregar dados do player (lê sempre do disco)
	static AskalPlayerData LoadPlayerData(string steamId)
	{
		if (!steamId || steamId == "")
			return NULL;

		Init();

		string filePath = GetPlayerFilePath(steamId);
		AskalPlayerData playerData = new AskalPlayerData();

		if (FileExist(filePath))
		{
			if (AskalJsonLoader<AskalPlayerData>.LoadFromFile(filePath, playerData, false))
			{
				// Garante estruturas válidas mesmo se JSON foi editado manualmente
				if (!playerData.Balance)
					playerData.Balance = new map<string, int>;
				if (!playerData.Permissions)
					playerData.Permissions = new map<string, int>;
				return playerData;
			}
			Print("[AskalBalance] ⚠️ Erro ao carregar dados do player, recriando: " + steamId);
		}
		else
		{
			Print("[AskalBalance] ⚠️ Arquivo do player não encontrado, criando novo: " + steamId);
		}

		playerData = CreateDefaultPlayerData();
		SavePlayerData(steamId, playerData);
		return playerData;
	}

	static AskalPlayerData CreateDefaultPlayerData()
	{
		AskalPlayerData playerData = new AskalPlayerData();
		playerData.FirstLogin = "0";
		EnsureMarketConfigLoaded();

		if (s_MarketConfig && s_MarketConfig.Currencies)
		{
			for (int currencyIdx = 0; currencyIdx < s_MarketConfig.Currencies.Count(); currencyIdx++)
			{
				string walletId = s_MarketConfig.Currencies.GetKey(currencyIdx);
				AskalCurrencyConfig currencyConfig = s_MarketConfig.Currencies.GetElement(currencyIdx);
				if (!currencyConfig)
					continue;

				int startAmount = currencyConfig.StartCurrency;
				if (startAmount <= 0)
					continue;

				if (!currencyConfig.Values || currencyConfig.Values.Count() == 0)
					continue;

				AskalCurrencyValueConfig defaultValue = currencyConfig.Values.Get(0);
				if (!defaultValue || !defaultValue.Name || defaultValue.Name == "")
					continue;

				playerData.Balance.Set(defaultValue.Name, startAmount);
				Print("[AskalBalance] 💰 StartCurrency aplicado: " + defaultValue.Name + " = " + startAmount);
			}
		}

		if (!playerData.Balance.Contains("Askal_Coin"))
			playerData.Balance.Set("Askal_Coin", 0);
		return playerData;
	}
	
	// Salvar dados do player
	static bool SavePlayerData(string steamId, AskalPlayerData playerData)
	{
		// ITER-1: Update cache
		if (s_Cache.Contains(steamId))
		{
			s_Cache.Set(steamId, playerData);
			s_CacheTimestamps.Set(steamId, GetGame().GetTime());
		}

		if (!steamId || steamId == "" || !playerData)
			return false;
		
		Init();
		
		string filePath = GetPlayerFilePath(steamId);
		bool success = AskalJsonLoader<AskalPlayerData>.SaveToFile(filePath, playerData);
		
		if (success)
		{
			Print("[AskalBalance] ✅ Dados do player salvos: " + steamId);
		}
		else
		{
			Print("[AskalBalance] ❌ Erro ao salvar dados do player: " + steamId);
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
	
	// Obter balance de uma moeda específica
	static int GetBalance(string steamId, string currency = "Askal_Coin")
	{
		// ITER-1: Use cache instead of always reading from disk
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		if (!playerData || !playerData.Balance)
			return 0;
		
		if (playerData.Balance.Contains(currency))
			return playerData.Balance.Get(currency);
		
		return 0;
	}
	
	// ITER-1: Reserve funds atomically (prevents double-spend)
	static bool ReserveFunds(string steamId, int amount, string currency = "Askal_Coin")
	{
		if (amount <= 0)
			return false;
		
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
		if (playerData.Balance.Contains(currency))
			currentBalance = playerData.Balance.Get(currency);
		
		// Check available balance (current - reserved)
		int reserved = GetReservedAmount(steamId, currency);
		int available = currentBalance - reserved;
		
		if (available < amount)
		{
			UnlockPlayer(steamId);
			Print("[AskalBalance] RESERVE_FAIL steamId=" + steamId + " amount=" + amount + " available=" + available + " reserved=" + reserved);
			return false;
		}
		
		// Add to reservations
		if (!s_Reservations.Contains(steamId))
			s_Reservations.Set(steamId, new map<string, int>());
		
		map<string, int> playerReservations = s_Reservations.Get(steamId);
		int currentReserved = 0;
		if (playerReservations.Contains(currency))
			currentReserved = playerReservations.Get(currency);
		
		playerReservations.Set(currency, currentReserved + amount);
		
		UnlockPlayer(steamId);
		Print("[AskalBalance] RESERVE_OK steamId=" + steamId + " amount=" + amount + " currency=" + currency + " available_after=" + (available - amount));
		return true;
	}
	
	// ITER-1: Get reserved amount for a currency
	private static int GetReservedAmount(string steamId, string currency)
	{
		if (!s_Reservations.Contains(steamId))
			return 0;
		
		map<string, int> playerReservations = s_Reservations.Get(steamId);
		if (!playerReservations.Contains(currency))
			return 0;
		
		return playerReservations.Get(currency);
	}
	
	// ITER-1: Confirm reservation (actually deduct balance)
	static bool ConfirmReservation(string steamId, int amount, string currency = "Askal_Coin")
	{
		if (!TryLockPlayer(steamId))
			return false;
		
		// Verify reservation exists
		if (!s_Reservations.Contains(steamId))
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		map<string, int> playerReservations = s_Reservations.Get(steamId);
		if (!playerReservations.Contains(currency))
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		int reserved = playerReservations.Get(currency);
		if (reserved < amount)
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		// Remove from reservations
		playerReservations.Set(currency, reserved - amount);
		if (playerReservations.Get(currency) == 0)
			playerReservations.Remove(currency);
		
		// Actually deduct balance
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		int currentBalance = playerData.Balance.Get(currency);
		playerData.Balance.Set(currency, currentBalance - amount);
		
		// ITER-1: Enqueue to outbox instead of immediate write
		EnqueueToOutbox(steamId, playerData);
		
		UnlockPlayer(steamId);
		return true;
	}
	
	// ITER-1: Release reservation (rollback)
	static bool ReleaseReservation(string steamId, int amount, string currency = "Askal_Coin")
	{
		if (!TryLockPlayer(steamId))
			return false;
		
		if (!s_Reservations.Contains(steamId))
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		map<string, int> playerReservations = s_Reservations.Get(steamId);
		if (!playerReservations.Contains(currency))
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		int reserved = playerReservations.Get(currency);
		if (reserved < amount)
		{
			UnlockPlayer(steamId);
			return false;
		}
		
		playerReservations.Set(currency, reserved - amount);
		if (playerReservations.Get(currency) == 0)
			playerReservations.Remove(currency);
		
		UnlockPlayer(steamId);
		return true;
	}
	
	// Adicionar balance
	static bool AddBalance(string steamId, int amount, string currency = "Askal_Coin")
	{
		if (amount <= 0)
			return false;
		
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		if (!playerData)
			return false;
		
		if (!playerData.Balance)
			playerData.Balance = new map<string, int>;
		
		int currentBalance = 0;
		if (playerData.Balance.Contains(currency))
			currentBalance = playerData.Balance.Get(currency);
		
		int newBalance = currentBalance + amount;
		playerData.Balance.Set(currency, newBalance);
		
		// ITER-1: Enqueue to outbox
		EnqueueToOutbox(steamId, playerData);
		return SavePlayerData(steamId, playerData);
	}
	
	// Remover balance (para compras)
	static bool RemoveBalance(string steamId, int amount, string currency = "Askal_Coin")
	{
		if (amount <= 0)
			return false;
		
		AskalPlayerData playerData = GetCachedOrLoad(steamId);
		if (!playerData)
			return false;
		
		if (!playerData.Balance)
			return false;
		
		int currentBalance = 0;
		if (playerData.Balance.Contains(currency))
			currentBalance = playerData.Balance.Get(currency);
		
		if (currentBalance < amount)
		{
			Print("[AskalBalance] ❌ Balance insuficiente: " + currentBalance + " < " + amount);
			return false;
		}
		
		int newBalance = currentBalance - amount;
		playerData.Balance.Set(currency, newBalance);
		
		return SavePlayerData(steamId, playerData);
	}
	
	// Verificar se tem balance suficiente
	static bool HasEnoughBalance(string steamId, int amount, string currency = "Askal_Coin")
	{
		int balance = GetBalance(steamId, currency);
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

