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
    // Carrega recursivamente todos os arquivos .json das subpastas
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
        
        // Log apenas para debug (comentado para evitar spam)
        // Log("Info", "[AskalDBLoader] 🔍 Buscando em: " + searchPattern);
        
        while (true)
        {
            if (fileName != "" && fileName != "." && fileName != ".." && fileName != "manifest.json")
            {
                string fullPath = directoryPath + fileName;
                
                // Se termina com .json, é arquivo - carrega
                if (fileName.Length() > 5 && fileName.Substring(fileName.Length() - 5, 5) == ".json")
                {
                    // Carrega formato hierárquico (JsonDataset)
                    // Usa AskalJsonLoader para suportar arquivos grandes (>64KB)
                    JsonDataset jsonDataset = new JsonDataset();
                    if (!AskalJsonLoader<JsonDataset>.LoadFromFile(fullPath, jsonDataset, false))
                    {
                        Log("Warning", "[AskalDBLoader] ⚠️ Falha ao carregar: " + fileName);
                        continue;
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
                else
                {
                    // Não termina com .json - provavelmente é diretório - recursão
                    LoadAllDatasetsRecursive(fullPath);
                }
            }
            
            if (!FindNextFile(handle, fileName, fileAttr)) break;
        }
        
        CloseFindFile(handle);
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
    
    // MÉTODO REMOVIDO: LoadDatasetsByIds
    // Cliente NÃO carrega arquivos - usa RPC para receber dados do servidor
    // Este método era defasado e causava confusão
}
