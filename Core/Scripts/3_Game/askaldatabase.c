// AskalDatabase - definições (3_Game) - sem operações de I/O

// Classes para formato JSON do database (formato hierárquico: Dataset → Categories → Items)

class JsonItemEntry {
    int BasePrice;
    int SellPercent;
    ref array<string> Variants;
    ref array<string> Attachments;
    
    void JsonItemEntry() {
        SellPercent = 0;
        Variants = new array<string>();
        Attachments = new array<string>();
    }
}

// Uma categoria dentro do dataset (Categories)
// Nova estrutura: BasePrice, DisplayName e Items dentro da categoria
class JsonCategory {
    int BasePrice;
    int SellPercent;
    string DisplayName;
    ref map<string, ref JsonItemEntry> Items;
    
    void JsonCategory() {
        SellPercent = 0;
        Items = new map<string, ref JsonItemEntry>();
    }
}

// Dataset completo (formato hierárquico com Categories)
// BasePrice é definido por categoria, não mais por Defaults global
class JsonDataset {
    int Version;
    string DatasetName;
    string DisplayName;
    string Icon;
    ref map<string, ref JsonCategory> Categories;
    ref array<string> CategoryOrder;
    
    void JsonDataset() {
        Categories = new map<string, ref JsonCategory>();
        CategoryOrder = new array<string>();
    }
}


// Classes internas do Core (formato de runtime)
class ItemData {
    int Price;
    int SellPercent;
    ref array<string> Flags;
    ref map<string, string> DispatchValues;
    ref array<string> variants;
    ref array<string> attachments;

    void ItemData() {
        Flags = new array<string>();
        DispatchValues = new map<string, string>();
        variants = new array<string>();
        attachments = new array<string>();
    }
    
    // Converte JsonItemEntry para ItemData
    static ItemData FromJson(JsonItemEntry jsonItem, int defaultPrice, int defaultSellPercent)
    {
        if (!jsonItem) return null;
        
        ItemData itemData = new ItemData();
        
        // Preço do item: usa BasePrice do item se disponível, senão usa o padrão
        int itemPrice = defaultPrice;
        if (jsonItem.BasePrice > 0)
            itemPrice = jsonItem.BasePrice;
        if (itemPrice <= 0)
            itemPrice = AskalMarketDefaults.DEFAULT_BUY_PRICE;
        
        itemData.Price = itemPrice;
        itemData.SellPercent = defaultSellPercent;
        if (jsonItem.SellPercent > 0)
            itemData.SellPercent = jsonItem.SellPercent;
        if (itemData.SellPercent <= 0)
            itemData.SellPercent = AskalMarketDefaults.DEFAULT_SELL_PERCENT;
        itemData.variants = jsonItem.Variants;
        if (jsonItem.Attachments)
        {
            for (int attIdx = 0; attIdx < jsonItem.Attachments.Count(); attIdx++)
            {
                string attClass = jsonItem.Attachments.Get(attIdx);
                if (attClass && attClass != "")
                    itemData.attachments.Insert(attClass);
            }
        }
        
        return itemData;
    }

    array<string> GetAttachments()
    {
        return attachments;
    }
}

// Uma categoria dentro de um dataset
class AskalCategory {
    string CategoryID;        // ID da categoria (ex: "CAT_Pistols")
    string DisplayName;      // Nome para exibição (ex: "PISTOLAS")
    int BasePrice;           // Preço padrão da categoria
    int SellPercent;         // Percentual padrão de venda
    ref map<string, ref ItemData> Items;

    void AskalCategory() {
        Items = new map<string, ref ItemData>();
    }
    
    // Converte JsonCategory para AskalCategory interna
    // Nova estrutura: BasePrice e DisplayName vêm diretamente de JsonCategory
    static AskalCategory FromJson(string categoryID, JsonCategory jsonCategory, int datasetDefaultPrice)
    {
        if (!jsonCategory) return null;
        
        AskalCategory category = new AskalCategory();
        category.CategoryID = categoryID;
        
        // DisplayName e BasePrice vêm diretamente da categoria (obrigatório)
        category.DisplayName = jsonCategory.DisplayName;
        category.BasePrice = jsonCategory.BasePrice;
        category.SellPercent = jsonCategory.SellPercent;
        
        // Fallback apenas se não foi definido (não deveria acontecer)
        if (category.BasePrice <= 0)
            category.BasePrice = AskalMarketDefaults.DEFAULT_BUY_PRICE;
        
        if (category.SellPercent <= 0)
            category.SellPercent = AskalMarketDefaults.DEFAULT_SELL_PERCENT;
        
        // Fallback para DisplayName (usa o ID removendo prefixo CAT_)
        if (!category.DisplayName || category.DisplayName == "")
        {
            if (categoryID.Length() > 4 && categoryID.Substring(0, 4) == "CAT_")
                category.DisplayName = categoryID.Substring(4, categoryID.Length() - 4);
            else
                category.DisplayName = categoryID;
        }
        
        // Processa itens da categoria
        if (jsonCategory.Items)
        {
            for (int i = 0; i < jsonCategory.Items.Count(); i++)
            {
                string itemKey = jsonCategory.Items.GetKey(i);
                JsonItemEntry jsonItem = jsonCategory.Items.GetElement(i);
                
                // Se jsonItem for null (não deveria acontecer mais, mas mantemos como fallback)
                if (!jsonItem)
                {
                    jsonItem = new JsonItemEntry();
                }
                
                // ItemData com preço padrão da categoria
                ItemData itemData = ItemData.FromJson(jsonItem, category.BasePrice, category.SellPercent);
                
                if (itemData)
                {
                // Adiciona o item principal
                    category.Items.Set(itemKey, itemData);
                }
            }
        }
        
        // SEGUNDA PASSAGEM: Processar variantes
        // Verifica se variantes têm declaração própria com preço diferente
        for (int varItemIdx = 0; varItemIdx < jsonCategory.Items.Count(); varItemIdx++)
        {
            string varItemKey = jsonCategory.Items.GetKey(varItemIdx);
            JsonItemEntry varJsonItem = jsonCategory.Items.GetElement(varItemIdx);
            
            if (!varJsonItem) continue;
            
            ItemData varItemData = category.Items.Get(varItemKey);
            if (!varItemData || !varItemData.variants) continue;
            
            // Processar variantes deste item
            for (int varLoopIdx = 0; varLoopIdx < varItemData.variants.Count(); varLoopIdx++)
                    {
                string variantName = varItemData.variants.Get(varLoopIdx);
                if (!variantName || variantName == "") continue;
                
                // Verificar se a variante já existe no map (tem declaração própria)
                if (category.Items.Contains(variantName))
                {
                    // Variante já declarada - manter seu próprio preço
                    // (não fazer nada, já está correta)
                }
                else
                {
                    // Variante não declarada - criar com preço herdado
                            ItemData variantData = new ItemData();
                    variantData.Price = varItemData.Price; // Variantes herdam o preço se não tiverem declaração própria
                    if (variantData.Price <= 0)
                        variantData.Price = AskalMarketDefaults.DEFAULT_BUY_PRICE;
                    variantData.SellPercent = varItemData.SellPercent;
                    if (variantData.SellPercent <= 0)
                        variantData.SellPercent = category.SellPercent;
                            variantData.variants = new array<string>(); // Variantes não têm sub-variantes
                    category.Items.Set(variantName, variantData);
                        }
                    }
                }
        
        return category;
    }
}

// Dataset completo (com múltiplas categorias)
class Dataset {
    string DatasetID;           // ID do dataset (ex: "DS_Firearms")
    string DisplayName;         // Nome para exibição
    int Version;
    int DefaultPrice;           // Preço padrão global do dataset
    string Icon;                // Caminho/set do ícone do dataset
    ref map<string, ref AskalCategory> Categories;
    ref array<string> CategoryOrder;
    
    void Dataset() {
        Categories = new map<string, ref AskalCategory>();
        Icon = "set:dayz_inventory image:missing";
        CategoryOrder = new array<string>();
    }
    
    // Converte JsonDataset para Dataset interno
    static Dataset FromJson(JsonDataset jsonDataset)
    {
        if (!jsonDataset) return null;
        
        Dataset dataset = new Dataset();
        dataset.DatasetID = jsonDataset.DatasetName;
        dataset.Version = jsonDataset.Version;
        dataset.DisplayName = jsonDataset.DisplayName;
        dataset.Icon = jsonDataset.Icon;
        
        // Fallback para DisplayName
        if (!dataset.DisplayName || dataset.DisplayName == "")
        {
            if (dataset.DatasetID.Length() > 3 && dataset.DatasetID.Substring(0, 3) == "DS_")
                dataset.DisplayName = dataset.DatasetID.Substring(3, dataset.DatasetID.Length() - 3);
            else
                dataset.DisplayName = dataset.DatasetID;
        }

        if (!dataset.Icon || dataset.Icon == "")
            dataset.Icon = "set:dayz_inventory image:missing";
        
        // Preço padrão do dataset (removido - agora usa apenas BasePrice das categorias)
        dataset.DefaultPrice = 0; // Não usado mais, categorias têm BasePrice obrigatório
        
        // Processa cada categoria
        if (jsonDataset.Categories)
        {
            for (int c = 0; c < jsonDataset.Categories.Count(); c++)
            {
                string categoryID = jsonDataset.Categories.GetKey(c);
                JsonCategory jsonCategory = jsonDataset.Categories.GetElement(c);
                
                if (!jsonCategory) continue;
                
                // Converte categoria (BasePrice vem obrigatoriamente da categoria)
                AskalCategory category = AskalCategory.FromJson(categoryID, jsonCategory, 0);
                
                if (category)
                {
                    dataset.Categories.Set(categoryID, category);
                        }
                    }
                }

        // Aplicar ordem explícita (se fornecida)
        if (jsonDataset.CategoryOrder && jsonDataset.CategoryOrder.Count() > 0)
        {
            for (int orderIdx = 0; orderIdx < jsonDataset.CategoryOrder.Count(); orderIdx++)
            {
                string orderedId = jsonDataset.CategoryOrder.Get(orderIdx);
                if (!orderedId || orderedId == "")
                    continue;
                if (dataset.Categories && dataset.Categories.Contains(orderedId))
                {
                    if (dataset.CategoryOrder.Find(orderedId) == -1)
                        dataset.CategoryOrder.Insert(orderedId);
                }
                else
                {
                    Print("[AskalDatabase] ⚠️ CategoryOrder referencia categoria inexistente: " + orderedId + " em " + dataset.DatasetID);
                }
            }
        }

        // Garantir que todas as categorias estejam na lista
        if (dataset.Categories)
        {
            for (int verifyIdx = 0; verifyIdx < dataset.Categories.Count(); verifyIdx++)
            {
                string verifiedId = dataset.Categories.GetKey(verifyIdx);
                if (verifiedId && dataset.CategoryOrder.Find(verifiedId) == -1)
                    dataset.CategoryOrder.Insert(verifiedId);
            }
        }
        
        return dataset;
    }
}


class AskalDatabase
{
    // caminho padrão — pode ser alterado em runtime pelo loader
    static string m_DatabasePath = "$profile:Askal/Database/Datasets/";
    
    // Armazena datasets por ID (ex: "DS_Firearms")
    static ref map<string, ref Dataset> m_Datasets = new map<string, ref Dataset>();

    // getters / setters simples (sem I/O)
    static void SetDatabasePath(string p)
    {
        if (p && p != "") m_DatabasePath = p;
    }

    static string GetDatabasePath()
    {
        return m_DatabasePath;
    }

    // ========================================
    // REGISTRO DE DATASETS
    // ========================================
    
    // Registra um dataset hierárquico
    static void RegisterDataset(Dataset dataset)
    {
        if (!dataset || !dataset.DatasetID || dataset.DatasetID == "") return;
        m_Datasets.Set(dataset.DatasetID, dataset);
    }

    // ========================================
    // ACESSO POR DS_ (DATASET)
    // ========================================
    
    // Obtém um dataset completo por ID (ex: "DS_Firearms")
    static Dataset GetDataset(string datasetID)
    {
        if (!m_Datasets) return null;
        if (m_Datasets.Contains(datasetID))
            return m_Datasets.Get(datasetID);
        return null;
    }
    
    // Obtém todos os datasets
    static array<string> GetAllDatasetIDs()
    {
        array<string> ids = new array<string>();
        if (!m_Datasets) return ids;
        
        for (int i = 0; i < m_Datasets.Count(); i++)
        {
            string id = m_Datasets.GetKey(i);
            if (id && id != "") ids.Insert(id);
    }

        return ids;
    }

    // ========================================
    // ACESSO POR CAT_ (CATEGORIA)
    // ========================================
    
    // Obtém uma categoria específica por ID completo (ex: "CAT_Pistols" do "DS_Firearms")
    static AskalCategory GetCategory(string datasetID, string categoryID)
    {
        Dataset dataset = GetDataset(datasetID);
        if (!dataset || !dataset.Categories) return null;
        if (dataset.Categories.Contains(categoryID))
            return dataset.Categories.Get(categoryID);
        return null;
    }

    // Obtém todas as categorias de um dataset
    static array<string> GetCategoryIDs(string datasetID)
    {
        array<string> ids = new array<string>();
        Dataset dataset = GetDataset(datasetID);
        if (!dataset || !dataset.Categories) return ids;
        
        for (int i = 0; i < dataset.Categories.Count(); i++)
        {
            string id = dataset.Categories.GetKey(i);
            if (id && id != "") ids.Insert(id);
        }
        
        return ids;
    }
    
    // Busca categoria em qualquer dataset (útil para config do market)
    static AskalCategory FindCategory(string categoryID)
    {
        if (!m_Datasets) return null;
        
        // Busca em todos os datasets
        for (int i = 0; i < m_Datasets.Count(); i++)
        {
            Dataset dataset = m_Datasets.GetElement(i);
            if (!dataset || !dataset.Categories) continue;
            
            if (dataset.Categories.Contains(categoryID))
                return dataset.Categories.Get(categoryID);
        }
        
        return null;
    }

    // ========================================
    // ACESSO A ITENS
    // ========================================
    
    // Obtém preço de um item (busca em todos os datasets/categorias)
    static int GetPrice(string itemName)
    {
        if (!m_Datasets) return -1;
        
        for (int d = 0; d < m_Datasets.Count(); d++)
        {
            Dataset dataset = m_Datasets.GetElement(d);
            if (!dataset || !dataset.Categories) continue;
            
            // Busca em cada categoria do dataset
            for (int c = 0; c < dataset.Categories.Count(); c++)
        {
                AskalCategory category = dataset.Categories.GetElement(c);
                if (!category || !category.Items) continue;
                
                if (category.Items.Contains(itemName))
                    return category.Items.Get(itemName).Price;
        }
        }
        
        return -1;
    }

    // Obtém ItemData de um item (case-sensitive)
    static ItemData GetItem(string itemName)
    {
        if (!m_Datasets) return null;
        
        for (int d = 0; d < m_Datasets.Count(); d++)
        {
            Dataset dataset = m_Datasets.GetElement(d);
            if (!dataset || !dataset.Categories) continue;
            
            for (int c = 0; c < dataset.Categories.Count(); c++)
        {
                AskalCategory category = dataset.Categories.GetElement(c);
                if (!category || !category.Items) continue;
                
                if (category.Items.Contains(itemName))
                    return category.Items.Get(itemName);
            }
        }
        
        return null;
    }
    
    // Obtém ItemData de um item (case-insensitive)
    static ItemData GetItemCaseInsensitive(string itemName)
    {
        if (!m_Datasets || !itemName || itemName == "") return null;
        
        string searchLower = itemName;
        searchLower.ToLower();
        
        for (int d = 0; d < m_Datasets.Count(); d++)
        {
            Dataset dataset = m_Datasets.GetElement(d);
            if (!dataset || !dataset.Categories) continue;
            
            for (int c = 0; c < dataset.Categories.Count(); c++)
            {
                AskalCategory category = dataset.Categories.GetElement(c);
                if (!category || !category.Items) continue;
                
                // Primeiro tenta busca exata (mais rápida)
                if (category.Items.Contains(itemName))
                    return category.Items.Get(itemName);
                
                // Se não encontrou, busca case-insensitive
                for (int i = 0; i < category.Items.Count(); i++)
                {
                    string key = category.Items.GetKey(i);
                    string keyLower = key;
                    keyLower.ToLower();
                    
                    if (keyLower == searchLower)
                        return category.Items.Get(key);
                }
            }
        }
        
        return null;
    }

    // Obtém todos os nomes de datasets
    static array<string> GetAllDatasetDisplayNames()
    {
        array<string> names = new array<string>();
        if (!m_Datasets) return names;
        
        for (int i = 0; i < m_Datasets.Count(); i++)
        {
            Dataset d = m_Datasets.GetElement(i);
            if (d && d.DisplayName) names.Insert(d.DisplayName);
        }
        
        return names;
    }
}
