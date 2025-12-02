// ==========================================
// CACHE DO DATABASE NO CLIENTE
// Cliente armazena dados sincronizados do servidor
// Padrão simples inspirado no TraderX
// ==========================================

// Classe serializável para enviar item via RPC
class AskalItemSyncData
{
	string ClassName;
	string DisplayName;
	int BasePrice;
	int SellPercent; // Percentual do BasePrice ao vender (0-100)
	ref array<string> Variants;
	ref array<string> Attachments;
	
	void AskalItemSyncData()
	{
		ClassName = "";
		DisplayName = "";
		BasePrice = 0;
		SellPercent = 50; // Default: 50%
		Variants = new array<string>;
		Attachments = new array<string>;
	}
}

// Classe serializável para enviar categoria via RPC
class AskalCategorySyncData
{
	string CategoryID;
	string DisplayName;
	int BasePrice;
	int SellPercent;
	ref map<string, ref AskalItemSyncData> Items;
	
	void AskalCategorySyncData()
	{
		CategoryID = "";
		DisplayName = "";
		BasePrice = 0;
		SellPercent = 50; // Default: 50%
		Items = new map<string, ref AskalItemSyncData>;
	}
}

// Classe para enviar categoria individual (parte de um dataset)
class AskalCategoryPartData
{
	string DatasetID;
	string CategoryID;
	string DisplayName;
	int BasePrice;
	ref map<string, ref AskalItemSyncData> Items;
	
	void AskalCategoryPartData()
	{
		DatasetID = "";
		CategoryID = "";
		DisplayName = "";
		BasePrice = 0;
		Items = new map<string, ref AskalItemSyncData>;
	}
}

// Classe COMPACTA para enviar batch de itens (apenas ClassName + Price, sem DisplayName)
// DisplayName será obtido no cliente via ConfigGetText
class AskalCategoryBatchData
{
	string DatasetID;
	string CategoryID;
	string DisplayName;
	int BasePrice;
	int CategorySellPercent;
	int BatchIndex;          // Índice deste batch (0-based)
	int TotalBatches;       // Total de batches para esta categoria
	ref array<string> ItemClassNames;  // Apenas classnames (compacto)
	ref array<int> ItemPrices;         // Apenas preços (compacto)
	ref array<string> ItemVariants;    // Variantes como string "item1,item2" (compacto)
	ref array<string> ItemAttachments; // Attachments como string "att1,att2"
	ref array<int> ItemSellPercents;
	
	void AskalCategoryBatchData()
	{
		DatasetID = "";
		CategoryID = "";
		DisplayName = "";
		BasePrice = 0;
		CategorySellPercent = 50; // Default: 50%
		BatchIndex = 0;
		TotalBatches = 1;
		ItemClassNames = new array<string>;
		ItemPrices = new array<int>;
		ItemVariants = new array<string>;
		ItemAttachments = new array<string>;
		ItemSellPercents = new array<int>;
	}
}

// Classe serializável para enviar dataset via RPC
class AskalDatasetSyncData
{
	string DatasetID;
	string DisplayName;
	string Description;
	string Icon;
	ref map<string, ref AskalCategorySyncData> Categories;
    ref array<string> CategoryOrder;
	
	void AskalDatasetSyncData()
	{
		DatasetID = "";
		DisplayName = "";
		Description = "";
		Icon = "";
		Categories = new map<string, ref AskalCategorySyncData>;
        CategoryOrder = new array<string>();
	}
}

// Gerenciador de cache do cliente
class AskalDatabaseClientCache
{
	private static ref AskalDatabaseClientCache s_Instance;
	
	protected ref map<string, ref AskalDatasetSyncData> m_CachedDatasets;
	protected bool m_IsSynced = false;
	
	void AskalDatabaseClientCache()
	{
		m_CachedDatasets = new map<string, ref AskalDatasetSyncData>;
		Print("[AskalCache] Cache inicializado");
	}
	
	static AskalDatabaseClientCache GetInstance()
	{
		if (!s_Instance)
		{
			s_Instance = new AskalDatabaseClientCache();
		}
		return s_Instance;
	}
	
	// Limpar cache
	void Clear()
	{
		m_CachedDatasets.Clear();
		m_IsSynced = false;
		Print("[AskalCache] Cache limpo");
	}
	
	// Adicionar um dataset ao cache (usado durante sincronização)
	void AddDataset(AskalDatasetSyncData dataset)
	{
		if (dataset)
		{
			m_CachedDatasets.Set(dataset.DatasetID, dataset);
			Print("[AskalCache] Dataset adicionado: " + dataset.DatasetID + " (Total: " + m_CachedDatasets.Count() + ")");
		}
	}
	
	// Marcar como sincronizado
	void SetSynced(bool synced)
	{
		m_IsSynced = synced;
		Print("[AskalCache] Synced = " + synced);
		if (synced)
		{
			Print("[AskalCache] ✅ Cache contém " + m_CachedDatasets.Count() + " datasets");
		}
	}
	
	// Verificar se está sincronizado
	bool IsSynced()
	{
		return m_IsSynced && m_CachedDatasets.Count() > 0;
	}
	
	// Obter todos os datasets
	map<string, ref AskalDatasetSyncData> GetDatasets()
	{
		return m_CachedDatasets;
	}
	
	// Obter dataset específico
	AskalDatasetSyncData GetDataset(string datasetID)
	{
		if (m_CachedDatasets.Contains(datasetID))
			return m_CachedDatasets.Get(datasetID);
		return NULL;
	}
	
	// Buscar item por classname
	AskalItemSyncData FindItem(string className)
	{
		if (!className || className == "")
			return NULL;
		
		string normalized = className;
		normalized.ToLower();
		bool hasNormalized = (normalized != className);
		
		for (int i = 0; i < m_CachedDatasets.Count(); i++)
		{
			AskalDatasetSyncData dataset = m_CachedDatasets.GetElement(i);
			if (!dataset) continue;
			
			for (int j = 0; j < dataset.Categories.Count(); j++)
			{
				AskalCategorySyncData category = dataset.Categories.GetElement(j);
				if (!category) continue;
				
				if (category.Items.Contains(className))
					return category.Items.Get(className);
				
				if (hasNormalized && category.Items.Contains(normalized))
					return category.Items.Get(normalized);
				
				// Fallback: comparação case-insensitive (evita perdas quando servidor envia letras maiúsculas)
				for (int k = 0; k < category.Items.Count(); k++)
				{
					string key = category.Items.GetKey(k);
					if (!key || key == "")
						continue;
					
					string keyNormalized = key;
					keyNormalized.ToLower();
					if (keyNormalized == normalized)
						return category.Items.GetElement(k);
				}
			}
		}
		return NULL;
	}
	
	// Debug
	void PrintCache()
	{
		Print("[AskalCache] ========== CACHE INFO ==========");
		Print("[AskalCache] Synced: " + m_IsSynced);
		Print("[AskalCache] Datasets: " + m_CachedDatasets.Count());
		
		for (int i = 0; i < m_CachedDatasets.Count(); i++)
		{
			AskalDatasetSyncData dataset = m_CachedDatasets.GetElement(i);
			Print("[AskalCache] Dataset: " + dataset.DatasetID + " (" + dataset.DisplayName + ")");
			Print("[AskalCache]   Categories: " + dataset.Categories.Count());
			
			for (int j = 0; j < dataset.Categories.Count(); j++)
			{
				AskalCategorySyncData category = dataset.Categories.GetElement(j);
				Print("[AskalCache]     " + category.CategoryID + " - Items: " + category.Items.Count());
			}
		}
		
		Print("[AskalCache] ================================");
	}
}

