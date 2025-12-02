// ==========================================
// ESTRUTURAS DE DADOS PARA O DATABASE ASKAL
// ==========================================

// Representa um item individual no database
class AskalItemData
{
	protected string m_ClassName;
	protected string m_DisplayName;
	protected string m_Category;
	protected int m_Price;
	protected int m_BasePrice;
	protected int m_Cost;
	protected int m_Restock;
	protected int m_Lifetime;
	protected ref map<string, bool> m_Flags;
	protected ref array<string> m_Variants;
	protected ref map<string, string> m_Attachments;
	protected ref array<string> m_Keywords;
	protected ref array<string> m_DefaultAttachments;
	
	void AskalItemData()
	{
		m_Flags = new map<string, bool>;
		m_Variants = new array<string>;
		m_Attachments = new map<string, string>;
		m_Keywords = new array<string>;
		m_DefaultAttachments = new array<string>;
		m_BasePrice = 0;
	}
	
	// Getters
	string GetClassName() { return m_ClassName; }
	string GetDisplayName() 
	{ 
		// Se não tiver display name, retorna o classname
		if (m_DisplayName == "" || !m_DisplayName)
			return m_ClassName;
		return m_DisplayName; 
	}
	string GetCategory() { return m_Category; }
	int GetPrice() { return m_Price; }
	int GetCost() { return m_Cost; }
	int GetRestock() { return m_Restock; }
	int GetLifetime() { return m_Lifetime; }
	map<string, bool> GetFlags() { return m_Flags; }
	array<string> GetVariants() { return m_Variants; }
	map<string, string> GetAttachments() { return m_Attachments; }
	array<string> GetKeywords() { return m_Keywords; }
	array<string> GetDefaultAttachments() { return m_DefaultAttachments; }
	int GetBasePrice() { return m_BasePrice; }
	
	// Setters
	void SetClassName(string name) { m_ClassName = name; }
	void SetDisplayName(string name) { m_DisplayName = name; }
	void SetCategory(string category) { m_Category = category; }
	void SetPrice(int price) { m_Price = price; }
	void SetBasePrice(int basePrice) { m_BasePrice = basePrice; }
	void SetCost(int cost) { m_Cost = cost; }
	void SetRestock(int restock) { m_Restock = restock; }
	void SetLifetime(int lifetime) { m_Lifetime = lifetime; }
	void SetDefaultAttachments(array<string> attachments)
	{
		m_DefaultAttachments.Clear();
		if (!attachments)
			return;
		foreach (string att : attachments)
		{
			if (att && att != "")
				m_DefaultAttachments.Insert(att);
		}
	}
	
	// Verifica se item está disponível na loja
	bool IsAvailableInStore()
	{
		if (m_Flags.Contains("Market"))
			return m_Flags.Get("Market");
		return false;
	}
	
	// Verifica se item está disponível no mercado
	bool IsAvailableInMarket()
	{
		if (m_Flags.Contains("Market"))
			return m_Flags.Get("Market");
		return false;
	}
	
	// Preço de venda (50% do preço de compra)
	int GetSellPrice()
	{
		int sellPrice = m_Price / 2;
		if (sellPrice < 1)
			sellPrice = 1;
		return sellPrice;
	}
	
	bool ValidateData()
	{
		if (!m_ClassName || m_ClassName == "") {
			Print("[AskalStore] ERROR: Invalid item class name");
			return false;
		}
		if (m_Price < 0) {
			Print("[AskalStore] ERROR: Invalid price for item " + m_ClassName);
			return false;
		}
		return true;
	}
	
	// Debug
	void DebugPrint()
	{
		Print("[AskalStore] DEBUG ==================");
		Print("[AskalStore] Item: " + m_ClassName);
		Print("[AskalStore] DisplayName: " + GetDisplayName());
		Print("[AskalStore] Price: " + m_Price);
		Print("[AskalStore] Cost: " + m_Cost);
		Print("[AskalStore] Variants: " + m_Variants.Count());
		Print("[AskalStore] Available in Store: " + IsAvailableInStore());
		Print("[AskalStore] ========================");
	}
}

// Representa uma categoria de items
class AskalCategoryData
{
	protected string m_CategoryName;
	protected string m_DisplayName;
	protected ref map<string, ref AskalItemData> m_Items;
	
	void AskalCategoryData()
	{
		m_Items = new map<string, ref AskalItemData>;
	}
	
	string GetCategoryName() { return m_CategoryName; }
	string GetDisplayName() 
	{ 
		if (m_DisplayName == "" || !m_DisplayName)
			return m_CategoryName;
		return m_DisplayName; 
	}
	map<string, ref AskalItemData> GetItems() { return m_Items; }
	
	void SetCategoryName(string name) { m_CategoryName = name; }
	void SetDisplayName(string name) { m_DisplayName = name; }
	
	// Alias para compatibilidade
	void SetName(string name) { m_CategoryName = name; }
	
	void AddItem(string className, AskalItemData itemData)
	{
		m_Items.Set(className, itemData);
	}
	
	void DebugPrint()
	{
		Print("[AskalCategoryData] Name: " + m_CategoryName);
		Print("[AskalCategoryData] DisplayName: " + m_DisplayName);
		Print("[AskalCategoryData] Items: " + m_Items.Count());
	}
	
	array<ref AskalItemData> GetStoreItems()
	{
		array<ref AskalItemData> storeItems = new array<ref AskalItemData>;
		
		for (int i = 0; i < m_Items.Count(); i++)
		{
			AskalItemData item = m_Items.GetElement(i);
			if (item && item.IsAvailableInStore())
			{
				storeItems.Insert(item);
			}
		}
		
		return storeItems;
	}
}

// Representa um dataset completo (um arquivo JSON)
class AskalDatasetData
{
	protected string m_DatasetName;
	protected string m_DisplayName;
	protected string m_Description;
	protected string m_FilePath;
    protected string m_Icon;
	protected ref map<string, ref AskalCategoryData> m_Categories;
	
	void AskalDatasetData()
	{
		m_Categories = new map<string, ref AskalCategoryData>;
        m_Icon = "set:dayz_inventory image:missing";
	}
	
	string GetDatasetName() { return m_DatasetName; }
	string GetDisplayName() 
	{ 
		if (m_DisplayName == "" || !m_DisplayName)
			return m_DatasetName;
		return m_DisplayName; 
	}
	string GetDescription() { return m_Description; }
	string GetFilePath() { return m_FilePath; }
    string GetIcon()
    {
        if (!m_Icon || m_Icon == "")
            return "set:dayz_inventory image:missing";
        return m_Icon;
    }
	map<string, ref AskalCategoryData> GetCategories() { return m_Categories; }
	
	void SetDatasetName(string name) { m_DatasetName = name; }
	void SetDisplayName(string name) { m_DisplayName = name; }
	void SetDescription(string desc) { m_Description = desc; }
	void SetFilePath(string path) { m_FilePath = path; }
    void SetIcon(string path) { m_Icon = path; }
	
	void AddCategory(string categoryName, AskalCategoryData categoryData)
	{
		m_Categories.Set(categoryName, categoryData);
	}
	
	AskalCategoryData GetCategory(string categoryName)
	{
		if (m_Categories.Contains(categoryName))
			return m_Categories.Get(categoryName);
		return null;
	}
	
	// Retorna todos os items do dataset disponíveis na loja
	array<ref AskalItemData> GetAllStoreItems()
	{
		array<ref AskalItemData> allItems = new array<ref AskalItemData>;
		
		for (int i = 0; i < m_Categories.Count(); i++)
		{
			AskalCategoryData category = m_Categories.GetElement(i);
			if (category)
			{
				array<ref AskalItemData> categoryItems = category.GetStoreItems();
				for (int j = 0; j < categoryItems.Count(); j++)
				{
					allItems.Insert(categoryItems.Get(j));
				}
			}
		}
		
		return allItems;
	}
	
	void DebugPrint()
	{
		Print("[AskalDatasetData] Name: " + m_DatasetName);
		Print("[AskalDatasetData] DisplayName: " + m_DisplayName);
		Print("[AskalDatasetData] Categories: " + m_Categories.Count());
	}
}


