class AskalMarketLoader
{
    protected static ref AskalMarketLoader s_Instance;

    static AskalMarketLoader GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new AskalMarketLoader();
        }
        return s_Instance;
    }

    // Carrega MarketConfig usando JsonFileLoader
    static bool LoadConfig(string path, out AskalMarketConfig config)
    {
		if (!config)
			config = new AskalMarketConfig();
		
		if (!path || path == "")
			return false;
		
		return config.LoadFromPath(path);
    }

    // Carrega TraderConfig usando AskalTraderConfig.Load (Core)
    static bool LoadTrader(string path, out AskalTraderConfig trader, AskalSetupResolver resolver = null)
    {
        if (!FileExist(path)) return false;

        // Extrair nome do arquivo do caminho
        string fileName = path;
        // Encontrar último "/" ou "\"
        int lastSlash = -1;
        for (int i = fileName.Length() - 1; i >= 0; i--)
        {
            string char = fileName.Substring(i, 1);
            if (char == "/" || char == "\\")
            {
                lastSlash = i;
                break;
            }
        }
        if (lastSlash != -1)
            fileName = fileName.Substring(lastSlash + 1, fileName.Length() - lastSlash - 1);
        
        // Remover prefixo "Trader_" se presente
        if (fileName.IndexOf("Trader_") == 0)
        {
            fileName = fileName.Substring(7, fileName.Length() - 7); // Remove "Trader_"
        }
        
        // Remover extensão
        if (fileName.IndexOf(".jsonc") != -1)
            fileName = fileName.Substring(0, fileName.IndexOf(".jsonc"));
        if (fileName.IndexOf(".json") != -1)
            fileName = fileName.Substring(0, fileName.IndexOf(".json"));
        
        // Usar método Load do Core (precisa do nome completo com prefixo "Trader_")
        string fullFileName = "Trader_" + fileName;
        trader = AskalTraderConfig.Load(fullFileName);
        
        // Registrar setup inline no resolver, se disponível
        if (resolver && trader && trader.SetupItems)
        {
            // Usar TraderName como ID e AcceptedCurrencyMap como CurrencyMode
            map<string, int> currencyMode = trader.AcceptedCurrencyMap;
            if (resolver && currencyMode)
            {
                // Verificar se o método existe antes de chamar
                // RegisterInlineSetup pode não existir na versão atual
                // Por enquanto, apenas log
                Print("[AskalMarketLoader] Trader carregado: " + trader.TraderName);
            }
        }
        return trader != NULL;
    }
    
    // Carrega SetupConfig usando JsonFileLoader
    static bool LoadSetup(string path, out AskalSetupConfig setup)
    {
        if (!FileExist(path)) return false;

        if (!setup) setup = new AskalSetupConfig();
        JsonFileLoader<AskalSetupConfig>.JsonLoadFile(path, setup);
        return true;
    }
    
    // Carrega VirtualStoreConfig usando JsonFileLoader (legacy - ainda usado)
    static bool LoadVirtualStoreConfig(string path, out AskalVirtualStoreConfig config)
    {
        if (!FileExist(path)) return false;

        if (!config) config = new AskalVirtualStoreConfig();
        // JsonFileLoader is a forward-declared type - use AskalJsonLoader instead
        AskalJsonLoader<AskalVirtualStoreConfig>.LoadFromFile(path, config, false);
        if (config)
        {
            config.NormalizeAcceptedCurrency();
            config.EnsureDefaults();
        }
        return true;
    }

    // Utilitário: carrega todos os traders de um diretório
    static void LoadTradersFromDirectory(string basePath, array<ref AskalTraderConfig> output, AskalSetupResolver resolver = null)
    {
        if (!basePath || basePath == "")
            return;

        string searchPath = basePath;
        if (searchPath.Length() > 0)
        {
            string lastChar = searchPath.Substring(searchPath.Length() - 1, 1);
            if (lastChar != "/" && lastChar != "\\")
                searchPath += "/";
        }
        else
        {
            searchPath = "/";
        }

        string fileName;
        FileAttr fileAttr;
        FindFileHandle handle = FindFile(searchPath + "Trader_*.json", fileName, fileAttr, 0);
        if (!handle)
        {
            // Tentar também .jsonc
            handle = FindFile(searchPath + "Trader_*.jsonc", fileName, fileAttr, 0);
            if (!handle)
                return;
        }

        bool keep = true;
        while (keep)
        {
            if (fileAttr != FileAttr.DIRECTORY && fileName != "")
            {
                string traderPath = searchPath + fileName;
                AskalTraderConfig trader;
                if (LoadTrader(traderPath, trader, resolver) && trader)
                {
                    if (output)
                        output.Insert(trader);
                }
            }
            keep = FindNextFile(handle, fileName, fileAttr);
        }
        CloseFindFile(handle);
    }
}
