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
	
	// Obter modo de um item (respeitando hierarquia: Item > Category > Dataset > ALL)
	static int GetItemMode(map<string, int> setupItems, string datasetID, string categoryID, string itemClassName)
	{
		if (!setupItems || setupItems.Count() == 0)
			return 3; // Sem filtros, tudo disponível
		
		// Normalizar IDs
		string normalizedDatasetID = NormalizeDatasetID(datasetID);
		string normalizedCategoryID = NormalizeCategoryID(categoryID);
		
		// PRIORIDADE 1: Verificar item específico (className exato)
		if (setupItems.Contains(itemClassName))
		{
			return setupItems.Get(itemClassName);
		}
		
		// PRIORIDADE 2: Verificar categoria (CAT_*)
		int categoryMode = GetCategoryMode(setupItems, normalizedDatasetID, normalizedCategoryID);
		if (categoryMode >= 0)
		{
			return categoryMode;
		}
		
		// PRIORIDADE 3: Verificar dataset (DS_*)
		int datasetMode = GetDatasetMode(setupItems, normalizedDatasetID);
		if (datasetMode >= 0)
		{
			return datasetMode;
		}
		
		// PRIORIDADE 4: Verificar "ALL"
		if (setupItems.Contains("ALL"))
		{
			return setupItems.Get("ALL");
		}
		
		// Sem configuração encontrada
		return -1;
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

