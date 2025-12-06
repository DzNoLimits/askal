// ==========================================================
//	Askal Market Configuration
//	Leitura de MarketConfig.json (moedas, líquidos, etc)
// ==========================================================

class AskalCurrencyValueConfig
{
	string Name;
	int Value;
	
	void AskalCurrencyValueConfig()
	{
		Name = "";
		Value = 1;
	}
}

class AskalCurrencyConfig
{
	string WalletId;
	string ShortName;
	int Mode; // 0=disabled, 1=physical (item-based), 2=virtual
	int StartCurrency;
	ref array<ref AskalCurrencyValueConfig> Values;
	
	void AskalCurrencyConfig()
	{
		WalletId = "";
		ShortName = "";
		Mode = 0;
		StartCurrency = 0;
		Values = new array<ref AskalCurrencyValueConfig>();
	}
}

class AskalMarketConfigFile
{
	string Version;
	string Description;
	string WarnText;
	int DelayTimeMS;
	string DefaultCurrencyId;
	ref map<string, ref AskalCurrencyConfig> Currencies;
	ref map<string, float> Liquids;
	ref map<string, string> LiquidNames;
	
	void AskalMarketConfigFile()
	{
		WarnText = "";
		DelayTimeMS = 500;
		DefaultCurrencyId = "";
		Currencies = new map<string, ref AskalCurrencyConfig>();
		Liquids = new map<string, float>();
		LiquidNames = new map<string, string>();
	}
}

class AskalMarketConfig
{
	protected static ref AskalMarketConfig s_Instance;
	
	// Dados processados
	protected ref map<int, float> m_LiquidPrices; // liquidType -> price per mL
	protected ref map<int, string> m_LiquidNames; // liquidType -> display
	ref map<string, ref AskalCurrencyConfig> Currencies; // walletId -> config
	string DefaultCurrencyId;
	string WarnText;
	int DelayTimeMS;
	
	void AskalMarketConfig()
	{
		m_LiquidPrices = new map<int, float>();
		m_LiquidNames = new map<int, string>();
		Currencies = new map<string, ref AskalCurrencyConfig>();
		WarnText = "";
		DelayTimeMS = 500; // Default: 500ms
		LoadConfig();
	}
	
	static AskalMarketConfig GetInstance()
	{
		if (!s_Instance)
			s_Instance = new AskalMarketConfig();
		return s_Instance;
	}
	
	protected void LoadConfig()
	{
		array<string> candidatePaths = {

			"$profile:Askal/Market/MarketConfig.json"
		};
		
		Print("[AskalMarket] 🔍 Procurando MarketConfig.json em " + candidatePaths.Count() + " caminhos...");
		foreach (string path : candidatePaths)
		{
			Print("[AskalMarket] 🔍 Tentando: " + path);
			if (LoadFromPath(path))
			{
				Print("[AskalMarket] ✅ MarketConfig carregada de " + path);
				return;
			}
		}
		
		Print("[AskalMarket] ⚠️ MarketConfig não encontrada em nenhum caminho. Aplicando valores padrão.");
		LoadDefaults();
		
	}
	
	bool LoadFromPath(string path)
	{
		if (!FileExist(path))
		{
			Print("[AskalMarket] ⚠️ Arquivo não existe: " + path);
			return false;
		}
		
		Print("[AskalMarket] 🔍 Tentando carregar MarketConfig de: " + path);
		AskalMarketConfigFile fileData = new AskalMarketConfigFile();
		JsonFileLoader<AskalMarketConfigFile>.JsonLoadFile(path, fileData);
		
		// Valida se carregou algo válido
		if (!fileData)
		{
			Print("[AskalMarket] ⚠️ fileData é NULL após JsonLoadFile: " + path);
			return false;
		}
		
		// Valida se tem Currencies
		if (!fileData.Currencies || fileData.Currencies.Count() == 0)
		{
			Print("[AskalMarket] ⚠️ fileData.Currencies está vazio ou NULL: " + path);
			return false;
		}
		
		Print("[AskalMarket] ✅ fileData carregado com " + fileData.Currencies.Count() + " currencies de: " + path);
		ApplyFileData(fileData);
		return true;
	}
	
	void ApplyFileData(AskalMarketConfigFile fileData)
	{
		m_LiquidPrices.Clear();
		m_LiquidNames.Clear();
		Currencies.Clear();
		WarnText = fileData.WarnText;
		DelayTimeMS = fileData.DelayTimeMS;
		if (DelayTimeMS <= 0)
			DelayTimeMS = 500; // Fallback
		DefaultCurrencyId = fileData.DefaultCurrencyId;
		if (!DefaultCurrencyId || DefaultCurrencyId == "")
			DefaultCurrencyId = "Askal_Money";
		
		if (fileData.Liquids)
		{
			foreach (string liquidKey, float price : fileData.Liquids)
			{
				int liquidType = liquidKey.ToInt();
				if (liquidType > 0)
					m_LiquidPrices.Insert(liquidType, price);
			}
		}
		
		if (fileData.LiquidNames)
		{
			foreach (string liquidNameKey, string displayName : fileData.LiquidNames)
			{
				int liquidTypeName = liquidNameKey.ToInt();
				if (liquidTypeName > 0)
					m_LiquidNames.Insert(liquidTypeName, displayName);
			}
		}
		
		if (fileData.Currencies)
		{
			foreach (string currencyId, AskalCurrencyConfig currencyCfg : fileData.Currencies)
			{
				if (!currencyCfg)
					continue;
				
				// Garante estruturas válidas
				if (!currencyCfg.Values)
					currencyCfg.Values = new array<ref AskalCurrencyValueConfig>();
				
				// Backfill do nome do wallet
				if (!currencyCfg.WalletId || currencyCfg.WalletId == "")
					currencyCfg.WalletId = currencyId;
				
				Currencies.Insert(currencyId, currencyCfg);
			}
		}
		
		// Log currency count
		Print("[AskalMarket] ✅ MarketConfig loaded with " + Currencies.Count() + " currencies");
		
		// Garante pelo menos uma moeda padrão
		if (Currencies.Count() == 0)
		{
			AddDefaultCurrency();
		}
		
		if (m_LiquidPrices.Count() == 0)
		{
			LoadDefaultLiquids();
		}
	}
	
	protected void LoadDefaults()
	{
		Currencies.Clear();
		m_LiquidPrices.Clear();
		m_LiquidNames.Clear();
		WarnText = "";
		DelayTimeMS = 500;
		
		AddDefaultCurrency();
		LoadDefaultLiquids();
	}
	
	int GetDelayTimeMS()
	{
		return DelayTimeMS;
	}
	
	protected void AddDefaultCurrency()
	{
		AskalCurrencyConfig defaultCurrency = new AskalCurrencyConfig();
		defaultCurrency.WalletId = "Askal_DefaultWallet";
		defaultCurrency.ShortName = "AKC";
		defaultCurrency.Mode = 1;
		defaultCurrency.StartCurrency = 0;
		
		AskalCurrencyValueConfig defaultValue = new AskalCurrencyValueConfig();
		defaultValue.Name = "ASK_Coin";
		defaultValue.Value = 1;
		defaultCurrency.Values.Insert(defaultValue);
		
		Currencies.Insert(defaultCurrency.WalletId, defaultCurrency);
	}
	
	protected void LoadDefaultLiquids()
	{
		// Valores corretos segundo constants.c do DayZ
		m_LiquidPrices.Insert(512, 0.001);     // LIQUID_WATER
		m_LiquidPrices.Insert(1024, 0.0008);   // LIQUID_RIVERWATER
		m_LiquidPrices.Insert(2048, 0.01);     // LIQUID_VODKA
		m_LiquidPrices.Insert(4096, 0.005);    // LIQUID_BEER
		m_LiquidPrices.Insert(8192, 0.008);    // LIQUID_GASOLINE
		m_LiquidPrices.Insert(16384, 0.015);   // LIQUID_DIESEL
		m_LiquidPrices.Insert(32768, 0.008);   // LIQUID_DISINFECTANT
		
		m_LiquidNames.Insert(512, "Água Potável");
		m_LiquidNames.Insert(1024, "Água");
		m_LiquidNames.Insert(2048, "Vodka");
		m_LiquidNames.Insert(4096, "Cerveja");
		m_LiquidNames.Insert(8192, "Gasolina");
		m_LiquidNames.Insert(16384, "Diesel");
		m_LiquidNames.Insert(32768, "Desinfetante");
	}
	
	// ======================================================
	//	Getters
	// ======================================================
	float GetLiquidPricePerML(int liquidType)
	{
		float price;
		if (m_LiquidPrices.Find(liquidType, price))
			return price;
		return 0.001;
	}
	
	string GetLiquidName(int liquidType)
	{
		string name;
		if (m_LiquidNames.Find(liquidType, name))
			return name;
		return "Unknown Liquid";
	}
	
	void GetAllLiquidTypes(out array<int> types)
	{
		types = m_LiquidPrices.GetKeyArray();
	}
	
	AskalCurrencyConfig GetCurrencyConfig(string currencyId)
	{
		AskalCurrencyConfig config;
		if (Currencies.Find(currencyId, config))
			return config;
		return NULL;
	}
	
	string GetDefaultCurrencyId()
	{
		return DefaultCurrencyId;
	}
	
	// Resolve accepted currency for trader or virtual store
	// Returns currencyId and currency config via out params
	// Returns true if valid currency found, false otherwise
	static bool ResolveAcceptedCurrency(string traderName, string virtualStoreCurrency, out string currencyId, out AskalCurrencyConfig currencyCfg)
	{
		currencyId = "";
		currencyCfg = NULL;
		
		AskalMarketConfig config = GetInstance();
		if (!config)
		{
			Print("[AskalMarket] ❌ MarketConfig not loaded");
			return false;
		}
		
		// Priority 1: Check virtual store AcceptedCurrency (if provided)
		// Virtual Store is independent of physical traders - it's a global system
		// When virtualStoreCurrency is provided, it indicates Virtual Store transaction
		if (virtualStoreCurrency && virtualStoreCurrency != "")
		{
			currencyId = virtualStoreCurrency;
			Print("[AskalMarket] 💰 Resolved currency for virtual store: " + currencyId);
			
			// Validate Virtual Store currency exists and is enabled
			currencyCfg = config.GetCurrencyConfig(currencyId);
			if (!currencyCfg)
			{
				Print("[AskalMarket] ❌ Virtual Store currency not found in MarketConfig: " + currencyId);
				Print("[AskalMarket] 💰 Falling back to MarketConfig DefaultCurrencyId");
				// Virtual Store fallback: use MarketConfig DefaultCurrencyId (NOT trader currency)
				currencyId = config.GetDefaultCurrencyId();
				currencyCfg = config.GetCurrencyConfig(currencyId);
				if (!currencyCfg)
				{
					Print("[AskalMarket] ❌ DefaultCurrencyId also not found: " + currencyId);
					return false;
				}
			}
			
			if (currencyCfg.Mode == 0)
			{
				Print("[AskalMarket] ❌ Virtual Store currency is disabled (Mode=0): " + currencyId);
				Print("[AskalMarket] 💰 Falling back to MarketConfig DefaultCurrencyId");
				// Virtual Store fallback: use MarketConfig DefaultCurrencyId (NOT trader currency)
				currencyId = config.GetDefaultCurrencyId();
				currencyCfg = config.GetCurrencyConfig(currencyId);
				if (!currencyCfg || currencyCfg.Mode == 0)
				{
					Print("[AskalMarket] ❌ DefaultCurrencyId also invalid: " + currencyId);
					return false;
				}
			}
			
			return true;
		}
		
		// Priority 2: Check trader AcceptedCurrency (only if virtual store not provided)
		// Physical traders are location-based NPCs
		if (traderName && traderName != "" && traderName != "Trader_Default")
		{
			AskalTraderConfig traderConfig = AskalTraderConfig.LoadByTraderName(traderName);
			if (traderConfig && traderConfig.AcceptedCurrency != "")
			{
				currencyId = traderConfig.AcceptedCurrency;
				Print("[AskalMarket] 💰 Resolved currency for trader " + traderName + ": " + currencyId);
			}
		}
		
		// Priority 3: Fallback to DefaultCurrencyId (for traders or when nothing specified)
		if (!currencyId || currencyId == "")
		{
			currencyId = config.GetDefaultCurrencyId();
			Print("[AskalMarket] 💰 Using default currency: " + currencyId);
		}
		
		// Validate currency exists and is enabled
		if (currencyId && currencyId != "")
		{
			currencyCfg = config.GetCurrencyConfig(currencyId);
			if (!currencyCfg)
			{
				Print("[AskalMarket] ❌ Currency not found in MarketConfig: " + currencyId);
				return false;
			}
			
			if (currencyCfg.Mode == 0)
			{
				Print("[AskalMarket] ❌ Currency is disabled (Mode=0): " + currencyId);
				return false;
			}
			
			return true;
		}
		
		return false;
	}
	
	// Aplicar configuração recebida do servidor via RPC
	static void ApplyConfigFromServer(string defaultCurrencyId, array<string> currencyIds, array<int> currencyModes, array<string> currencyShortNames, array<int> currencyStartCurrencies)
	{
		AskalMarketConfig config = GetInstance();
		if (!config)
		{
			Print("[AskalMarket] ❌ MarketConfig instance não encontrada ao aplicar dados do servidor");
			return;
		}
		
		// Limpar currencies existentes
		config.Currencies.Clear();
		
		// Aplicar DefaultCurrencyId
		if (defaultCurrencyId && defaultCurrencyId != "")
			config.DefaultCurrencyId = defaultCurrencyId;
		else
			config.DefaultCurrencyId = "Askal_Money";
		
		// Aplicar currencies
		if (currencyIds && currencyModes && currencyShortNames && currencyStartCurrencies)
		{
			int count = currencyIds.Count();
			if (count == currencyModes.Count() && count == currencyShortNames.Count() && count == currencyStartCurrencies.Count())
			{
				for (int i = 0; i < count; i++)
				{
					string currencyId = currencyIds.Get(i);
					if (!currencyId || currencyId == "")
						continue;
					
					AskalCurrencyConfig currencyCfg = new AskalCurrencyConfig();
					currencyCfg.Mode = currencyModes.Get(i);
					currencyCfg.ShortName = currencyShortNames.Get(i);
					currencyCfg.StartCurrency = currencyStartCurrencies.Get(i);
					currencyCfg.WalletId = currencyId;
					currencyCfg.Values = new array<ref AskalCurrencyValueConfig>(); // Empty for virtual currencies
					
					config.Currencies.Insert(currencyId, currencyCfg);
				}
			}
		}
		
		Print("[AskalMarket] ✅ MarketConfig aplicado do servidor: " + config.Currencies.Count() + " currencies, DefaultCurrencyId: " + config.DefaultCurrencyId);
	}
}
