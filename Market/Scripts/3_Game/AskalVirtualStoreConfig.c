class AskalVirtualStoreConfig
{
	string Version;
	int LogsLevel;
	int VirtualStoreMode;
	string AcceptedCurrency;
	ref array<string> AcceptedCurrencyList;
	ref map<string, int> SetupItems;
	float BuyCoefficient;
	float SellCoefficient;
	
	void AskalVirtualStoreConfig()
	{
		AcceptedCurrency = "";
		AcceptedCurrencyList = new array<string>();
		SetupItems = new map<string, int>();
		BuyCoefficient = 1.0;
		SellCoefficient = 1.0;
	}
	
	void NormalizeAcceptedCurrency()
	{
		if (!AcceptedCurrencyList)
			AcceptedCurrencyList = new array<string>();
		
		if (AcceptedCurrency && AcceptedCurrency != "")
		{
			if (AcceptedCurrencyList.Find(AcceptedCurrency) == -1)
				AcceptedCurrencyList.Insert(AcceptedCurrency);
			AcceptedCurrency = "";
		}
	}
	
	void EnsureDefaults()
	{
		if (BuyCoefficient <= 0)
			BuyCoefficient = 1.0;
		if (SellCoefficient <= 0)
			SellCoefficient = 1.0;
	}
	
	string GetPrimaryCurrency()
	{
		if (AcceptedCurrencyList && AcceptedCurrencyList.Count() > 0)
			return AcceptedCurrencyList.Get(0);
		return "";
	}
	
	static AskalVirtualStoreConfig LoadFromAny()
	{
		array<string> candidatePaths = {
			"$profile:config/Askal/Market/VirtualStore_Config.json",
			"$profile:config\\Askal\\Market\\VirtualStore_Config.json",
			"$profile:Askal/Market/VirtualStore_Config.json",
			"$profile:Askal\\Market\\VirtualStore_Config.json",
			"$mission:Askal/Market/VirtualStore_Config.json",
			"$mission:Askal\\Market\\VirtualStore_Config.json",
			"Askal/Market/VirtualStore_Config.json",
			"Askal\\Market\\VirtualStore_Config.json",
			"config/Askal/Market/VirtualStore_Config.json",
			"config\\Askal\\Market\\VirtualStore_Config.json"
		};
		
		AskalVirtualStoreConfig loadedConfig = NULL;
		foreach (string path : candidatePaths)
		{
			AskalVirtualStoreConfig tempConfig;
			if (AskalMarketLoader.LoadVirtualStoreConfig(path, tempConfig) && tempConfig)
			{
				tempConfig.NormalizeAcceptedCurrency();
				tempConfig.EnsureDefaults();
				loadedConfig = tempConfig;
				
				// Resolve and validate AcceptedCurrency
				string acceptedCurrency = tempConfig.GetPrimaryCurrency();
				int currencyMode = -1;
				AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
				if (marketConfig && acceptedCurrency != "")
				{
					AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(acceptedCurrency);
					if (currencyCfg)
					{
						currencyMode = currencyCfg.Mode;
					}
					else
					{
						Print("[AskalVirtualStoreConfig] ⚠️ Currency " + acceptedCurrency + " not defined in MarketConfig, using default");
						acceptedCurrency = marketConfig.GetDefaultCurrencyId();
						if (acceptedCurrency != "")
						{
							AskalCurrencyConfig defaultCfg = marketConfig.GetCurrencyOrNull(acceptedCurrency);
							if (defaultCfg)
								currencyMode = defaultCfg.Mode;
						}
					}
				}
				
				int setupItemsCount = 0;
				if (tempConfig.SetupItems)
					setupItemsCount = tempConfig.SetupItems.Count();
				
				Print("[AskalVirtualStoreConfig] ✅ Config loaded from: " + path + " | AcceptedCurrency: " + acceptedCurrency + " (mode=" + currencyMode + ") | SetupItems loaded: " + setupItemsCount);
				break;
			}
		}
		
		if (!loadedConfig)
		{
			loadedConfig = new AskalVirtualStoreConfig();
			loadedConfig.NormalizeAcceptedCurrency();
			loadedConfig.EnsureDefaults();
			Print("[AskalVirtualStoreConfig] ⚠️ Config não encontrada. Usando valores padrão.");
		}
		
		return loadedConfig;
	}
}

class AskalVirtualStoreSettings
{
	protected static ref AskalVirtualStoreConfig s_Config;
	protected static bool s_ConfigSynced = false;
	
	static AskalVirtualStoreConfig GetConfig()
	{
		if (!s_Config)
		{
			if (GetGame() && GetGame().IsServer())
			{
				s_Config = AskalVirtualStoreConfig.LoadFromAny();
			}
			else
			{
				s_Config = new AskalVirtualStoreConfig();
				s_Config.NormalizeAcceptedCurrency();
				s_Config.EnsureDefaults();
			}
		}
		if (!s_Config)
		{
			s_Config = new AskalVirtualStoreConfig();
			s_Config.NormalizeAcceptedCurrency();
			s_Config.EnsureDefaults();
		}
		return s_Config;
	}
	
	static void ApplyConfigFromServer(string currencyId, float buyCoeff, float sellCoeff, array<string> setupKeys, array<int> setupValues)
	{
		AskalVirtualStoreConfig config = GetConfig();
		
		config.BuyCoefficient = buyCoeff;
		config.SellCoefficient = sellCoeff;
		
		if (!config.AcceptedCurrencyList)
			config.AcceptedCurrencyList = new array<string>();
		config.AcceptedCurrencyList.Clear();
		if (currencyId && currencyId != "")
			config.AcceptedCurrencyList.Insert(currencyId);
		config.AcceptedCurrency = "";
		
		if (!config.SetupItems)
			config.SetupItems = new map<string, int>();
		config.SetupItems.Clear();
		
		if (setupKeys && setupValues && setupKeys.Count() == setupValues.Count())
		{
			for (int i = 0; i < setupKeys.Count(); i++)
			{
				string key = setupKeys.Get(i);
				int mode = setupValues.Get(i);
				if (key && key != "")
					config.SetupItems.Set(key, mode);
			}
		}
		
		config.NormalizeAcceptedCurrency();
		config.EnsureDefaults();
		s_ConfigSynced = true;
	}
	
	static bool IsConfigSynced()
	{
		return s_ConfigSynced;
	}
	
	static float GetBuyCoefficient()
	{
		return GetConfig().BuyCoefficient;
	}
	
	static float GetSellCoefficient()
	{
		return GetConfig().SellCoefficient;
	}
	
	static string GetPrimaryCurrency()
	{
		return GetConfig().GetPrimaryCurrency();
	}
}