// ==========================================
// AskalJsonLoader - Carregador JSON customizado
// Inspirado em padrões open-source do DayZ Editor
// Permite carregar arquivos JSON maiores que 64KB
// ==========================================

class AskalJsonLoader<Class T>
{
    protected static ref JsonSerializer m_Serializer;
    
    static JsonSerializer GetSerializer()
    {
        if (!m_Serializer)
        {
            m_Serializer = new JsonSerializer();
        }
        return m_Serializer;
    }
    
    // Converte string JSON para objeto
    static bool StringToObject(string stringData, out T data)
    {
        if (!stringData || stringData == "")
        {
            Print("[AskalJsonLoader] ⚠️ String vazia fornecida");
            return false;
        }
        
        string error;
        JsonSerializer serializer = GetSerializer();
        
        if (!serializer.ReadFromString(data, stringData, error))
        {
            Print("[AskalJsonLoader] ❌ Erro ao ler JSON: " + error);
            return false;
        }
        
        return true;
    }
    
    // Converte objeto para string JSON
    static string ObjectToString(T data)
    {
        if (!data)
        {
            Print("[AskalJsonLoader] ⚠️ Objeto NULL fornecido");
            return string.Empty;
        }
        
        JsonSerializer serializer = GetSerializer();
        string stringData;
        
        if (!serializer.WriteToString(data, true, stringData))
        {
            Print("[AskalJsonLoader] ❌ Erro ao serializar objeto");
            return string.Empty;
        }
        
        return stringData;
    }
    
    // Generate timestamp string for backup files (format: YYYYMMDDTHHMMSS)
    protected static string GenerateTimestamp()
    {
        int year, month, day, hour, minute, second;
        GetYearMonthDay(year, month, day);
        GetHourMinuteSecond(hour, minute, second);
        
        string timestamp = year.ToString();
        if (month < 10) timestamp += "0";
        timestamp += month.ToString();
        if (day < 10) timestamp += "0";
        timestamp += day.ToString();
        timestamp += "T";
        if (hour < 10) timestamp += "0";
        timestamp += hour.ToString();
        if (minute < 10) timestamp += "0";
        timestamp += minute.ToString();
        if (second < 10) timestamp += "0";
        timestamp += second.ToString();
        
        return timestamp;
    }
    
    // Generate UTC ISO timestamp (format: YYYY-MM-DDTHH:MM:SSZ)
    static string GenerateUtcIsoTimestamp()
    {
        int year, month, day, hour, minute, second;
        GetYearMonthDay(year, month, day);
        GetHourMinuteSecond(hour, minute, second);
        
        string timestamp = year.ToString() + "-";
        if (month < 10) timestamp += "0";
        timestamp += month.ToString() + "-";
        if (day < 10) timestamp += "0";
        timestamp += day.ToString() + "T";
        if (hour < 10) timestamp += "0";
        timestamp += hour.ToString() + ":";
        if (minute < 10) timestamp += "0";
        timestamp += minute.ToString() + ":";
        if (second < 10) timestamp += "0";
        timestamp += second.ToString() + "Z";
        
        return timestamp;
    }
    
    // Create backup of file before modification
    protected static bool CreateBackup(string filePath, string originalContent)
    {
        if (!filePath || filePath == "" || !originalContent || originalContent == "")
            return false;
        
        string timestamp = GenerateUtcIsoTimestamp();
        string backupPath = filePath + ".bak." + timestamp;
        
        FileHandle fh = OpenFile(backupPath, FileMode.WRITE);
        if (!fh)
        {
            Print("[AskalJsonLoader] ❌ Não foi possível criar backup: " + backupPath);
            return false;
        }
        
        FPrintln(fh, originalContent);
        CloseFile(fh);
        Print("[AskalJsonLoader] Backup written: " + backupPath);
        return true;
    }
    
    // Create backup of corrupt file (raw data)
    static bool CreateCorruptBackup(string filePath, string rawContent)
    {
        if (!filePath || filePath == "" || !rawContent || rawContent == "")
            return false;
        
        string timestamp = GenerateUtcIsoTimestamp();
        string corruptPath = filePath + ".corrupt." + timestamp;
        
        FileHandle fh = OpenFile(corruptPath, FileMode.WRITE);
        if (!fh)
        {
            Print("[AskalJsonLoader] ❌ Não foi possível criar backup corrupto: " + corruptPath);
            return false;
        }
        
        FPrintln(fh, rawContent);
        CloseFile(fh);
        Print("[AskalJsonLoader] Corrupt player file backup written: " + corruptPath);
        return true;
    }
    
    // Salva objeto em arquivo JSON (atomic write: tmp file then rename)
    static bool SaveToFile(string path, T data)
    {
        if (!path || path == "" || !data)
        {
            Print("[AskalJsonLoader] ⚠️ Parâmetros inválidos para SaveToFile");
            return false;
        }
        
        JsonSerializer serializer = GetSerializer();
        string jsonData;
        bool success = serializer.WriteToString(data, true, jsonData);
        
        if (!success || jsonData == string.Empty)
        {
            Print("[AskalJsonLoader] ❌ Erro ao serializar dados para: " + path);
            return false;
        }
        
        // Atomic write: write to tmp file first, then copy to final path
        // Since RenameFile doesn't exist in Enforce, we write to tmp, then copy to final
        string tmpPath = path + ".tmp";
        FileHandle fh = OpenFile(tmpPath, FileMode.WRITE);
        if (!fh)
        {
            Print("[AskalJsonLoader] ❌ Não foi possível criar arquivo temporário: " + tmpPath);
            return false;
        }
        
        FPrintln(fh, jsonData);
        CloseFile(fh);
        
        // Copy tmp file to final path (read tmp, write to final)
        FileHandle tmpRead = OpenFile(tmpPath, FileMode.READ);
        if (!tmpRead)
        {
            Print("[AskalJsonLoader] ❌ Não foi possível ler arquivo temporário: " + tmpPath);
            DeleteFile(tmpPath);
            return false;
        }
        
        FileHandle finalWrite = OpenFile(path, FileMode.WRITE);
        if (!finalWrite)
        {
            Print("[AskalJsonLoader] ❌ Não foi possível escrever arquivo final: " + path);
            CloseFile(tmpRead);
            DeleteFile(tmpPath);
            return false;
        }
        
        // Copy content from tmp to final
        string line;
        while (FGets(tmpRead, line) > 0)
        {
            FPrintln(finalWrite, line);
        }
        
        CloseFile(tmpRead);
        CloseFile(finalWrite);
        DeleteFile(tmpPath);
        
        Print("[AskalJsonLoader] ✅ Arquivo salvo: " + path);
        return true;
    }
    
    // Salva objeto em arquivo JSON com backup (para migrações)
    static bool SaveToFileWithBackup(string path, T data, string originalContent)
    {
        if (!path || path == "" || !data)
        {
            Print("[AskalJsonLoader] ⚠️ Parâmetros inválidos para SaveToFileWithBackup");
            return false;
        }
        
        // Create backup first
        if (originalContent && originalContent != "")
        {
            if (!CreateBackup(path, originalContent))
            {
                Print("[AskalJsonLoader] ❌ Falha ao criar backup, abortando migração: " + path);
                return false;
            }
        }
        
        // Save using atomic write
        return SaveToFile(path, data);
    }
    
    // Carrega objeto de arquivo JSON (lê linha por linha para arquivos grandes)
    static bool LoadFromFile(string path, out T data, bool logSuccess = true)
    {
        if (!path || path == "")
        {
            Print("[AskalJsonLoader] ⚠️ Caminho inválido");
            return false;
        }
        
        if (!FileExist(path))
        {
            Print("[AskalJsonLoader] ⚠️ Arquivo não encontrado: " + path);
            return false;
        }
        
        FileHandle fh = OpenFile(path, FileMode.READ);
        if (!fh)
        {
            Print("[AskalJsonLoader] ❌ Não foi possível abrir arquivo: " + path);
            return false;
        }
        
        // Ler arquivo linha por linha (suporta arquivos grandes)
        string jsonData = "";
        string line;
        while (FGets(fh, line) > 0)
        {
            jsonData = jsonData + "\n" + line;
        }
        CloseFile(fh);
        
        if (jsonData == "")
        {
            Print("[AskalJsonLoader] ⚠️ Arquivo vazio: " + path);
            return false;
        }
        
        // Parse JSON
        string error;
        JsonSerializer serializer = GetSerializer();
        bool success = serializer.ReadFromString(data, jsonData, error);
        
        if (!success || error != string.Empty)
        {
            Print("[AskalJsonLoader] ❌ Erro ao fazer parse de: " + path);
            Print("[AskalJsonLoader] Erro: " + error);
            return false;
        }
        
        if (logSuccess)
        {
            Print("[AskalJsonLoader] Player file loaded: " + path);
        }
        
        return true;
    }
    
    // Carrega objeto de arquivo JSON e retorna conteúdo original (para backup)
    static bool LoadFromFileWithOriginal(string path, out T data, out string originalContent, bool logSuccess = true)
    {
        originalContent = "";
        
        if (!path || path == "")
        {
            Print("[AskalJsonLoader] ⚠️ Caminho inválido");
            return false;
        }
        
        if (!FileExist(path))
        {
            Print("[AskalJsonLoader] ⚠️ Arquivo não encontrado: " + path);
            return false;
        }
        
        FileHandle fh = OpenFile(path, FileMode.READ);
        if (!fh)
        {
            Print("[AskalJsonLoader] ❌ Não foi possível abrir arquivo: " + path);
            return false;
        }
        
        // Ler arquivo linha por linha (suporta arquivos grandes)
        string jsonData = "";
        string line;
        while (FGets(fh, line) > 0)
        {
            jsonData = jsonData + "\n" + line;
        }
        CloseFile(fh);
        
        if (jsonData == "")
        {
            Print("[AskalJsonLoader] ⚠️ Arquivo vazio: " + path);
            return false;
        }
        
        // Store original content for backup
        originalContent = jsonData;
        
        // Parse JSON
        string error;
        JsonSerializer serializer = GetSerializer();
        bool success = serializer.ReadFromString(data, jsonData, error);
        
        if (!success || error != string.Empty)
        {
            Print("[AskalJsonLoader] ❌ Erro ao fazer parse de: " + path);
            Print("[AskalJsonLoader] Erro: " + error);
            return false;
        }
        
        if (logSuccess)
        {
            Print("[AskalJsonLoader] Player file loaded: " + path);
        }
        
        return true;
    }
}

// Specialized loader for AskalPlayerData with synchronous load/create/migrate
class AskalPlayerConfigLoader
{
    private static ref map<string, ref AskalPlayerData> s_LoadResults = new map<string, ref AskalPlayerData>();
    private static ref map<string, bool> s_LoadSuccess = new map<string, bool>();
    private static ref map<string, string> s_LoadReasons = new map<string, string>();
    private static ref map<string, bool> s_LoadLocks = new map<string, bool>();
    
    // Try to acquire load lock (returns true if acquired, false if already locked)
    private static bool TryAcquireLoadLock(string steamId)
    {
        if (!steamId || steamId == "")
            return false;
        
        if (s_LoadLocks.Contains(steamId) && s_LoadLocks.Get(steamId))
            return false; // Already locked
        
        s_LoadLocks.Set(steamId, true);
        return true;
    }
    
    // Release load lock
    private static void ReleaseLoadLock(string steamId)
    {
        if (!steamId || steamId == "")
            return;
        
        s_LoadLocks.Set(steamId, false);
    }
    
    // Fallback: Load MarketConfig.json directly from disk if in-memory is empty
    // Returns: (success, currencies map, currencyList string)
    private static bool LoadMarketConfigFallback(out map<string, ref AskalCurrencyConfig> currencies, out string currencyList)
    {
        currencies = new map<string, ref AskalCurrencyConfig>();
        currencyList = "";
        
        // Try same paths as AskalMarketConfig
        array<string> candidatePaths = {
            "$profile:config/Askal/Market/MarketConfig.json",
            "$profile:config\\Askal\\Market\\MarketConfig.json",
            "$profile:Askal/Market/MarketConfig.json",
            "$profile:Askal\\Market\\MarketConfig.json",
            "$mission:Askal/Market/MarketConfig.json",
            "$mission:Askal\\Market\\MarketConfig.json",
            "Askal/Market/MarketConfig.json",
            "Askal\\Market\\MarketConfig.json"
        };
        
        foreach (string path : candidatePaths)
        {
            if (!FileExist(path))
                continue;
            
            // Parse JSON using JsonFileLoader (same method as AskalMarketConfig)
            AskalMarketConfigFile fileData = new AskalMarketConfigFile();
            JsonFileLoader<AskalMarketConfigFile>.JsonLoadFile(path, fileData);
            
            if (!fileData)
            {
                Print("[AskalJsonLoader] MarketConfig fallback parse error: failed to load file");
                continue;
            }
            
            // Extract currencies from fileData
            if (fileData.Currencies)
            {
                for (int fallbackIdx = 0; fallbackIdx < fileData.Currencies.Count(); fallbackIdx++)
                {
                    string currencyId = fileData.Currencies.GetKey(fallbackIdx);
                    AskalCurrencyConfig currencyConfig = fileData.Currencies.GetElement(fallbackIdx);
                    if (currencyConfig)
                    {
                        currencies.Set(currencyId, currencyConfig);
                        if (currencyList != "")
                            currencyList += ", ";
                        currencyList += currencyId;
                    }
                }
            }
            
            // Also check VirtualCurrencies if present in fileData (future support)
            // Note: VirtualCurrencies would need to be added to AskalMarketConfigFile if it exists
            
            if (currencies.Count() > 0)
            {
                Print("[AskalJsonLoader] MarketConfig fallback loaded from disk: " + path + " — currencies: " + currencyList);
                
                // Populate in-memory MarketConfig if possible (to avoid future fallbacks)
                AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
                if (marketConfig)
                {
                    // Only populate if in-memory is empty
                    if (!marketConfig.Currencies || marketConfig.Currencies.Count() == 0)
                    {
                        if (!marketConfig.Currencies)
                            marketConfig.Currencies = new map<string, ref AskalCurrencyConfig>();
                        else
                            marketConfig.Currencies.Clear();
                        
                        // Copy currency references from fileData
                        for (int copyIdx = 0; copyIdx < currencies.Count(); copyIdx++)
                        {
                            string copyCurrencyId = currencies.GetKey(copyIdx);
                            AskalCurrencyConfig copyCurrencyConfig = currencies.GetElement(copyIdx);
                            if (copyCurrencyConfig)
                                marketConfig.Currencies.Set(copyCurrencyId, copyCurrencyConfig);
                        }
                        
                        if (fileData.DefaultCurrencyId != "")
                            marketConfig.DefaultCurrencyId = fileData.DefaultCurrencyId;
                    }
                }
                
                return true;
            }
        }
        
        return false;
    }
    
    // Get all currencies from MarketConfig (Currencies + VirtualCurrencies if present)
    // Returns a map of currencyId -> AskalCurrencyConfig
    private static map<string, ref AskalCurrencyConfig> GetAllCurrencies(AskalMarketConfig marketConfig)
    {
        map<string, ref AskalCurrencyConfig> allCurrencies = new map<string, ref AskalCurrencyConfig>();
        
        if (!marketConfig)
            return allCurrencies;
        
        // Add Currencies
        if (marketConfig.Currencies)
        {
            for (int i = 0; i < marketConfig.Currencies.Count(); i++)
            {
                string currencyId = marketConfig.Currencies.GetKey(i);
                AskalCurrencyConfig currencyConfig = marketConfig.Currencies.GetElement(i);
                if (currencyConfig)
                    allCurrencies.Set(currencyId, currencyConfig);
            }
        }
        
        // Add VirtualCurrencies if present (union - don't overwrite existing)
        // Note: VirtualCurrencies would need to be added to AskalMarketConfig class if it exists
        // For now, we only use Currencies, but this structure supports future VirtualCurrencies
        
        return allCurrencies;
    }
    
    // Synchronous load or create player config
    // Returns: (playerData, success, reason)
    // Thread-safe: uses locks to prevent concurrent loads
    static void LoadOrCreatePlayerConfigSync(string steamId, out AskalPlayerData playerData, out bool success, out string reason)
    {
        playerData = NULL;
        success = false;
        reason = "";
        
        if (!steamId || steamId == "")
        {
            reason = "empty steamId";
            return;
        }
        
        // Check if load is in progress (simple lock mechanism)
        if (!TryAcquireLoadLock(steamId))
        {
            // Another load is in progress, use cached result if available
            if (s_LoadResults.Contains(steamId))
            {
                playerData = s_LoadResults.Get(steamId);
                if (s_LoadSuccess.Contains(steamId))
                    success = s_LoadSuccess.Get(steamId);
                if (s_LoadReasons.Contains(steamId))
                    reason = s_LoadReasons.Get(steamId);
                return;
            }
            // If no cached result, proceed anyway (will handle race condition)
        }
        
        Print("[AskalJsonLoader] Connect handler called for player: " + steamId);
        
        // Ensure MarketConfig is loaded first (with fallback to disk read)
        AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
        map<string, ref AskalCurrencyConfig> allCurrencies = new map<string, ref AskalCurrencyConfig>();
        
        // Try in-memory first
        if (marketConfig && marketConfig.Currencies && marketConfig.Currencies.Count() > 0)
        {
            allCurrencies = GetAllCurrencies(marketConfig);
        }
        
        // Fallback: read from disk if in-memory is empty
        if (allCurrencies.Count() == 0)
        {
            string fallbackCurrencyList = "";
            bool fallbackSuccess = LoadMarketConfigFallback(allCurrencies, fallbackCurrencyList);
            
            if (!fallbackSuccess || allCurrencies.Count() == 0)
            {
                // Both in-memory and disk read failed
                reason = "No currencies defined in MarketConfig";
                Print("[AskalJsonLoader] ERROR: No currencies defined in MarketConfig");
                ReleaseLoadLock(steamId);
                return;
            }
        }
        
        string filePath = "$profile:Askal/Database/Players/" + steamId + ".json";
        Print("[AskalJsonLoader] Loading player file: " + filePath);
        
        // Ensure directories exist
        string playersDir = "$profile:Askal/Database/Players";
        if (!FileExist(playersDir))
        {
            MakeDirectory(playersDir);
        }
        
        bool fileExists = FileExist(filePath);
        string originalContent = "";
        
        if (fileExists)
        {
            // Try to load file
            FileHandle fh = OpenFile(filePath, FileMode.READ);
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
                    
                    // Try to parse JSON
                    string error;
                    JsonSerializer serializer = new JsonSerializer();
                    playerData = new AskalPlayerData();
                    bool parseSuccess = serializer.ReadFromString(playerData, rawContent, error);
                    
                    if (!parseSuccess || error != string.Empty)
                    {
                        Print("[AskalJsonLoader] ERROR: Failed to parse player JSON: " + error);
                        
                        // Create corrupt backup
                        string corruptPath = filePath + ".corrupt." + AskalJsonLoader<AskalPlayerData>.GenerateUtcIsoTimestamp();
                        FileHandle corruptFh = OpenFile(corruptPath, FileMode.WRITE);
                        if (corruptFh)
                        {
                            FPrintln(corruptFh, rawContent);
                            CloseFile(corruptFh);
                            Print("[AskalJsonLoader] Corrupt player file backup written: " + corruptPath);
                        }
                        
                        // Reset for new file creation
                        playerData = NULL;
                        originalContent = "";
                    }
                    else
                    {
                        // Ensure Balance is object
                        if (!playerData.Balance)
                            playerData.Balance = new map<string, int>();
                    }
                }
            }
        }
        
        // If file doesn't exist or parse failed, create default
        if (!fileExists || !playerData)
        {
            playerData = new AskalPlayerData();
            playerData.FirstLogin = "";
            playerData.Balance = new map<string, int>();
            playerData.CreditCard = 0;
            playerData.PremiumStatusExpireInHours = 0;
            playerData.Permissions = new map<string, int>();
            
            // Set FirstLogin timestamp (UTC ISO8601)
            playerData.FirstLogin = AskalJsonLoader<AskalPlayerData>.GenerateUtcIsoTimestamp();
            
            // Seed all currencies (from union of Currencies + VirtualCurrencies)
            string currencyList = "";
            string seedSummary = "";
            for (int seedIdx = 0; seedIdx < allCurrencies.Count(); seedIdx++)
            {
                string seedCurrencyId = allCurrencies.GetKey(seedIdx);
                AskalCurrencyConfig seedCurrencyConfig = allCurrencies.GetElement(seedIdx);
                if (!seedCurrencyConfig)
                    continue;
                
                // Determine seed value based on Mode
                int seedValue = 0;
                int seedCurrencyMode = seedCurrencyConfig.Mode;
                // If Mode is missing (defaults to 1 in constructor), treat as Mode == 1
                if (seedCurrencyMode == AskalMarketConstants.CURRENCY_MODE_DISABLED)
                {
                    seedValue = 0;
                }
                else
                {
                    // Mode == 1 or Mode == 2: use StartCurrency if present, else 0
                    seedValue = seedCurrencyConfig.StartCurrency;
                    if (seedValue < 0)
                        seedValue = 0;
                }
                
                playerData.Balance.Set(seedCurrencyId, seedValue);
                
                if (currencyList != "")
                    currencyList += ", ";
                currencyList += seedCurrencyId;
                
                if (seedSummary != "")
                    seedSummary += ",";
                seedSummary += seedCurrencyId + "=" + seedValue;
            }
            
            Print("[AskalJsonLoader] Created new player JSON for " + steamId + " with currencies: " + currencyList);
            Print("[AskalJsonLoader] Player " + steamId + " Balance seeded: " + seedSummary);
            
            // Save atomically (write to .tmp then rename)
            if (!AskalJsonLoader<AskalPlayerData>.SaveToFile(filePath, playerData))
            {
                reason = "disk write error: failed to create new file";
                Print("[AskalJsonLoader] Failed to create/load player " + steamId + " config — reason: " + reason);
                playerData = NULL;
                ReleaseLoadLock(steamId);
                return;
            }
            
            if (!FileExist(filePath))
            {
                reason = "disk write error: file not created after write";
                Print("[AskalJsonLoader] Failed to create/load player " + steamId + " config — reason: " + reason);
                playerData = NULL;
                ReleaseLoadLock(steamId);
                return;
            }
            
            Print("[AskalJsonLoader] Player file saved (migrated/created): " + filePath);
            
            success = true;
            reason = "ok";
            
            // Cache result and release lock
            s_LoadResults.Set(steamId, playerData);
            s_LoadSuccess.Set(steamId, success);
            s_LoadReasons.Set(steamId, reason);
            ReleaseLoadLock(steamId);
            return;
        }
        
        // File exists and parsed successfully, migrate if needed
        bool needsMigration = false;
        
        // Check for missing currencies - NEVER overwrite existing balances
        // Use union of Currencies + VirtualCurrencies
        for (int migrateIdx = 0; migrateIdx < allCurrencies.Count(); migrateIdx++)
        {
            string migrateCurrencyId = allCurrencies.GetKey(migrateIdx);
            AskalCurrencyConfig migrateCurrencyConfig = allCurrencies.GetElement(migrateIdx);
            if (!migrateCurrencyConfig)
                continue;
            
            // Only add if currency is missing - never overwrite existing values
            if (!playerData.Balance.Contains(migrateCurrencyId))
            {
                needsMigration = true;
                int migrateSeedValue = 0;
                
                // Determine seed value based on Mode
                int migrateCurrencyMode = migrateCurrencyConfig.Mode;
                // If Mode is missing (defaults to 1 in constructor), treat as Mode == 1
                if (migrateCurrencyMode == AskalMarketConstants.CURRENCY_MODE_DISABLED)
                {
                    migrateSeedValue = 0;
                }
                else
                {
                    // Mode == 1 or Mode == 2: use StartCurrency if present, else 0
                    migrateSeedValue = migrateCurrencyConfig.StartCurrency;
                    if (migrateSeedValue < 0)
                        migrateSeedValue = 0;
                }
                
                // Only insert missing currency - never overwrite
                playerData.Balance.Set(migrateCurrencyId, migrateSeedValue);
                Print("[AskalJsonLoader] MIGRATE: Added missing currency \"" + migrateCurrencyId + "\"=" + migrateSeedValue + " to player " + steamId);
            }
        }
        
        // If migration needed, save with backup
        if (needsMigration)
        {
            // Create backup before modifying (mandatory before overwriting valid file)
            if (originalContent != "")
            {
                string backupPath = filePath + ".bak." + AskalJsonLoader<AskalPlayerData>.GenerateUtcIsoTimestamp();
                FileHandle backupFh = OpenFile(backupPath, FileMode.WRITE);
                if (backupFh)
                {
                    FPrintln(backupFh, originalContent);
                    CloseFile(backupFh);
                    Print("[AskalJsonLoader] Backup written: " + backupPath);
                }
                else
                {
                    reason = "disk write error: failed to create backup";
                    Print("[AskalJsonLoader] Failed to create/load player " + steamId + " config — reason: " + reason);
                    playerData = NULL;
                    ReleaseLoadLock(steamId);
                    return;
                }
            }
            
            // Save migrated data atomically (write to .tmp then rename)
            if (!AskalJsonLoader<AskalPlayerData>.SaveToFile(filePath, playerData))
            {
                reason = "disk write error: failed to save migrated file";
                Print("[AskalJsonLoader] Failed to create/load player " + steamId + " config — reason: " + reason);
                playerData = NULL;
                ReleaseLoadLock(steamId);
                return;
            }
            
            Print("[AskalJsonLoader] Player file saved (migrated/created): " + filePath);
        }
        
        success = true;
        reason = "ok";
        
        // Cache result and release lock
        s_LoadResults.Set(steamId, playerData);
        s_LoadSuccess.Set(steamId, success);
        s_LoadReasons.Set(steamId, reason);
        ReleaseLoadLock(steamId);
    }
    
    // Public function to call on player connect
    // Should be called from OnStoreLoad or similar player initialization hook
    // Note: This function should be called from 4_World context where AskalMarketHelpers is available
    static void OnPlayerConnect(string steamId)
    {
        if (!steamId || steamId == "")
            return;
        
        AskalPlayerData playerData;
        bool success;
        string reason;
        
        LoadOrCreatePlayerConfigSync(steamId, playerData, success, reason);
        
        if (success && playerData)
        {
            // Initialize in-memory balances
            AskalPlayerBalance.InitializePlayerBalances(steamId, playerData);
            // Note: SetPlayerConfigValid should be called from 4_World context
        }
    }
}

