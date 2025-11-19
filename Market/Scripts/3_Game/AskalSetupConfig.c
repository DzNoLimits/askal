// AskalSetupConfig - Configuração de Setup (preset reutilizável para traders/virtual store)
// Suporta Dataset (DS_), Categoria (CAT_) e Itens individuais
// Prioridade: Item > Categoria > Dataset

class AskalSetupConfig
{
    string Version;
    ref map<string, int> CurrencyMode;  // Ex: {"Askal_Coin": 2}
    ref map<string, int> SetupItems;    // Ex: {"DS_Firearms": 3, "CAT_Pistols": 2, "AKM": 0}
    
    void AskalSetupConfig()
    {
        CurrencyMode = new map<string, int>();
        SetupItems = new map<string, int>();
    }

    void SetSetupItems(map<string, int> other)
    {
        SetupItems.Clear();
        if (!other)
            return;
        foreach (string key, int mode : other)
        {
            SetupItems.Set(key, mode);
        }
    }

    void SetCurrencyMode(map<string, int> other)
    {
        CurrencyMode.Clear();
        if (!other)
            return;
        foreach (string key, int mode : other)
        {
            CurrencyMode.Set(key, mode);
        }
    }
    
    // Helper: verifica se uma chave é Dataset (DS_), Categoria (CAT_) ou Item
    static int GetItemType(string key)
    {
        if (!key || key == "") return -1;
        
        // Dataset: começa com DS_
        if (key.Length() > 3 && key.Substring(0, 3) == "DS_")
            return 0; // Dataset
        
        // Categoria: começa com CAT_
        if (key.Length() > 4 && key.Substring(0, 4) == "CAT_")
            return 1; // Categoria
        
        // Item individual (classname)
        return 2; // Item
    }
    
    // Helper: obtém o modo configurado para um item específico
    // Retorna -1 se não encontrado
    // Prioridade: Item > Categoria > Dataset
    int GetItemMode(string itemClassname)
    {
        if (!itemClassname || itemClassname == "") return -1;
        
        // 1. Prioridade máxima: Item individual
        if (SetupItems.Contains(itemClassname))
            return SetupItems.Get(itemClassname);
        
        // 2. Busca pela categoria do item (precisa consultar Core API)
        // Nota: Isso será implementado no resolver que tem acesso ao Core
        
        // 3. Prioridade mínima: Dataset inteiro
        // Nota: Verificar dataset de cada item também precisa do Core
        
        // Por enquanto, retorna -1 se não encontrado
        // O resolver completo será implementado em AskalSetupResolver
        return -1;
    }
}

