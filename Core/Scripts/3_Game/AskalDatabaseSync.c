// ==========================================
// SISTEMA DE SINCRONIZAÇÃO RPC OTIMIZADO
// Envia apenas dados ESSENCIAIS para evitar corrupção
// DisplayName é obtido no cliente via ConfigGetText
// ==========================================

class AskalDatabaseSync
{
	private static bool s_ClientSynced = false;
	private static int s_ExpectedCategories = 0;
	private static int s_ReceivedCategories = 0;
	private static ref map<string, ref AskalDatasetSyncData> s_BuildingDatasets = new map<string, ref AskalDatasetSyncData>();
	private static bool s_ServerWarnTextLoaded = false;
	private static string s_ServerWarnText = "";
	private static bool s_ClientWarnTextLoaded = false;
	private static string s_ClientWarnText = "";
	
	// ========================================
	// ESTADO E CONTROLE
	// ========================================
	
	static bool IsClientSynced()
	{
		return s_ClientSynced && AskalDatabaseClientCache.GetInstance().IsSynced();
	}
	
	static void MarkClientSynced()
	{
		s_ClientSynced = true;
		Print("[AskalSync] ✅ Cliente marcado como sincronizado");
	}
	
	static void ResetSyncState()
	{
		s_ClientSynced = false;
		s_ExpectedCategories = 0;
		s_ReceivedCategories = 0;
		s_BuildingDatasets.Clear();
		s_ClientWarnTextLoaded = false;
		s_ClientWarnText = "";
		Print("[AskalSync] 🔄 Estado de sincronização resetado");
	}

	static string GetServerWarnText()
	{
		if (s_ServerWarnTextLoaded)
			return s_ServerWarnText;

		array<string> candidatePaths = new array<string>();
		candidatePaths.Insert("$profile:Askal/Market/MarketConfig.json");
		candidatePaths.Insert("$profile:Askal\\Market\\MarketConfig.json");
		candidatePaths.Insert("$profile:config/Askal/Market/MarketConfig.json");
		candidatePaths.Insert("$profile:config\\Askal\\Market\\MarketConfig.json");
		candidatePaths.Insert("$mission:Askal/Market/MarketConfig.json");
		candidatePaths.Insert("$mission:Askal\\Market\\MarketConfig.json");
		candidatePaths.Insert("Askal/Market/MarketConfig.json");
		candidatePaths.Insert("Askal\\Market\\MarketConfig.json");
		candidatePaths.Insert("config/Askal/Market/MarketConfig.json");
		candidatePaths.Insert("config\\Askal\\Market\\MarketConfig.json");

		foreach (string path : candidatePaths)
		{
			AskalMarketConfig config = new AskalMarketConfig();
			if (AskalMarketLoader.LoadConfig(path, config))
			{
				if (config && config.WarnText && config.WarnText != "")
				{
					s_ServerWarnText = config.WarnText;
					Print("[AskalSync] ⚙️ WarnText carregado de: " + path);
					break;
				}
			}
		}

		s_ServerWarnTextLoaded = true;
		return s_ServerWarnText;
	}

	static void SetClientWarnText(string text)
	{
		s_ClientWarnText = text;
		s_ClientWarnTextLoaded = true;
	}

	static bool IsClientWarnTextLoaded()
	{
		return s_ClientWarnTextLoaded;
	}

	static string GetClientWarnText()
	{
		return s_ClientWarnText;
	}
	
	// ========================================
	// SERVIDOR: ENVIO DE DADOS
	// ========================================
	
	static void SendAllDatasetsToClient(PlayerIdentity identity)
	{
		if (!GetGame().IsServer())
		{
			Print("[AskalSync] ❌ ERROR: SendAllDatasetsToClient chamado no cliente!");
			return;
		}
		
		if (!identity)
		{
			Print("[AskalSync] ❌ ERROR: PlayerIdentity NULL!");
			return;
		}
		
		Print("[AskalSync] ========================================");
		Print("[AskalSync] 🚀 Iniciando sincronização para: " + identity.GetName());
		
		array<string> datasetIDs = AskalDatabase.GetAllDatasetIDs();
		
		if (!datasetIDs || datasetIDs.Count() == 0)
		{
			Print("[AskalSync] ⚠️ Nenhum dataset em memória!");
			SendSyncComplete(identity, 0);
			return;
		}
		
		Print("[AskalSync] ✅ Encontrados " + datasetIDs.Count() + " datasets");
		
		// Preparar dados de sincronização
		array<ref AskalDatasetSyncData> allDatasets = new array<ref AskalDatasetSyncData>();
		int totalCategories = 0;
		
		foreach (string datasetID : datasetIDs)
		{
			Dataset sourceDataset = AskalDatabase.GetDataset(datasetID);
			if (!sourceDataset)
			{
				Print("[AskalSync] ⚠️ Dataset NULL: " + datasetID);
				continue;
			}
			
			AskalDatasetSyncData syncDataset = ConvertDatasetToSync(sourceDataset);
			if (syncDataset && syncDataset.Categories)
			{
				allDatasets.Insert(syncDataset);
				totalCategories += syncDataset.Categories.Count();
				Print("[AskalSync] 📦 Dataset preparado: " + datasetID + " (" + syncDataset.Categories.Count() + " categorias)");
			}
		}
		
		if (allDatasets.Count() == 0)
		{
			Print("[AskalSync] ⚠️ Nenhum dataset válido para enviar!");
			SendSyncComplete(identity, 0);
			return;
		}
		
		Print("[AskalSync] 📤 Enviando " + allDatasets.Count() + " datasets (" + totalCategories + " categorias)...");
		
		int sentCount = 0;
		foreach (AskalDatasetSyncData dataset : allDatasets)
		{
			if (!dataset || !dataset.Categories) continue;
			
			SendDatasetHeader(identity, dataset);
			
			array<string> orderedCategories = new array<string>();
			if (dataset.CategoryOrder && dataset.CategoryOrder.Count() > 0)
			{
				for (int ordIdx = 0; ordIdx < dataset.CategoryOrder.Count(); ordIdx++)
				{
					string orderedId = dataset.CategoryOrder.Get(ordIdx);
					if (orderedId && dataset.Categories.Contains(orderedId))
						orderedCategories.Insert(orderedId);
				}
			}
			
			// Fallback para ordem do map caso CategoryOrder esteja vazia ou inconsistente
			if (orderedCategories.Count() == 0)
			{
				for (int fallbackIdx = 0; fallbackIdx < dataset.Categories.Count(); fallbackIdx++)
				{
					string fallbackId = dataset.Categories.GetKey(fallbackIdx);
					if (fallbackId)
						orderedCategories.Insert(fallbackId);
				}
			}
			
			foreach (string catID : orderedCategories)
			{
				AskalCategorySyncData sendCat = dataset.Categories.Get(catID);
				if (!sendCat) continue;
				
				if (SendCategoryOptimized(identity, dataset.DatasetID, sendCat))
				{
					sentCount++;
				}
				else
				{
					Print("[AskalSync] ⚠️ Falha ao enviar categoria: " + catID);
				}
			}
		}
		
		SendSyncComplete(identity, sentCount);
		
		Print("[AskalSync] ✅ Sincronização completa! Enviadas " + sentCount + " categorias");
		Print("[AskalSync] ========================================");
	}
	
	// Converte Dataset para formato de sync
	static AskalDatasetSyncData ConvertDatasetToSync(Dataset sourceDataset)
	{
		if (!sourceDataset) return NULL;
		
		AskalDatasetSyncData syncDataset = new AskalDatasetSyncData();
		syncDataset.DatasetID = sourceDataset.DatasetID;
		syncDataset.DisplayName = sourceDataset.DisplayName;
		syncDataset.Description = "";
		syncDataset.Icon = sourceDataset.Icon;
		if (!syncDataset.Icon || syncDataset.Icon == "")
			syncDataset.Icon = "set:dayz_inventory image:missing";
		
		if (!sourceDataset.Categories) return syncDataset;
		syncDataset.CategoryOrder = new array<string>();
		if (sourceDataset.CategoryOrder)
		{
			for (int orderIdx = 0; orderIdx < sourceDataset.CategoryOrder.Count(); orderIdx++)
			{
				string orderCategoryID = sourceDataset.CategoryOrder.Get(orderIdx);
				if (orderCategoryID && orderCategoryID != "")
					syncDataset.CategoryOrder.Insert(orderCategoryID);
			}
		}
		
		for (int catIdx = 0; catIdx < sourceDataset.Categories.Count(); catIdx++)
		{
			string catID = sourceDataset.Categories.GetKey(catIdx);
			AskalCategory sourceCategory = sourceDataset.Categories.GetElement(catIdx);
			
			if (!sourceCategory) continue;
			
			AskalCategorySyncData syncCat = ConvertCategoryToSync(catID, sourceCategory);
			if (syncCat)
			{
				syncDataset.Categories.Set(catID, syncCat);
			}
		}
		
		return syncDataset;
	}
	
	// Converte Category para formato de sync (SEM DisplayName de itens)
	static AskalCategorySyncData ConvertCategoryToSync(string catID, AskalCategory sourceCat)
	{
		if (!sourceCat) return NULL;
		
		AskalCategorySyncData syncCat = new AskalCategorySyncData();
		syncCat.CategoryID = catID;
		syncCat.DisplayName = sourceCat.DisplayName;
		syncCat.BasePrice = sourceCat.BasePrice;
		if (sourceCat.SellPercent > 0)
			syncCat.SellPercent = sourceCat.SellPercent;
		else
			syncCat.SellPercent = AskalMarketDefaults.DEFAULT_SELL_PERCENT;
		
		if (!sourceCat.Items) return syncCat;
		
		for (int itemIdx = 0; itemIdx < sourceCat.Items.Count(); itemIdx++)
		{
			string itemClassName = sourceCat.Items.GetKey(itemIdx);
			ItemData itemData = sourceCat.Items.GetElement(itemIdx);
			
			if (!itemData) continue;
			
			// Criar item sync SEM DisplayName (será obtido no cliente)
			AskalItemSyncData syncItem = new AskalItemSyncData();
			syncItem.ClassName = itemClassName;
			syncItem.DisplayName = ""; // VAZIO - será obtido no cliente
			syncItem.BasePrice = itemData.Price;
			if (syncItem.BasePrice <= 0)
				syncItem.BasePrice = AskalMarketDefaults.DEFAULT_BUY_PRICE;
			if (itemData.SellPercent > 0)
				syncItem.SellPercent = itemData.SellPercent;
			else
				syncItem.SellPercent = syncCat.SellPercent;
			
			// Variantes apenas se houver
			if (itemData.variants && itemData.variants.Count() > 0)
			{
				for (int varLoopIdx = 0; varLoopIdx < itemData.variants.Count(); varLoopIdx++)
				{
					string varItem = itemData.variants.Get(varLoopIdx);
					syncItem.Variants.Insert(varItem);
				}
			}

			if (itemData.attachments && itemData.attachments.Count() > 0)
			{
				for (int attIdx = 0; attIdx < itemData.attachments.Count(); attIdx++)
				{
					string attachmentClass = itemData.attachments.Get(attIdx);
					if (attachmentClass && attachmentClass != "")
						syncItem.Attachments.Insert(attachmentClass);
				}
			}
			
			syncCat.Items.Set(itemClassName, syncItem);
		}
		
		return syncCat;
	}
	
	// Envia header de dataset
	static void SendDatasetHeader(PlayerIdentity identity, AskalDatasetSyncData dataset)
	{
		if (!identity || !dataset) return;
		
		string dsID = dataset.DatasetID;
		string dsName = dataset.DisplayName;
		int catCount = 0;
		if (dataset.Categories)
			catCount = dataset.Categories.Count();
		
		string iconPath = dataset.Icon;
		if (!iconPath || iconPath == "")
			iconPath = "set:dayz_inventory image:missing";

		Param4<string, string, int, string> header = new Param4<string, string, int, string>(dsID, dsName, catCount, iconPath);
		GetRPCManager().SendRPC("AskalCoreModule", "SendDatasetHeader", header, true, identity, NULL);
		
		Print("[AskalSync] 📤 Header enviado: " + dsID + " (" + catCount + " categorias)");
	}
	
	// Envia categoria OTIMIZADA (divide automaticamente em batches de 5-7 itens)
	static bool SendCategoryOptimized(PlayerIdentity identity, string dsID, AskalCategorySyncData syncCat)
	{
		if (!identity || !syncCat || !syncCat.Items) return false;
		
		int totalItems = syncCat.Items.Count();
		if (totalItems == 0)
		{
			// Categoria vazia - enviar apenas metadados
			array<string> emptyBatch = new array<string>();
			return SendCategoryBatch(identity, dsID, syncCat, emptyBatch, 0, 1);
		}
		
		// Preparar lista de classnames
		array<string> itemClassNames = new array<string>();
		for (int nameIdx = 0; nameIdx < syncCat.Items.Count(); nameIdx++)
		{
			itemClassNames.Insert(syncCat.Items.GetKey(nameIdx));
		}
		
		// Dividir em batches de 3 itens (número muito seguro para evitar corrupção de string)
		int itemsPerBatch = 3;
		int totalBatches = (totalItems + itemsPerBatch - 1) / itemsPerBatch;
		int sentBatches = 0;
		
		for (int batchIdx = 0; batchIdx < totalBatches; batchIdx++)
		{
			int startIdx = batchIdx * itemsPerBatch;
			int endIdx = startIdx + itemsPerBatch;
			if (endIdx > totalItems) endIdx = totalItems;
			
			// Criar batch com subconjunto de itens
			array<string> batchItems = new array<string>();
			for (int copyIdx = startIdx; copyIdx < endIdx; copyIdx++)
			{
				batchItems.Insert(itemClassNames.Get(copyIdx));
			}
			
			if (SendCategoryBatch(identity, dsID, syncCat, batchItems, batchIdx, totalBatches))
			{
				sentBatches++;
			}
		}
		
		if (totalBatches > 1)
		{
			Print("[AskalSync] ✅ Categoria dividida: " + syncCat.CategoryID + " (" + sentBatches + " batches, " + totalItems + " items)");
		}
		
		return sentBatches > 0;
	}
	
	// Envia um batch de itens de uma categoria (formato compacto: apenas ClassName + Price)
	static bool SendCategoryBatch(PlayerIdentity identity, string dsID, AskalCategorySyncData syncCat, array<string> itemClassNames, int batchIdx, int totalBatches)
	{
		if (!identity || !syncCat) return false;
		
		// Criar estrutura mínima: apenas ClassName e Price (sem DisplayName)
		AskalCategoryBatchData batchData = new AskalCategoryBatchData();
		batchData.DatasetID = dsID;
		batchData.CategoryID = syncCat.CategoryID;
		batchData.DisplayName = syncCat.DisplayName;
		batchData.BasePrice = syncCat.BasePrice;
		batchData.CategorySellPercent = syncCat.SellPercent;
		batchData.BatchIndex = batchIdx;
		batchData.TotalBatches = totalBatches;
		batchData.ItemClassNames = new array<string>();
		batchData.ItemPrices = new array<int>();
		batchData.ItemVariants = new array<string>();
		batchData.ItemAttachments = new array<string>();
		batchData.ItemSellPercents = new array<int>();

		// Processar itens do batch
		if (itemClassNames && itemClassNames.Count() > 0)
		{
			for (int processIdx = 0; processIdx < itemClassNames.Count(); processIdx++)
			{
				string className = itemClassNames.Get(processIdx);
				AskalItemSyncData itemData = syncCat.Items.Get(className);
				
				if (itemData)
				{
					batchData.ItemClassNames.Insert(className);
					batchData.ItemPrices.Insert(itemData.BasePrice);
					batchData.ItemSellPercents.Insert(itemData.SellPercent);
					
					// Variantes como string concatenada (compacto)
					string variantsStr = "";
					if (itemData.Variants && itemData.Variants.Count() > 0)
					{
						for (int varIdx = 0; varIdx < itemData.Variants.Count(); varIdx++)
						{
							if (varIdx > 0) variantsStr += ",";
							variantsStr += itemData.Variants.Get(varIdx);
						}
					}
					batchData.ItemVariants.Insert(variantsStr);

					string attachmentsStr = "";
					if (itemData.Attachments && itemData.Attachments.Count() > 0)
					{
						for (int attIdx = 0; attIdx < itemData.Attachments.Count(); attIdx++)
						{
							if (attIdx > 0) attachmentsStr += ",";
							attachmentsStr += itemData.Attachments.Get(attIdx);
						}
					}
					batchData.ItemAttachments.Insert(attachmentsStr);
				}
			}
		}
		
		// Serializar
		string jsonData = AskalJsonLoader<AskalCategoryBatchData>.ObjectToString(batchData);
		
		if (!jsonData || jsonData == "")
		{
			Print("[AskalSync] ❌ Erro ao serializar batch");
			return false;
		}
		
		// Verificar tamanho (deve ser < 1KB para segurança máxima)
		int sizeBytes = jsonData.Length();
		if (sizeBytes > 1000)
		{
			Print("[AskalSync] [AVISO] Batch muito grande: " + sizeBytes + " bytes (limite: 1000)");
			// Tentar reduzir ainda mais o batch
			return false;
		}
		
		// Enviar via RPC
		Param1<string> params = new Param1<string>(jsonData);
		GetRPCManager().SendRPC("AskalCoreModule", "SendCategoryBatch", params, true, identity, NULL);
		
		Print("[AskalSync] 📤 Batch enviado: " + syncCat.CategoryID + " [" + (batchData.BatchIndex + 1) + "/" + batchData.TotalBatches + "] (" + batchData.ItemClassNames.Count() + " items, " + sizeBytes + " bytes)");
		return true;
	}
	
	// Envia conclusão
	static void SendSyncComplete(PlayerIdentity identity, int totalCategories)
	{
		if (!identity) return;
		
		Print("[AskalSync] 📤 Enviando conclusão (" + totalCategories + " categorias)...");
		string warnText = GetServerWarnText();
		Param2<int, string> completeParams = new Param2<int, string>(totalCategories, warnText);
		GetRPCManager().SendRPC("AskalCoreModule", "SendDatasetsComplete", completeParams, true, identity, NULL);
	}
	
	// ========================================
	// CLIENTE: RECEPÇÃO DE DADOS
	// ========================================
	
	// Recebe header de dataset
    static void RPC_ReceiveDatasetHeader(string dsID, string dsName, int catCount, string iconPath)
	{
		if (!GetGame().IsClient()) return;
		
		Print("[AskalSync] 📥 Header recebido: " + dsID + " (" + catCount + " categorias esperadas)");
		
		if (!s_BuildingDatasets.Contains(dsID))
		{
			AskalDatasetSyncData dataset = new AskalDatasetSyncData();
			dataset.DatasetID = dsID;
			dataset.DisplayName = dsName;
			dataset.Description = "";
			dataset.Icon = iconPath;
			if (!dataset.Icon || dataset.Icon == "")
				dataset.Icon = "set:dayz_inventory image:missing";
			dataset.Categories = new map<string, ref AskalCategorySyncData>();
			s_BuildingDatasets.Set(dsID, dataset);
		}
		else
		{
			AskalDatasetSyncData existing = s_BuildingDatasets.Get(dsID);
			if (existing)
			{
				existing.DisplayName = dsName;
				if (iconPath && iconPath != "")
					existing.Icon = iconPath;
				else if (!existing.Icon || existing.Icon == "")
					existing.Icon = "set:dayz_inventory image:missing";
				existing.CategoryOrder = new array<string>();
			}
		}
		
		s_ExpectedCategories += catCount;
	}
	
	// Recebe batch de categoria (dados compactos)
	static void RPC_ReceiveCategoryBatch(string jsonData)
	{
		if (!GetGame().IsClient()) return;
		
		if (!jsonData || jsonData == "")
		{
			Print("[AskalSync] ⚠️ JSON vazio recebido");
			return;
		}
		
		AskalCategoryBatchData batchData = new AskalCategoryBatchData();
		if (!AskalJsonLoader<AskalCategoryBatchData>.StringToObject(jsonData, batchData))
		{
			Print("[AskalSync] ❌ Erro ao deserializar batch!");
			return;
		}
		
		if (!batchData || !batchData.DatasetID || !batchData.CategoryID)
		{
			Print("[AskalSync] ⚠️ Batch inválido");
			return;
		}
		
		// Criar chave única
		string catKey = batchData.DatasetID + "::" + batchData.CategoryID;
		
		// Obter ou criar dataset
		if (!s_BuildingDatasets.Contains(batchData.DatasetID))
		{
			AskalDatasetSyncData newDs = new AskalDatasetSyncData();
			newDs.DatasetID = batchData.DatasetID;
			newDs.DisplayName = batchData.DatasetID;
			newDs.Description = "";
			newDs.Icon = "set:dayz_inventory image:missing";
			newDs.Categories = new map<string, ref AskalCategorySyncData>();
			s_BuildingDatasets.Set(batchData.DatasetID, newDs);
		}
		
		AskalDatasetSyncData dataset = s_BuildingDatasets.Get(batchData.DatasetID);
		
		// Obter ou criar categoria
		AskalCategorySyncData batchCategory = null;
		if (dataset.Categories.Contains(batchData.CategoryID))
		{
			batchCategory = dataset.Categories.Get(batchData.CategoryID);
		}
		else
		{
			batchCategory = new AskalCategorySyncData();
			batchCategory.CategoryID = batchData.CategoryID;
			batchCategory.DisplayName = batchData.DisplayName;
			batchCategory.BasePrice = batchData.BasePrice;
			if (batchData.CategorySellPercent > 0)
				batchCategory.SellPercent = batchData.CategorySellPercent;
			else
				batchCategory.SellPercent = AskalMarketDefaults.DEFAULT_SELL_PERCENT;
			batchCategory.Items = new map<string, ref AskalItemSyncData>();
			dataset.Categories.Set(batchData.CategoryID, batchCategory);
			if (!dataset.CategoryOrder)
				dataset.CategoryOrder = new array<string>();
			if (dataset.CategoryOrder.Find(batchData.CategoryID) == -1)
				dataset.CategoryOrder.Insert(batchData.CategoryID);
		}
		
		if (batchCategory)
		{
			if (batchData.CategorySellPercent > 0)
				batchCategory.SellPercent = batchData.CategorySellPercent;
			else if (batchCategory.SellPercent <= 0)
				batchCategory.SellPercent = AskalMarketDefaults.DEFAULT_SELL_PERCENT;
		}
		
		// Processar itens do batch
		if (batchData.ItemClassNames && batchData.ItemPrices)
		{
			for (int itemIdx = 0; itemIdx < batchData.ItemClassNames.Count(); itemIdx++)
			{
				string className = batchData.ItemClassNames.Get(itemIdx);
				int price = batchData.ItemPrices.Get(itemIdx);
				int itemSellPercent = AskalMarketDefaults.DEFAULT_SELL_PERCENT;
				if (batchData.ItemSellPercents && itemIdx < batchData.ItemSellPercents.Count())
					itemSellPercent = batchData.ItemSellPercents.Get(itemIdx);
				if (itemSellPercent <= 0 && batchCategory)
					itemSellPercent = batchCategory.SellPercent;
				if (itemSellPercent <= 0)
					itemSellPercent = AskalMarketDefaults.DEFAULT_SELL_PERCENT;
				
				if (price <= 0)
				{
					if (batchCategory && batchCategory.BasePrice > 0)
						price = batchCategory.BasePrice;
					else
						price = AskalMarketDefaults.DEFAULT_BUY_PRICE;
				}
				
				// Obter DisplayName no cliente (localmente)
				string displayName = "";
				GetGame().ConfigGetText("CfgVehicles " + className + " displayName", displayName);
				if (displayName == "")
					GetGame().ConfigGetText("CfgWeapons " + className + " displayName", displayName);
				// Para munições e carregadores, verificar CfgMagazines e CfgAmmo
				if (displayName == "")
					GetGame().ConfigGetText("CfgMagazines " + className + " displayName", displayName);
				if (displayName == "")
					GetGame().ConfigGetText("CfgAmmo " + className + " displayName", displayName);
				if (displayName == "")
					displayName = className;
				
				// Criar item sync
				AskalItemSyncData syncItem = new AskalItemSyncData();
				syncItem.ClassName = className;
				syncItem.DisplayName = displayName;
				syncItem.BasePrice = price;
				if (syncItem.BasePrice <= 0)
					syncItem.BasePrice = AskalMarketDefaults.DEFAULT_BUY_PRICE;
				syncItem.SellPercent = itemSellPercent;
				
				// Processar variantes (string separada por vírgula)
				if (batchData.ItemVariants && itemIdx < batchData.ItemVariants.Count())
				{
					string variantsStr = batchData.ItemVariants.Get(itemIdx);
					if (variantsStr != "" && variantsStr != "0")
					{
						// Parse manual da string (ex: "item1,item2,item3")
						int variantStartPos = 0;
						string parseVariant = "";
						while (variantStartPos < variantsStr.Length())
						{
							int variantCommaPos = variantsStr.IndexOfFrom(variantStartPos, ",");
							if (variantCommaPos == -1)
							{
								// Último item
								parseVariant = variantsStr.Substring(variantStartPos, variantsStr.Length() - variantStartPos);
								if (parseVariant != "")
									syncItem.Variants.Insert(parseVariant);
								break;
							}
							else
							{
								parseVariant = variantsStr.Substring(variantStartPos, variantCommaPos - variantStartPos);
								if (parseVariant != "")
									syncItem.Variants.Insert(parseVariant);
								variantStartPos = variantCommaPos + 1;
							}
						}
					}
				}
				
				// Processar attachments (string separada por vírgula)
				if (batchData.ItemAttachments && itemIdx < batchData.ItemAttachments.Count())
				{
					string attachmentsStr = batchData.ItemAttachments.Get(itemIdx);
					if (attachmentsStr != "" && attachmentsStr != "0")
					{
						// Parse manual da string (ex: "attachment1,attachment2,attachment3")
						int attachmentStartPos = 0;
						string parseAttachment = "";
						while (attachmentStartPos < attachmentsStr.Length())
						{
							int attachmentCommaPos = attachmentsStr.IndexOfFrom(attachmentStartPos, ",");
							if (attachmentCommaPos == -1)
							{
								// Último item
								parseAttachment = attachmentsStr.Substring(attachmentStartPos, attachmentsStr.Length() - attachmentStartPos);
								if (parseAttachment != "")
									syncItem.Attachments.Insert(parseAttachment);
								break;
							}
							else
							{
								parseAttachment = attachmentsStr.Substring(attachmentStartPos, attachmentCommaPos - attachmentStartPos);
								if (parseAttachment != "")
									syncItem.Attachments.Insert(parseAttachment);
								attachmentStartPos = attachmentCommaPos + 1;
							}
						}
					}
				}
				
				batchCategory.Items.Set(className, syncItem);
			}
		}
		
		// Se último batch, marcar categoria como completa
		if (batchData.BatchIndex + 1 >= batchData.TotalBatches)
		{
			s_ReceivedCategories++;
			Print("[AskalSync] ✅ Categoria completa: " + batchData.CategoryID + " (" + batchCategory.Items.Count() + " items) [" + s_ReceivedCategories + "/" + s_ExpectedCategories + "]");
		}
		else
		{
			Print("[AskalSync] 📦 Batch recebido: " + batchData.CategoryID + " [" + (batchData.BatchIndex + 1) + "/" + batchData.TotalBatches + "]");
		}
	}
	
	// Recebe conclusão
	static void RPC_ReceiveDatasetsComplete(int totalCategories, string warnText)
	{
		if (!GetGame().IsClient()) return;
		
		Print("[AskalSync] ========================================");
		Print("[AskalSync] 📥 Sinal de conclusão recebido");
		Print("[AskalSync] Esperado: " + s_ExpectedCategories + " | Recebido: " + s_ReceivedCategories + " | Servidor: " + totalCategories);
		SetClientWarnText(warnText);
		
		if (s_ReceivedCategories != s_ExpectedCategories)
		{
			Print("[AskalSync] ⚠️ Contagem não confere!");
		}
		
		// Mover para cache permanente
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		int dsCount = 0;
		int totalItems = 0;
		
		for (int dsIdx = 0; dsIdx < s_BuildingDatasets.Count(); dsIdx++)
		{
			string dsID = s_BuildingDatasets.GetKey(dsIdx);
			AskalDatasetSyncData ds = s_BuildingDatasets.GetElement(dsIdx);
			
			if (ds && ds.Categories && ds.Categories.Count() > 0)
			{
				if (ValidateDataset(ds))
				{
					cache.AddDataset(ds);
					dsCount++;
					
					for (int catIdx = 0; catIdx < ds.Categories.Count(); catIdx++)
					{
						AskalCategorySyncData countCat = ds.Categories.GetElement(catIdx);
						if (countCat && countCat.Items)
							totalItems += countCat.Items.Count();
					}
					
					Print("[AskalSync] ✅ Dataset: " + dsID + " (" + ds.Categories.Count() + " categorias)");
				}
			}
		}
		
		s_BuildingDatasets.Clear();
		
		Print("[AskalSync] ✅ SINCRONIZAÇÃO COMPLETA!");
		Print("[AskalSync] Datasets: " + dsCount + " | Categorias: " + s_ReceivedCategories + " | Itens: " + totalItems);
		Print("[AskalSync] ========================================");
		
		cache.SetSynced(true);
		MarkClientSynced();
		cache.PrintCache();
	}
	
	// Valida dataset
	static bool ValidateDataset(AskalDatasetSyncData ds)
	{
		if (!ds) return false;
		if (!ds.DatasetID || ds.DatasetID == "") return false;
		if (!ds.Categories || ds.Categories.Count() == 0) return false;
		
		for (int valIdx = 0; valIdx < ds.Categories.Count(); valIdx++)
		{
			AskalCategorySyncData valCat = ds.Categories.GetElement(valIdx);
			if (!valCat || !valCat.CategoryID || !valCat.Items) return false;
		}
		
		return true;
	}
}
