// AskalDatasetManager - Interface entre Core e outros módulos (formato hierárquico)
class AskalDatasetManager
{
    // Retorna item de uma categoria específica por ID (CAT_)
    ref ItemData FindItem(string categoryID, string className)
    {
        if (!categoryID || !className) return null;
        
        AskalCategory category = AskalDatabase.FindCategory(categoryID);
        if (!category || !category.Items) return null;
        
        if (category.Items.Contains(className))
            return category.Items.Get(className);
            
        return null;
    }

    // Lista todos os datasets disponíveis
    array<string> GetAvailableDatasets()
    {
        return AskalDatabase.GetAllDatasetIDs();
    }

    // Lista todas as categorias de um dataset
    array<string> GetCategoriesFromDataset(string datasetID)
    {
        return AskalDatabase.GetCategoryIDs(datasetID);
    }

    // Retorna items de uma categoria por ID (CAT_)
    map<string, ref ItemData> GetItemsFromCategory(string categoryID)
    {
        map<string, ref ItemData> items = new map<string, ref ItemData>();
        
        AskalCategory category = AskalDatabase.FindCategory(categoryID);
        if (!category || !category.Items) return items;
        
        for (int i = 0; i < category.Items.Count(); i++)
        {
            string className = category.Items.GetKey(i);
            ItemData itemData = category.Items.Get(className);
            if (itemData)
                items.Set(className, itemData);
        }
        
        return items;
    }

    // Retorna N items aleatórios de uma categoria
    array<ref ItemData> GetRandomItems(string categoryID, int count)
    {
        array<ref ItemData> items = new array<ref ItemData>();
        
        AskalCategory category = AskalDatabase.FindCategory(categoryID);
        if (!category || !category.Items) return items;
        
        // Coleta todas as chaves
        array<string> classNames = new array<string>();
        for (int i = 0; i < category.Items.Count(); i++)
        {
            classNames.Insert(category.Items.GetKey(i));
        }
        
        // Retorna até 'count' itens (ou todos se houver menos)
        int itemsToReturn = count;
        if (itemsToReturn > classNames.Count())
            itemsToReturn = classNames.Count();
        
        for (int j = 0; j < itemsToReturn; j++)
        {
            string className = classNames.Get(j);
            ItemData itemData = category.Items.Get(className);
            if (itemData)
                items.Insert(itemData);
        }
        
        return items;
    }
}
