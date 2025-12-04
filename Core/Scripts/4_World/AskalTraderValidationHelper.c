// ==========================================
// AskalTraderValidationHelper - Validação de SetupItems no Servidor
// Garante que transações respeitem as configurações do trader
// ==========================================

class AskalTraderValidationHelper
{
	// Normalizar ID de dataset (adicionar prefixo DS_ se necessário)
	static string NormalizeDatasetID(string datasetID)
	{
		if (!datasetID || datasetID == "")
			return "";
		
		// Se não tem prefixo DS_, adicionar
		if (datasetID.IndexOf("DS_") != 0)
			return "DS_" + datasetID;
		
		return datasetID;
	}
	
	// Normalizar ID de categoria (adicionar prefixo CAT_ se necessário)
	static string NormalizeCategoryID(string categoryID)
	{
		if (!categoryID || categoryID == "")
			return "";
		
		// Se não tem prefixo CAT_, adicionar
		if (categoryID.IndexOf("CAT_") != 0)
			return "CAT_" + categoryID;
		
		return categoryID;
	}
	
	// Obter modo de um dataset
	// Wrapper para compatibilidade - usa ResolveModeForItem internamente
	static int GetDatasetMode(map<string, int> setupItems, string datasetID)
	{
		if (!setupItems || setupItems.Count() == 0)
			return 3; // Sem filtros, tudo disponível
		
		// Normalizar ID
		string normalizedID = NormalizeDatasetID(datasetID);
		
		// Verificar se há "ALL": 3 (todos os datasets disponíveis)
		int allMode = -1;
		if (setupItems.Contains("ALL"))
		{
			allMode = setupItems.Get("ALL");
		}
		
		// Verificar se há configuração específica para este dataset (DS_*)
		if (setupItems.Contains(normalizedID))
		{
			return setupItems.Get(normalizedID);
		}
		
		// Se "ALL" está definido, usar esse modo
		if (allMode >= 0)
		{
			return allMode;
		}
		
		// Sem configuração, não disponível
		return -1;
	}
	
	// Obter modo de uma categoria
	// Wrapper para compatibilidade - usa ResolveModeForItem internamente
	static int GetCategoryMode(map<string, int> setupItems, string datasetID, string categoryID)
	{
		if (!setupItems || setupItems.Count() == 0)
			return 3; // Sem filtros, tudo disponível
		
		// Normalizar IDs
		string normalizedDatasetID = NormalizeDatasetID(datasetID);
		string normalizedCategoryID = NormalizeCategoryID(categoryID);
		
		// Verificar categoria específica (CAT_*)
		if (setupItems.Contains(normalizedCategoryID))
		{
			return setupItems.Get(normalizedCategoryID);
		}
		
		// Verificar dataset (DS_*)
		int datasetMode = GetDatasetMode(setupItems, normalizedDatasetID);
		if (datasetMode >= 0)
		{
			return datasetMode;
		}
		
		// Verificar "ALL"
		if (setupItems.Contains("ALL"))
		{
			return setupItems.Get("ALL");
		}
		
		return -1;
	}
	
	// ResolveModeForItem: precedência Item > Category > Dataset > ALL
	// Função centralizada e reutilizável para resolução de modo
	static int ResolveModeForItem(string itemClassname, string categoryId, string datasetId, int defaultAllMode, map<string, int> traderSetupItems, map<string, int> traderSetupCategories = NULL, map<string, int> traderSetupDatasets = NULL)
	{
		// Se não há configurações, retornar default
		bool hasSetupItems = (traderSetupItems && traderSetupItems.Count() > 0);
		bool hasSetupCategories = (traderSetupCategories && traderSetupCategories.Count() > 0);
		bool hasSetupDatasets = (traderSetupDatasets && traderSetupDatasets.Count() > 0);
		
		if (!hasSetupItems && !hasSetupCategories && !hasSetupDatasets)
		{
			return defaultAllMode;
		}
		
		// Normalizar IDs
		string normalizedDatasetID = NormalizeDatasetID(datasetId);
		string normalizedCategoryID = NormalizeCategoryID(categoryId);
		
		// PRIORIDADE 1: item-level (verificar em SetupItems)
		// Primeiro tentar busca exata (case-sensitive)
		if (traderSetupItems && traderSetupItems.Contains(itemClassname))
		{
			int itemMode = traderSetupItems.Get(itemClassname);
			Print("[AskalTraderValidation] ✅ PRIORIDADE 1 (ITEM): Item '" + itemClassname + "' encontrado em SetupItems (exato) com modo: " + itemMode);
			return itemMode;
		}
		
		// Se não encontrou, tentar busca case-insensitive
		if (traderSetupItems && traderSetupItems.Count() > 0)
		{
			string itemLower = itemClassname;
			itemLower.ToLower();
			for (int checkIdx = 0; checkIdx < traderSetupItems.Count(); checkIdx++)
			{
				string key = traderSetupItems.GetKey(checkIdx);
				// Pular se a chave começa com CAT_, DS_ ou é "ALL" (não são itens)
				if (key.IndexOf("CAT_") == 0 || key.IndexOf("DS_") == 0 || key == "ALL")
					continue;
				
				string keyLower = key;
				keyLower.ToLower();
				if (keyLower == itemLower)
				{
					int foundItemMode = traderSetupItems.Get(key);
					Print("[AskalTraderValidation] ✅ PRIORIDADE 1 (ITEM): Item '" + itemClassname + "' encontrado em SetupItems como '" + key + "' (case-insensitive) com modo: " + foundItemMode);
					return foundItemMode;
				}
			}
		}
		
		// PRIORIDADE 2: category-level
		if (categoryId != "" && normalizedCategoryID != "")
		{
			// Tentar SetupCategories primeiro (se existir)
			if (traderSetupCategories && traderSetupCategories.Contains(normalizedCategoryID))
			{
				int catMode = traderSetupCategories.Get(normalizedCategoryID);
				Print("[AskalTraderValidation] ✅ PRIORIDADE 2 (CATEGORY): Categoria '" + normalizedCategoryID + "' encontrada em SetupCategories com modo: " + catMode);
				return catMode;
			}
			// Fallback para SetupItems (compatibilidade)
			if (traderSetupItems && traderSetupItems.Contains(normalizedCategoryID))
			{
				int catModeFromItems = traderSetupItems.Get(normalizedCategoryID);
				Print("[AskalTraderValidation] ✅ PRIORIDADE 2 (CATEGORY): Categoria '" + normalizedCategoryID + "' encontrada em SetupItems com modo: " + catModeFromItems);
				return catModeFromItems;
			}
		}
		
		// PRIORIDADE 3: dataset-level
		if (datasetId != "" && normalizedDatasetID != "")
		{
			// Tentar SetupDatasets primeiro (se existir)
			if (traderSetupDatasets && traderSetupDatasets.Contains(normalizedDatasetID))
			{
				int dsMode = traderSetupDatasets.Get(normalizedDatasetID);
				Print("[AskalTraderValidation] ✅ PRIORIDADE 3 (DATASET): Dataset '" + normalizedDatasetID + "' encontrado em SetupDatasets com modo: " + dsMode);
				return dsMode;
			}
			// Fallback para SetupItems (compatibilidade)
			if (traderSetupItems && traderSetupItems.Contains(normalizedDatasetID))
			{
				int dsModeFromItems = traderSetupItems.Get(normalizedDatasetID);
				Print("[AskalTraderValidation] ✅ PRIORIDADE 3 (DATASET): Dataset '" + normalizedDatasetID + "' encontrado em SetupItems com modo: " + dsModeFromItems);
				return dsModeFromItems;
			}
		}
		
		// PRIORIDADE 4: ALL fallback
		if (traderSetupItems && traderSetupItems.Contains("ALL"))
		{
			int allMode = traderSetupItems.Get("ALL");
			Print("[AskalTraderValidation] ✅ PRIORIDADE 4 (ALL): 'ALL' encontrado em SetupItems com modo: " + allMode);
			return allMode;
		}
		
		// Sem configuração encontrada
		return -1;
	}
	
	// Obter modo de um item (respeitando hierarquia: Item > Category > Dataset > ALL)
	// Wrapper para compatibilidade - usa ResolveModeForItem internamente
	static int GetItemMode(map<string, int> setupItems, string datasetID, string categoryID, string itemClassName)
	{
		return ResolveModeForItem(itemClassName, categoryID, datasetID, 3, setupItems, NULL, NULL);
	}
	
	// Resolver dataset e categoria de um item (busca no database)
	static void ResolveDatasetAndCategoryForClass(string itemClassName, out string datasetId, out string categoryId)
	{
		datasetId = "";
		categoryId = "";
		
		if (!itemClassName || itemClassName == "")
			return;
		
		// Buscar no database do servidor
		ItemData itemData = AskalDatabase.GetItemCaseInsensitive(itemClassName);
		if (!itemData)
		{
			Print("[AskalTraderValidation] ⚠️ Item não encontrado no database: " + itemClassName);
			return;
		}
		
		// Buscar em todos os datasets e categorias para encontrar onde o item está
		array<string> datasetIDs = AskalDatabase.GetAllDatasetIDs();
		if (!datasetIDs)
			return;
		
		for (int d = 0; d < datasetIDs.Count(); d++)
		{
			string dsID = datasetIDs.Get(d);
			Dataset dataset = AskalDatabase.GetDataset(dsID);
			if (!dataset || !dataset.Categories)
				continue;
			
			// Buscar em cada categoria do dataset (Categories é um map)
			for (int c = 0; c < dataset.Categories.Count(); c++)
			{
				string catID = dataset.Categories.GetKey(c);
				AskalCategory category = dataset.Categories.GetElement(c);
				if (!category || !category.Items)
					continue;
				
				// Verificar se o item está nesta categoria (case-insensitive)
				if (category.Items.Contains(itemClassName))
				{
					datasetId = dsID;
					categoryId = catID;
					return;
				}
				
				// Busca case-insensitive
				for (int i = 0; i < category.Items.Count(); i++)
				{
					string key = category.Items.GetKey(i);
					string keyLower = key;
					keyLower.ToLower();
					string itemLower = itemClassName;
					itemLower.ToLower();
					
					if (keyLower == itemLower)
					{
						datasetId = dsID;
						categoryId = catID;
						return;
					}
				}
			}
		}
		
		Print("[AskalTraderValidation] ⚠️ Dataset/Categoria não encontrados para item: " + itemClassName);
	}
	
	// Verificar se item pode ser comprado
	static bool CanBuyItem(string traderName, string itemClassName, string datasetID = "", string categoryID = "")
	{
		if (!traderName || traderName == "")
		{
			// Se não há trader, permitir (compatibilidade com VirtualStore antigo)
			return true;
		}
		
		// Carregar config do trader (buscar por TraderName, não por fileName)
		AskalTraderConfig config = AskalTraderConfig.LoadByTraderName(traderName);
		if (!config || !config.SetupItems)
		{
			Print("[AskalTraderValidation] ⚠️ Trader não encontrado ou sem SetupItems: " + traderName);
			return false; // Se trader existe mas não tem config, bloquear por segurança
		}
		
		// Se dataset/category não foram fornecidos, tentar resolver
		if (datasetID == "" || categoryID == "")
		{
			Print("[AskalTraderValidation] 🔍 Resolvendo dataset/categoria para: " + itemClassName);
			ResolveDatasetAndCategoryForClass(itemClassName, datasetID, categoryID);
			Print("[AskalTraderValidation] 🔍 Resolvido - Dataset: " + datasetID + " | Categoria: " + categoryID);
		}
		
		// Obter modo do item
		int itemMode = GetItemMode(config.SetupItems, datasetID, categoryID, itemClassName);
		Print("[AskalTraderValidation] 🔍 Modo do item: " + itemMode + " (DS: " + datasetID + ", CAT: " + categoryID + ", Item: " + itemClassName + ")");
		
		// Modo 1 (Buy Only) ou 3 (Buy + Sell) permitem compra
		bool canBuy = (itemMode == 1 || itemMode == 3);
		
		if (!canBuy)
		{
			Print("[AskalTraderValidation] ❌ Item não pode ser comprado: " + itemClassName + " (modo: " + itemMode + ") no trader: " + traderName);
		}
		else
		{
			Print("[AskalTraderValidation] ✅ Item pode ser comprado: " + itemClassName + " (modo: " + itemMode + ")");
		}
		
		return canBuy;
	}
	
	// Verificar se item pode ser vendido
	static bool CanSellItem(string traderName, string itemClassName, string datasetID = "", string categoryID = "")
	{
		if (!traderName || traderName == "")
		{
			// Se não há trader, permitir (compatibilidade com VirtualStore antigo)
			return true;
		}
		
		// Carregar config do trader (buscar por TraderName, não por fileName)
		AskalTraderConfig config = AskalTraderConfig.LoadByTraderName(traderName);
		if (!config || !config.SetupItems)
		{
			Print("[AskalTraderValidation] ⚠️ Trader não encontrado ou sem SetupItems: " + traderName);
			return false; // Se trader existe mas não tem config, bloquear por segurança
		}
		
		// Se dataset/category não foram fornecidos, tentar resolver
		if (datasetID == "" || categoryID == "")
		{
			Print("[AskalTraderValidation] 🔍 Resolvendo dataset/categoria para: " + itemClassName);
			ResolveDatasetAndCategoryForClass(itemClassName, datasetID, categoryID);
			Print("[AskalTraderValidation] 🔍 Resolvido - Dataset: " + datasetID + " | Categoria: " + categoryID);
		}
		
		// Obter modo do item
		int itemMode = GetItemMode(config.SetupItems, datasetID, categoryID, itemClassName);
		Print("[AskalTraderValidation] 🔍 Modo do item: " + itemMode + " (DS: " + datasetID + ", CAT: " + categoryID + ", Item: " + itemClassName + ")");
		
		// Modo 2 (Sell Only) ou 3 (Buy + Sell) permitem venda
		bool canSell = (itemMode == 2 || itemMode == 3);
		
		if (!canSell)
		{
			Print("[AskalTraderValidation] ❌ Item não pode ser vendido: " + itemClassName + " (modo: " + itemMode + ") no trader: " + traderName);
		}
		else
		{
			Print("[AskalTraderValidation] ✅ Item pode ser vendido: " + itemClassName + " (modo: " + itemMode + ")");
		}
		
		return canSell;
	}
}

