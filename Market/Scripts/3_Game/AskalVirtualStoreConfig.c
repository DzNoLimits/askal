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
				Print("[AskalVirtualStoreConfig] ✅ Config carregada de: " + path);
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
	
	static void ApplyConfigFromServer(string currencyId, float buyCoeff, float sellCoeff, int virtualStoreMode, array<string> setupKeys, array<int> setupValues)
	{
		AskalVirtualStoreConfig config = GetConfig();
		
		config.BuyCoefficient = buyCoeff;
		config.SellCoefficient = sellCoeff;
		config.VirtualStoreMode = virtualStoreMode;
		
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
		
		Print("[AskalVirtualStore] ✅ Config sincronizada do servidor: VirtualStoreMode=" + virtualStoreMode.ToString());
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
	
	// Verificar se o Virtual Store está habilitado
	static bool IsVirtualStoreEnabled()
	{
		AskalVirtualStoreConfig config = NULL;
		bool enabled = false;
		
		// No servidor, carregar diretamente do arquivo para garantir que está atualizado
		if (GetGame() && GetGame().IsServer())
		{
			config = AskalVirtualStoreConfig.LoadFromAny();
			if (!config)
			{
				Print("[AskalVirtualStore] ⚠️ IsVirtualStoreEnabled: Config não encontrada no servidor - retornando false");
				return false;
			}
			// Garantir que VirtualStoreMode seja 0 ou 1
			if (config.VirtualStoreMode != 0 && config.VirtualStoreMode != 1)
			{
				Print("[AskalVirtualStore] ⚠️ IsVirtualStoreEnabled: VirtualStoreMode inválido (" + config.VirtualStoreMode + ") - usando padrão (enabled)");
				return true; // Default: Enabled
			}
			enabled = config.VirtualStoreMode == 1;
			Print("[AskalVirtualStore] 🔍 IsVirtualStoreEnabled (servidor): VirtualStoreMode=" + config.VirtualStoreMode + " → " + enabled.ToString());
			return enabled;
		}
		
		// No cliente, usar config sincronizada
		config = GetConfig();
		if (!config)
		{
			Print("[AskalVirtualStore] ⚠️ IsVirtualStoreEnabled: Config NULL no cliente - retornando false");
			return false;
		}
		
		// Se a config foi sincronizada do servidor, usar ela
		// Caso contrário, no cliente não podemos confiar (retornar false para segurança)
		if (!s_ConfigSynced)
		{
			// No cliente, se não foi sincronizado, não assumir que está habilitado
			// Isso evita que o menu abra antes da config chegar
			Print("[AskalVirtualStore] ⚠️ IsVirtualStoreEnabled: Config não sincronizada no cliente - retornando false");
			return false;
		}
		
		// Garantir que VirtualStoreMode seja 0 ou 1
		if (config.VirtualStoreMode != 0 && config.VirtualStoreMode != 1)
		{
			Print("[AskalVirtualStore] ⚠️ IsVirtualStoreEnabled: VirtualStoreMode inválido (" + config.VirtualStoreMode + ") - usando padrão (enabled)");
			return true; // Default: Enabled
		}
		enabled = config.VirtualStoreMode == 1;
		Print("[AskalVirtualStore] 🔍 IsVirtualStoreEnabled (cliente): VirtualStoreMode=" + config.VirtualStoreMode + " → " + enabled.ToString());
		return enabled;
	}
	
	// Obter o modo do Virtual Store
	static int GetVirtualStoreMode()
	{
		AskalVirtualStoreConfig config = GetConfig();
		if (!config)
			return 1; // Default: Enabled
		return config.VirtualStoreMode;
	}
}