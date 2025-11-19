// ==========================================
// AskalCoreDatabaseManager - Core Database Manager
// Part of Askal_Core
// Simples wrapper para expor dados do AskalDatabase
// SEM dependências de Market ou Trader
// ==========================================

// Classe simples para expor ItemData (apenas para uso interno do Core)
// NOTA: Market tem sua própria AskalItemData completa em AskalDatabaseStructures.c
class AskalCoreItemData
{
	protected string m_ClassName;
	protected string m_Category;
	protected int m_Price;
	protected ref array<string> m_Variants;
	protected ref map<string, bool> m_Flags;
	protected ref array<string> m_Attachments;
	
	void AskalCoreItemData()
	{
		m_Variants = new array<string>();
		m_Flags = new map<string, bool>();
		m_Attachments = new array<string>();
	}
	
	// Getters
	string GetClassName() { return m_ClassName; }
	string GetCategory() { return m_Category; }
	int GetPrice() { return m_Price; }
	array<string> GetVariants() { return m_Variants; }
	map<string, bool> GetFlags() { return m_Flags; }
	array<string> GetAttachments() { return m_Attachments; }
	
	// Setters
	void SetClassName(string name) { m_ClassName = name; }
	void SetCategory(string category) { m_Category = category; }
	void SetPrice(int price) { m_Price = price; }
	
	// Criar a partir de ItemData
	static AskalCoreItemData FromItemData(string className, string category, ItemData itemData)
	{
		if (!itemData) return null;
		
		AskalCoreItemData askalItem = new AskalCoreItemData();
		askalItem.SetClassName(className);
		askalItem.SetCategory(category);
		askalItem.SetPrice(itemData.Price);
		
		if (itemData.variants)
		{
			for (int i = 0; i < itemData.variants.Count(); i++)
			{
				askalItem.GetVariants().Insert(itemData.variants.Get(i));
			}
		}

		if (itemData.attachments)
		{
			for (int attIdx = 0; attIdx < itemData.attachments.Count(); attIdx++)
			{
				askalItem.GetAttachments().Insert(itemData.attachments.Get(attIdx));
			}
		}
		
		return askalItem;
	}
}

class AskalCoreDatabaseManager
{
	private static ref AskalCoreDatabaseManager s_Instance;
	protected bool m_IsLoaded = false;
	
	void AskalCoreDatabaseManager()
	{
		m_IsLoaded = false;
	}
	
	// Singleton
	static AskalCoreDatabaseManager GetInstance()
	{
		if (!s_Instance)
		{
			s_Instance = new AskalCoreDatabaseManager();
		}
		return s_Instance;
	}
	
	// Load database - apenas marca como carregado (o loader já faz o trabalho)
	// NOTA: Este método só deve ser chamado no servidor
	bool LoadDatabase()
	{
		Print("[ASKAL_CORE] ==========================================");
		Print("[ASKAL_CORE] LoadDatabase() CHAMADO!");
		Print("[ASKAL_CORE] ==========================================");
		
		// Configurar caminho do database APENAS no servidor
		// Cliente recebe dados via RPC, não precisa acessar filesystem
		if (AskalCoreHelpers.IsServerSafe())
		{
			// Configurar caminho do database se ainda não foi configurado
			string dbPath = AskalDatabase.GetDatabasePath();
			if (!dbPath || dbPath == "")
			{
				dbPath = "$profile:Askal/Database/Datasets/";
				AskalDatabase.SetDatabasePath(dbPath);
				Print("[ASKAL_CORE] Caminho do database configurado: " + dbPath);
			}
			else
			{
				Print("[ASKAL_CORE] Caminho do database já configurado: " + dbPath);
			}
		}
		else
		{
			Print("[ASKAL_CORE] Cliente detectado - database será recebido via RPC");
		}
		
		// O AskalDatabaseLoader já carrega os dados
		// Este método apenas marca como pronto
		m_IsLoaded = true;
		Print("[ASKAL_CORE] Database marked as loaded");
		Print("[ASKAL_CORE] ==========================================");
		return true;
	}
	
	// ========================================
	// ACESSO POR DS_ (DATASET)
	// ========================================
	
	// Obtém um dataset completo por ID (ex: "DS_Firearms")
	Dataset GetDataset(string datasetID)
	{
		if (!m_IsLoaded)
		{
			Print("[ASKAL_CORE] Database not loaded");
			return null;
		}
		
		return AskalDatabase.GetDataset(datasetID);
	}
	
	// Obtém todos os IDs de datasets
	array<string> GetAllDatasetIDs()
	{
		if (!m_IsLoaded)
		{
			Print("[ASKAL_CORE] Database not loaded");
			return new array<string>();
		}
		
		return AskalDatabase.GetAllDatasetIDs();
	}
	
	// ========================================
	// ACESSO POR CAT_ (CATEGORIA)
	// ========================================
	
	// Obtém uma categoria específica por ID (ex: "CAT_Pistols")
	AskalCategory GetCategoryByID(string categoryID)
	{
		if (!m_IsLoaded)
		{
			Print("[ASKAL_CORE] Database not loaded");
			return null;
		}
		
		return AskalDatabase.FindCategory(categoryID);
	}
	
	// Obtém todas as categorias de um dataset
	array<string> GetCategoryIDs(string datasetID)
	{
		if (!m_IsLoaded)
		{
			Print("[ASKAL_CORE] Database not loaded");
			return new array<string>();
		}
		
		return AskalDatabase.GetCategoryIDs(datasetID);
	}
	
	// ========================================
	// ACESSO A ITENS
	// ========================================
	
	// Get item by className (busca em datasets e categorias legadas)
	AskalCoreItemData GetItem(string className)
	{
		if (!m_IsLoaded)
		{
			Print("[ASKAL_CORE] Database not loaded");
			return null;
		}
		
		// Buscar usando método direto do AskalDatabase (busca em tudo)
		ItemData itemData = AskalDatabase.GetItem(className);
		if (itemData)
		{
			// Tenta determinar a categoria do item
			string categoryName = "Unknown";
			
			// Busca em datasets hierárquicos
			array<string> datasetIDs = AskalDatabase.GetAllDatasetIDs();
			for (int d = 0; d < datasetIDs.Count(); d++)
			{
				Dataset dataset = AskalDatabase.GetDataset(datasetIDs.Get(d));
				if (!dataset || !dataset.Categories) continue;
				
				for (int c = 0; c < dataset.Categories.Count(); c++)
				{
					string catID = dataset.Categories.GetKey(c);
					AskalCategory cat = dataset.Categories.GetElement(c);
					if (cat && cat.Items && cat.Items.Contains(className))
					{
						categoryName = cat.DisplayName;
						break;
					}
				}
				
				if (categoryName != "Unknown") break;
			}
			
			
			return AskalCoreItemData.FromItemData(className, categoryName, itemData);
		}
		
		return null;
	}
	
	// Get all items (busca em datasets hierárquicos e categorias legadas)
	array<ref AskalCoreItemData> GetAllItems()
	{
		array<ref AskalCoreItemData> allItems = new array<ref AskalCoreItemData>();
		
		if (!m_IsLoaded)
		{
			Print("[ASKAL_CORE] Database not loaded");
			return allItems;
		}
		
		// Busca em datasets hierárquicos
		array<string> datasetIDs = AskalDatabase.GetAllDatasetIDs();
		for (int d = 0; d < datasetIDs.Count(); d++)
		{
			Dataset dataset = AskalDatabase.GetDataset(datasetIDs.Get(d));
			if (!dataset || !dataset.Categories) continue;
			
			for (int c = 0; c < dataset.Categories.Count(); c++)
			{
				string catID = dataset.Categories.GetKey(c);
				AskalCategory cat = dataset.Categories.GetElement(c);
				if (!cat || !cat.Items) continue;
				
				for (int i = 0; i < cat.Items.Count(); i++)
				{
					string itemKey = cat.Items.GetKey(i);
					ItemData itemData = cat.Items.GetElement(i);
					
					if (itemData)
					{
						AskalCoreItemData askalItem = AskalCoreItemData.FromItemData(itemKey, cat.DisplayName, itemData);
						if (askalItem)
							allItems.Insert(askalItem);
					}
				}
			}
		}
		
		return allItems;
	}
	
	// Obtém todos os nomes de datasets
	array<string> GetAllDatasets()
	{
		if (!m_IsLoaded)
		{
			Print("[ASKAL_CORE] Database not loaded");
			return new array<string>();
		}
		
		return AskalDatabase.GetAllDatasetDisplayNames();
	}
	
	// Obtém itens de uma categoria específica
	array<ref AskalCoreItemData> GetItemsFromCategory(string categoryID)
	{
		array<ref AskalCoreItemData> items = new array<ref AskalCoreItemData>();
		
		if (!m_IsLoaded)
		{
			Print("[ASKAL_CORE] Database not loaded");
			return items;
		}
		
		AskalCategory category = AskalDatabase.FindCategory(categoryID);
		if (category && category.Items)
		{
			for (int i = 0; i < category.Items.Count(); i++)
			{
				string itemKey = category.Items.GetKey(i);
				ItemData itemData = category.Items.GetElement(i);
				
				if (itemData)
				{
					AskalCoreItemData askalItem = AskalCoreItemData.FromItemData(itemKey, category.DisplayName, itemData);
					if (askalItem)
						items.Insert(askalItem);
				}
			}
		}
		
		return items;
	}
	
	// Reload database
	// NOTA: Este método apenas marca como não carregado
	// O loader (5_Mission) deve ser chamado externamente
	void ReloadDatabase()
	{
		Print("[ASKAL_CORE] Marking database for reload...");
		m_IsLoaded = false;
		Print("[ASKAL_CORE] Database marked for reload. Call AskalDatabaseLoader.LoadAllDatasets() from 5_Mission to reload.");
	}
}
