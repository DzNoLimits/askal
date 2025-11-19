// AskalDatabaseLoader - roda no contexto da MissionServer (5_Mission) e faz I/O de forma segura
// Usa CF_Log se disponível, fallback para Print

class AskalDatabaseLoader
{
    // Helper para logging (usa CF_Log se disponível, fallback para Print)
    static void Log(string level, string message)
    {
        // Usa Print direto para garantir que sempre apareça
        // CF_Log pode não estar disponível neste momento da inicialização
        Print(message);
        
        // Também tenta usar CF_Log se disponível (para logs estruturados)
        // Mas não quebra se não estiver disponível
        #ifdef CF_TRACE_ENABLED
        switch(level)
        {
            case "Info":
                CF_Log.Info(message);
                break;
            case "Warning":
                CF_Log.Warn(message);
                break;
            case "Error":
                CF_Log.Error(message);
                break;
            default:
                CF_Log.Info(message);
        }
        #endif
    }
    // Extrai número do nome do arquivo para ordenação numérica
    // Suporta formatos: "DS_1_Food.json" -> 1, "DS_10_Base.json" -> 10, "1_Food.json" -> 1, "10_Base.json" -> 10
    static int ExtractDatasetNumber(string fileName)
    {
        if (!fileName || fileName == "") return 9999; // Arquivos sem número vão para o final
        
        int startPos = 0;
        int endPos = 0;
        
        // Primeiro tenta padrão DS_<número>_
        int dsPos = fileName.IndexOf("DS_");
        if (dsPos != -1)
        {
            // Formato: DS_<número>_
            startPos = dsPos + 3; // Após "DS_"
            endPos = startPos;
        }
        else
        {
            // Formato: <número>_ (começa diretamente com número)
            startPos = 0;
            endPos = 0;
        }
        
        // Encontra onde termina o número (até encontrar _ ou .)
        while (endPos < fileName.Length())
        {
            string currentChar = fileName.Substring(endPos, 1);
            if (currentChar == "_" || currentChar == ".")
                break;
            // Verificar se é um dígito (0-9)
            bool isDigit = false;
            if (currentChar == "0") isDigit = true;
            else if (currentChar == "1") isDigit = true;
            else if (currentChar == "2") isDigit = true;
            else if (currentChar == "3") isDigit = true;
            else if (currentChar == "4") isDigit = true;
            else if (currentChar == "5") isDigit = true;
            else if (currentChar == "6") isDigit = true;
            else if (currentChar == "7") isDigit = true;
            else if (currentChar == "8") isDigit = true;
            else if (currentChar == "9") isDigit = true;
            
            if (!isDigit)
                break;
            endPos++;
        }
        
        if (endPos <= startPos) return 9999;
        
        string numberStr = fileName.Substring(startPos, endPos - startPos);
        
        // Converter string para int manualmente (mais confiável que ToInt())
        int number = 0;
        for (int digitIdx = 0; digitIdx < numberStr.Length(); digitIdx++)
        {
            string digit = numberStr.Substring(digitIdx, 1);
            int digitValue = 0;
            if (digit == "0") digitValue = 0;
            else if (digit == "1") digitValue = 1;
            else if (digit == "2") digitValue = 2;
            else if (digit == "3") digitValue = 3;
            else if (digit == "4") digitValue = 4;
            else if (digit == "5") digitValue = 5;
            else if (digit == "6") digitValue = 6;
            else if (digit == "7") digitValue = 7;
            else if (digit == "8") digitValue = 8;
            else if (digit == "9") digitValue = 9;
            else break; // Não é um dígito válido
            
            number = number * 10 + digitValue;
        }
        
        return number;
    }
    
    // Ordena array de nomes de arquivos numericamente baseado no número do dataset
    static void SortDatasetFiles(array<string> files)
    {
        if (!files || files.Count() <= 1) return;
        
        // Bubble sort simples (arrays pequenos, performance não é crítica)
        for (int i = 0; i < files.Count() - 1; i++)
        {
            for (int j = 0; j < files.Count() - 1 - i; j++)
            {
                int num1 = ExtractDatasetNumber(files.Get(j));
                int num2 = ExtractDatasetNumber(files.Get(j + 1));
                
                if (num1 > num2)
                {
                    string temp = files.Get(j);
                    files.Set(j, files.Get(j + 1));
                    files.Set(j + 1, temp);
                }
            }
        }
    }
    
    // Carrega um arquivo JSON de dataset
    static void LoadDatasetFile(string fullPath, string fileName)
    {
        // Carrega formato hierárquico (JsonDataset)
        // Usa AskalJsonLoader para suportar arquivos grandes (>64KB)
        JsonDataset jsonDataset = new JsonDataset();
        if (!AskalJsonLoader<JsonDataset>.LoadFromFile(fullPath, jsonDataset, false))
        {
            Log("Warning", "[AskalDBLoader] ⚠️ Falha ao carregar: " + fileName);
            return;
        }
        
        // Verifica se tem Categories (formato hierárquico obrigatório)
        if (jsonDataset && jsonDataset.Categories && jsonDataset.Categories.Count() > 0)
        {
            // Formato hierárquico: Dataset com Categories
            Dataset dataset = Dataset.FromJson(jsonDataset);
            
            if (dataset)
            {
                // Registra o dataset
                AskalDatabase.RegisterDataset(dataset);
                
                // Conta total de itens em todas as categorias
                int totalItems = 0;
                if (dataset.Categories)
                {
                    for (int c = 0; c < dataset.Categories.Count(); c++)
                    {
                        AskalCategory cat = dataset.Categories.GetElement(c);
                        if (cat && cat.Items)
                            totalItems += cat.Items.Count();
                    }
                }
                
                Log("Info", "[AskalDBLoader] ✅ Loaded Dataset: " + dataset.DatasetID + " -> " + dataset.DisplayName + " v" + dataset.Version + " (" + dataset.Categories.Count() + " categories, " + totalItems + " items)");
            }
            else
            {
                Log("Warning", "[AskalDBLoader] ⚠️ Failed to convert JSON dataset: " + fileName);
            }
        }
        else
        {
            Log("Warning", "[AskalDBLoader] ⚠️ JSON file missing Categories (hierarchical format required): " + fileName);
        }
    }
    
    // Carrega recursivamente todos os arquivos .json das subpastas
    // ORDENA arquivos numericamente antes de carregar para preservar ordem correta
    static void LoadAllDatasetsRecursive(string directoryPath)
    {
        if (!directoryPath || directoryPath == "") return;
        
        // Garante que termina com /
        if (directoryPath[directoryPath.Length() - 1] != "/")
            directoryPath += "/";
        
        string fileName = "";
        FileAttr fileAttr = 0;
        string searchPattern = directoryPath + "*";
        
        FindFileHandle handle = FindFile(searchPattern, fileName, fileAttr, 0);
        if (!handle)
        {
            Log("Warning", "[AskalDBLoader] ⚠️ FindFile retornou NULL para: " + searchPattern);
            return;
        }
        
        // Coletar todos os arquivos .json primeiro
        array<string> jsonFiles = new array<string>();
        array<string> subdirectories = new array<string>();
        string fullPath;
        
        while (true)
        {
            if (fileName != "" && fileName != "." && fileName != ".." && fileName != "manifest.json")
            {
                fullPath = directoryPath + fileName;
                
                // Se termina com .json, é arquivo - adiciona à lista
                if (fileName.Length() > 5 && fileName.Substring(fileName.Length() - 5, 5) == ".json")
                {
                    jsonFiles.Insert(fileName);
                }
                else
                {
                    // Não termina com .json - provavelmente é diretório - adiciona à lista de subdiretórios
                    subdirectories.Insert(fullPath);
                }
            }
            
            if (!FindNextFile(handle, fileName, fileAttr)) break;
        }
        
        CloseFindFile(handle);
        
        // Ordenar arquivos JSON numericamente (DS_1, DS_2, ..., DS_10, DS_11, ...)
        if (jsonFiles.Count() > 0)
        {
            Log("Info", "[AskalDBLoader] 📋 Arquivos encontrados: " + jsonFiles.Count());
            SortDatasetFiles(jsonFiles);
        }
        
        // Carregar arquivos na ordem numérica
        for (int fileIdx = 0; fileIdx < jsonFiles.Count(); fileIdx++)
        {
            string sortedFileName = jsonFiles.Get(fileIdx);
            fullPath = directoryPath + sortedFileName;
            LoadDatasetFile(fullPath, sortedFileName);
        }
        
        // Processar subdiretórios recursivamente
        for (int dirIdx = 0; dirIdx < subdirectories.Count(); dirIdx++)
        {
            LoadAllDatasetsRecursive(subdirectories.Get(dirIdx));
        }
    }
    
    // SOLO SERVIDOR: Carrega datasets JSON do disco
    // CLIENTE NUNCA DEVE CHAMAR ESTE MÉTODO - usar RPC RequestDatasets
    static void LoadAllDatasets()
    {
        // CRITICAL: Só funciona no servidor
        if (GetGame().IsClient() && GetGame().IsMultiplayer())
        {
            Log("Error", "[AskalDBLoader] ❌ ERRO CRÍTICO: LoadAllDatasets() chamado no CLIENTE!");
            Log("Error", "[AskalDBLoader] Cliente NÃO deve carregar arquivos - usar RPC RequestDatasets");
            return;
        }
        
        Log("Info", "[AskalDBLoader] ==========================================");
        Log("Info", "[AskalDBLoader] LoadAllDatasets() - SERVIDOR APENAS");
        Log("Info", "[AskalDBLoader] ==========================================");
        
        // Determinar caminho (servidor geralmente usa $profile:)
        string path = AskalDatabase.GetDatabasePath();
        if (!path || path == "") 
        {
            path = "$profile:Askal/Database/Datasets/";
            Log("Info", "[AskalDBLoader] Caminho não configurado, usando padrão: " + path);
            AskalDatabase.SetDatabasePath(path);
        }
        
        Log("Info", "[AskalDBLoader] 🔄 Carregando datasets de: " + path);
        
        // Verificar se o diretório existe
        if (!FileExist(path))
        {
            Log("Error", "[AskalDBLoader] ❌ ERRO: Diretório não existe: " + path);
            Log("Error", "[AskalDBLoader] Verifique se o caminho está correto!");
            return;
        }
        
        Log("Info", "[AskalDBLoader] ✅ Diretório encontrado, iniciando busca...");
        Log("Info", "[AskalDBLoader] Buscando arquivos .json recursivamente...");

        LoadAllDatasetsRecursive(path);

        int totalDatasets = 0;
        
        if (AskalDatabase && AskalDatabase.m_Datasets)
        {
            totalDatasets = AskalDatabase.m_Datasets.Count();
        }
        
        Log("Info", "[AskalDBLoader] ==========================================");
        Log("Info", "[AskalDBLoader] ✅ Carregamento concluído:");
        Log("Info", "[AskalDBLoader]    - Datasets carregados: " + totalDatasets);
        Log("Info", "[AskalDBLoader] ==========================================");
    }
}
