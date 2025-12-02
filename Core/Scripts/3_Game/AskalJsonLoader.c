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
    
    // Salva objeto em arquivo JSON
    static bool SaveToFile(string path, T data)
    {
        if (!path || path == "" || !data)
        {
            Print("[AskalJsonLoader] ⚠️ Parâmetros inválidos para SaveToFile");
            return false;
        }
        
        FileHandle fh = OpenFile(path, FileMode.WRITE);
        if (!fh)
        {
            Print("[AskalJsonLoader] ❌ Não foi possível criar arquivo: " + path);
            return false;
        }
        
        JsonSerializer serializer = GetSerializer();
        string jsonData;
        bool success = serializer.WriteToString(data, true, jsonData);
        
        if (success && jsonData != string.Empty)
        {
            FPrintln(fh, jsonData);
        }
        else
        {
            Print("[AskalJsonLoader] ❌ Erro ao serializar dados para: " + path);
            CloseFile(fh);
            return false;
        }
        
        CloseFile(fh);
        Print("[AskalJsonLoader] ✅ Arquivo salvo: " + path);
        return true;
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
            Print("[AskalJsonLoader] ✅ Arquivo carregado: " + path);
        }
        
        return true;
    }
}

