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
	private static bool s_Initialized = false;
	private static bool s_MarketConfigLoaded = false;
	private static ref AskalMarketConfig s_MarketConfig;

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
		bool isNewFile = false;

		if (FileExist(filePath))
		{
			if (AskalJsonLoader<AskalPlayerData>.LoadFromFile(filePath, playerData, false))
			{
				// Garante estruturas válidas mesmo se JSON foi editado manualmente
				if (!playerData.Balance)
					playerData.Balance = new map<string, int>;
				if (!playerData.Permissions)
					playerData.Permissions = new map<string, int>;
				
				// Patch balance with missing currencies
				PatchPlayerBalance(steamId, playerData);
				return playerData;
			}
			Print("[AskalBalance] ⚠️ Erro ao carregar dados do player, recriando: " + steamId);
		}
		else
		{
			Print("[AskalBalance] ⚠️ Arquivo do player não encontrado, criando novo: " + steamId);
			isNewFile = true;
		}

		playerData = CreateDefaultPlayerData();
		if (isNewFile)
		{
			// Set FirstLogin to current UTC time
			playerData.FirstLogin = GetDateString();
			Print("[AskalBalance] 📝 Player JSON criado: " + steamId + " | FirstLogin: " + playerData.FirstLogin);
		}
		SavePlayerData(steamId, playerData);
		return playerData;
	}
	
	// Patch player balance to ensure all currencies from MarketConfig are present
	static void PatchPlayerBalance(string steamId, AskalPlayerData playerData)
	{
		if (!steamId || steamId == "" || !playerData)
			return;
		
		EnsureMarketConfigLoaded();
		if (!s_MarketConfig || !s_MarketConfig.Currencies)
			return;
		
		if (!playerData.Balance)
			playerData.Balance = new map<string, int>;
		
		bool balancePatched = false;
		array<string> addedCurrencies = new array<string>();
		
		// For each currency in MarketConfig, ensure it exists in player Balance
		// Always use currencyId as the balance key (not Value.Name)
		for (int currencyIdx = 0; currencyIdx < s_MarketConfig.Currencies.Count(); currencyIdx++)
		{
			string currencyId = s_MarketConfig.Currencies.GetKey(currencyIdx);
			AskalCurrencyConfig currencyCfg = s_MarketConfig.Currencies.GetElement(currencyIdx);
			if (!currencyCfg)
				continue;
			
			// Always use currencyId as balance key (not Value.Name)
			// Value.Name is only for physical item representation, balance uses currencyID
			string balanceKey = currencyId;
			
			// Only add if it doesn't exist (preserve existing balance)
			if (!playerData.Balance.Contains(balanceKey))
			{
				int startAmount = currencyCfg.StartCurrency;
				playerData.Balance.Set(balanceKey, startAmount);
				addedCurrencies.Insert(balanceKey);
				balancePatched = true;
				Print("[AskalBalance] 💰 Currency adicionada ao balance: " + balanceKey + " = " + startAmount);
			}
		}
		
		if (balancePatched)
		{
			SavePlayerData(steamId, playerData);
			Print("[AskalBalance] ✅ Balance atualizado para player " + steamId + " | " + addedCurrencies.Count() + " currencies adicionadas");
		}
	}
	
	// Get current UTC date as ISO string
	static string GetDateString()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);
		return string.Format("%1-%2-%3T%4:%5:%6Z", year.ToString(), month.ToString(), day.ToString(), hour.ToString(), minute.ToString(), second.ToString());
	}

	static AskalPlayerData CreateDefaultPlayerData()
	{
		AskalPlayerData playerData = new AskalPlayerData();
		playerData.FirstLogin = "";
		EnsureMarketConfigLoaded();

		if (s_MarketConfig && s_MarketConfig.Currencies)
		{
			for (int currencyIdx = 0; currencyIdx < s_MarketConfig.Currencies.Count(); currencyIdx++)
			{
				string currencyId = s_MarketConfig.Currencies.GetKey(currencyIdx);
				AskalCurrencyConfig currencyConfig = s_MarketConfig.Currencies.GetElement(currencyIdx);
				if (!currencyConfig)
					continue;

				int startAmount = currencyConfig.StartCurrency;
				
				// Always use currencyId as balance key (not Value.Name)
				// Value.Name is only for physical item representation, balance uses currencyID
				string balanceKey = currencyId;

				playerData.Balance.Set(balanceKey, startAmount);
				Print("[AskalBalance] 💰 StartCurrency aplicado: " + balanceKey + " = " + startAmount);
			}
		}

		return playerData;
	}
	
	// Salvar dados do player
	static bool SavePlayerData(string steamId, AskalPlayerData playerData)
	{
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
	
	// Obter balance de uma moeda específica
	static int GetBalance(string steamId, string currency = "Askal_Coin")
	{
		AskalPlayerData playerData = LoadPlayerData(steamId);
		if (!playerData || !playerData.Balance)
			return 0;
		
		if (playerData.Balance.Contains(currency))
			return playerData.Balance.Get(currency);
		
		return 0;
	}
	
	// Adicionar balance
	static bool AddBalance(string steamId, int amount, string currency = "Askal_Coin")
	{
		if (amount <= 0)
			return false;
		
		AskalPlayerData playerData = LoadPlayerData(steamId);
		if (!playerData)
			return false;
		
		if (!playerData.Balance)
			playerData.Balance = new map<string, int>;
		
		int currentBalance = 0;
		if (playerData.Balance.Contains(currency))
			currentBalance = playerData.Balance.Get(currency);
		
		int newBalance = currentBalance + amount;
		playerData.Balance.Set(currency, newBalance);
		
		return SavePlayerData(steamId, playerData);
	}
	
	// Remover balance (para compras)
	static bool RemoveBalance(string steamId, int amount, string currency = "Askal_Coin")
	{
		if (amount <= 0)
			return false;
		
		AskalPlayerData playerData = LoadPlayerData(steamId);
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
	
	// Resolve balance key from currencyId
	// Always returns currencyId (balance always uses currencyID, not Value.Name)
	// Value.Name is only for physical item representation in inventory
	static string ResolveBalanceKey(string currencyId)
	{
		if (!currencyId || currencyId == "")
			return "";
		
		// Balance always uses currencyID directly
		return currencyId;
	}
	
	// Limpar cache (útil para reload)
	static void ClearCache(string steamId = "")
	{
		// Sem cache em memória; mantido por compatibilidade
		Print("[AskalBalance] ClearCache chamado, mas cache em memória está desativado");
	}
}

