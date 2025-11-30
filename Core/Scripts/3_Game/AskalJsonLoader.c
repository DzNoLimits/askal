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
    
    // Create backup of file before modification
    protected static bool CreateBackup(string filePath, string originalContent)
    {
        if (!filePath || filePath == "" || !originalContent || originalContent == "")
            return false;
        
        string timestamp = GenerateTimestamp();
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

