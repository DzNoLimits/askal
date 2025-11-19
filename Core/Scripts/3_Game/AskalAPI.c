// ==========================================
// AskalAPI - Global API for Askal Core
// Provides access to Core database data
// ==========================================

class AskalAPI
{
	// Global Database API - Get item by className
	static AskalCoreItemData GetItem(string className)
	{
		AskalCoreDatabaseManager dbManager = AskalCoreDatabaseManager.GetInstance();
		return dbManager.GetItem(className);
	}
	
	// Global Database API - Get all items
	static array<ref AskalCoreItemData> GetAllItems()
	{
		AskalCoreDatabaseManager dbManager = AskalCoreDatabaseManager.GetInstance();
		return dbManager.GetAllItems();
	}
	
	// Global Database API - Get all datasets
	static array<string> GetAllDatasets()
	{
		AskalCoreDatabaseManager dbManager = AskalCoreDatabaseManager.GetInstance();
		return dbManager.GetAllDatasets();
	}
	
	// Global Database API - Get dataset by ID (DS_)
	static Dataset GetDataset(string datasetID)
	{
		AskalCoreDatabaseManager dbManager = AskalCoreDatabaseManager.GetInstance();
		return dbManager.GetDataset(datasetID);
	}
	
	// Global Database API - Get category by ID (CAT_)
	static AskalCategory GetCategory(string categoryID)
	{
		AskalCoreDatabaseManager dbManager = AskalCoreDatabaseManager.GetInstance();
		return dbManager.GetCategoryByID(categoryID);
	}
	
	// Global Database API - Get items from category by ID (CAT_)
	static array<ref AskalCoreItemData> GetItemsFromCategory(string categoryID)
	{
		AskalCoreDatabaseManager dbManager = AskalCoreDatabaseManager.GetInstance();
		return dbManager.GetItemsFromCategory(categoryID);
	}
	
	// Global Database API - Reload database
	// NOTA: Este método apenas marca para reload. O loader deve ser chamado externamente
	static void Reload()
	{
		AskalCoreDatabaseManager dbManager = AskalCoreDatabaseManager.GetInstance();
		dbManager.ReloadDatabase();
		Print("[AskalAPI] Database marked for reload. You need to call AskalDatabaseLoader.LoadAllDatasets() from 5_Mission context.");
	}
	
	// Get price for item
	static int GetPrice(string className)
	{
		AskalCoreItemData item = GetItem(className);
		if (item)
			return item.GetPrice();
		return -1;
	}
}
