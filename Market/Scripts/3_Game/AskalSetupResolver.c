// AskalSetupResolver - Resolve configurações de Setup com prioridade correta
// Prioridade: Item > Categoria > Dataset

// Forward declaration
class JsonFileLoader<Class T>;

class AskalSetupResolver
{
    ref map<string, ref AskalSetupConfig> m_LoadedSetups;
    
    void AskalSetupResolver()
    {
        m_LoadedSetups = new map<string, ref AskalSetupConfig>();
    }

    // Registra um setup "inline" (ex: configurado diretamente no trader JSON)
    void RegisterInlineSetup(string setupName, map<string, int> setupItems, map<string, int> currencyMode = null)
    {
        if (!setupName || setupName == "" )
            return;

        AskalSetupConfig setup;
        if (m_LoadedSetups.Contains(setupName))
        {
            setup = m_LoadedSetups.Get(setupName);
        }
        else
        {
            setup = new AskalSetupConfig();
            m_LoadedSetups.Set(setupName, setup);
        }

        if (setup)
        {
            setup.SetSetupItems(setupItems);
            setup.SetCurrencyMode(currencyMode);
        }
    }
    
    // Resolve o modo de um item específico considerando todos os setups
    // Prioridade: Item individual > Categoria > Dataset
    // Retorna -1 se não encontrado em nenhum setup
    int ResolveItemMode(string itemClassname, array<string> setupNames)
    {
        if (!itemClassname || itemClassname == "" || !setupNames || setupNames.Count() == 0)
        {
            Print("[AskalSetupResolver] ⚠️ ResolveItemMode: parâmetros inválidos");
            return -1;
        }
        
        Print("[AskalSetupResolver] Resolvendo modo para: " + itemClassname + " (em " + setupNames.Count() + " setups)");
        
        int itemMode = -1;
        int highestPriority = -1; // 0 = Dataset, 1 = Categoria, 2 = Item (maior prioridade)
        int mode = -1; // Variável única para todos os blocos
        
        // Itera através de todos os setups
        for (int s = 0; s < setupNames.Count(); s++)
        {
            string setupName = setupNames.Get(s);
            AskalSetupConfig setup = GetSetup(setupName);
            if (!setup)
            {
                Print("[AskalSetupResolver] ⚠️ Setup " + setupName + " não foi carregado!");
                continue;
            }
            if (!setup.SetupItems)
            {
                Print("[AskalSetupResolver] ⚠️ Setup " + setupName + " não tem SetupItems!");
                continue;
            }
            
            Print("[AskalSetupResolver] Verificando setup " + setupName + " (" + setup.SetupItems.Count() + " configurações)");
            
            // 1. Prioridade máxima: Item individual
            if (setup.SetupItems.Contains(itemClassname))
            {
                mode = setup.SetupItems.Get(itemClassname);
                // Valida modo: -1 (Disabled) ou 0-3 (válidos)
                if (mode >= -1 && mode <= 3 && highestPriority < 2)
                {
                    itemMode = mode;
                    highestPriority = 2;
                }
                else if (mode > 3)
                {
                    Print("[AskalSetupResolver] ⚠️ Modo inválido (" + mode + ") para item " + itemClassname + " em setup " + setupName + ". Valores válidos: -1, 0-3");
                }
                continue; // Item encontrado, não precisa verificar categoria/dataset
            }
            
            // 2. Busca pela categoria do item
            if (highestPriority < 1)
            {
                // Busca em qual categoria o item está usando AskalAPI (métodos estáticos)
                Dataset itemDataset = null;
                AskalCategory itemCategory = null;
                
                // Itera através de todos os datasets
                array<string> allDatasets = AskalAPI.GetAllDatasets();
                foreach (string datasetID : allDatasets)
                {
                    Dataset dataset = AskalAPI.GetDataset(datasetID);
                    if (!dataset || !dataset.Categories) continue;
                    
                    // Busca em todas as categorias do dataset
                    for (int c = 0; c < dataset.Categories.Count(); c++)
                    {
                        string categoryID = dataset.Categories.GetKey(c);
                        AskalCategory category = dataset.Categories.GetElement(c);
                        if (!category || !category.Items) continue;
                        
                        // Verifica se o item está nesta categoria
                        if (category.Items.Contains(itemClassname))
                        {
                            itemDataset = dataset;
                            itemCategory = category;
                            break;
                        }
                    }
                    
                    if (itemCategory) break;
                }
                
                // Se encontrou a categoria, verifica se está no setup
                if (itemCategory && setup.SetupItems.Contains(itemCategory.CategoryID))
                {
                    mode = setup.SetupItems.Get(itemCategory.CategoryID);
                    // Valida modo
                    if (mode >= -1 && mode <= 3)
                    {
                        itemMode = mode;
                        highestPriority = 1;
                    }
                    else if (mode > 3)
                    {
                        Print("[AskalSetupResolver] ⚠️ Modo inválido (" + mode + ") para categoria " + itemCategory.CategoryID + " em setup " + setupName);
                    }
                    continue; // Categoria encontrada, não precisa verificar dataset
                }
                
                // 3. Verifica dataset inteiro
                if (itemDataset && setup.SetupItems.Contains(itemDataset.DatasetID))
                {
                    mode = setup.SetupItems.Get(itemDataset.DatasetID);
                    // Valida modo: -1 (Disabled) ou 0-3 (válidos)
                    // Nota: Modo 4+ será tratado como inválido, mas não bloqueia
                    if (highestPriority < 0 && mode >= -1 && mode <= 3)
                    {
                        itemMode = mode;
                        highestPriority = 0;
                    }
                }
            }
        }
        
        return itemMode;
    }
    
    // Resolve o modo de um dataset (DS_)
    // datasetKey pode vir com ou sem prefixo DS_
    int ResolveDatasetMode(string datasetKey, array<string> setupNames)
    {
        if (!datasetKey || datasetKey == "" || !setupNames || setupNames.Count() == 0)
            return -1;
        
        int datasetMode = -1;
        
        // Garantir que tem o prefixo DS_
        string dsKeyWithPrefix = datasetKey;
        if (dsKeyWithPrefix.IndexOf("DS_") != 0)
            dsKeyWithPrefix = "DS_" + datasetKey;
        
        // Itera através de todos os setups
        for (int s = 0; s < setupNames.Count(); s++)
        {
            string setupName = setupNames.Get(s);
            AskalSetupConfig setup = GetSetup(setupName);
            if (!setup || !setup.SetupItems) continue;
            
            // Verificar se o dataset está no setup (com prefixo DS_)
            if (setup.SetupItems.Contains(dsKeyWithPrefix))
            {
                int mode = setup.SetupItems.Get(dsKeyWithPrefix);
                // Valida modo: -1 (Disabled) ou 0-3 (válidos)
                if (mode >= -1 && mode <= 3)
                {
                    datasetMode = mode;
                    break; // Primeiro encontrado é usado
                }
            }
        }
        
        return datasetMode;
    }
    
    // Resolve o modo de uma categoria (CAT_)
    // categoryKey pode vir com ou sem prefixo CAT_
    int ResolveCategoryMode(string categoryKey, array<string> setupNames)
    {
        if (!categoryKey || categoryKey == "" || !setupNames || setupNames.Count() == 0)
            return -1;
        
        int categoryMode = -1;
        
        // Garantir que tem o prefixo CAT_
        string catKeyWithPrefix = categoryKey;
        if (catKeyWithPrefix.IndexOf("CAT_") != 0)
            catKeyWithPrefix = "CAT_" + categoryKey;
        
        // Itera através de todos os setups
        for (int s = 0; s < setupNames.Count(); s++)
        {
            string setupName = setupNames.Get(s);
            AskalSetupConfig setup = GetSetup(setupName);
            if (!setup || !setup.SetupItems) continue;
            
            // Verificar se a categoria está no setup (com prefixo CAT_)
            if (setup.SetupItems.Contains(catKeyWithPrefix))
            {
                int mode = setup.SetupItems.Get(catKeyWithPrefix);
                // Valida modo: -1 (Disabled) ou 0-3 (válidos)
                if (mode >= -1 && mode <= 3)
                {
                    categoryMode = mode;
                    break; // Primeiro encontrado é usado
                }
            }
        }
        
        return categoryMode;
    }
    
    // Carrega um setup por nome
    AskalSetupConfig GetSetup(string setupName)
    {
        if (!setupName || setupName == "") return null;
        
        // Já carregado?
        if (m_LoadedSetups.Contains(setupName))
        {
            Print("[AskalSetupResolver] Setup " + setupName + " já está em cache");
            return m_LoadedSetups.Get(setupName);
        }
        
        Print("[AskalSetupResolver] Carregando setup: " + setupName);
        
        // Carrega do arquivo (tenta múltiplos caminhos para compatibilidade)
        string path = "$profile:Askal/Market/Setups/" + setupName + ".json";
        if (!FileExist(path))
        {
            // Fallback: tenta em Traders/ (legacy)
            path = "$profile:Askal/Market/Traders/" + setupName + ".json";
            if (!FileExist(path))
            {
                // Fallback: tenta em $mission (alternativa)
                path = "$mission:Askal/Traders/" + setupName + ".json";
                if (!FileExist(path))
                {
                    Print("[AskalSetupResolver] ❌ Setup " + setupName + " não encontrado em nenhum caminho!");
                    return null;
                }
            }
        }
        
        Print("[AskalSetupResolver] ✅ Setup encontrado em: " + path);
        
        AskalSetupConfig setup = new AskalSetupConfig();
        JsonFileLoader<AskalSetupConfig>.JsonLoadFile(path, setup);
        
        if (setup)
        {
            if (setup.SetupItems)
            {
                Print("[AskalSetupResolver] ✅ Setup carregado com " + setup.SetupItems.Count() + " itens/configurações");
            }
            else
            {
                Print("[AskalSetupResolver] ⚠️ Setup carregado mas SetupItems é NULL!");
            }
            m_LoadedSetups.Set(setupName, setup);
        }
        else
        {
            Print("[AskalSetupResolver] ❌ Falha ao carregar setup de: " + path);
        }
        
        return setup;
    }
    
    // Limpa cache de setups (útil para reload)
    void ClearCache()
    {
        m_LoadedSetups.Clear();
    }
}

