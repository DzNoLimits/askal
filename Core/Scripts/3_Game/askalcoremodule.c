// AskalCoreModule - integra com Community Framework
// Sistema simplificado de sincronização
[CF_RegisterModule(AskalCoreModule)]
class AskalCoreModule : CF_ModuleGame
{
    void AskalCoreModule()
    {
		CF_Log.Info("[AskalCore] Módulo Askal Core inicializado");
    }

    override void OnInit()
    {
        super.OnInit();
		
		Print("[AskalCore] ========================================");
		
		// Configurar caminho do database APENAS no servidor
		// Cliente recebe dados via RPC, não precisa acessar filesystem
		// Verificação segura: GetGame() pode retornar NULL durante inicialização
		if (GetGame() && GetGame().IsServer())
		{
			Print("[AskalCore] Configurando AskalDatabase (SERVIDOR)");
			
			// Configurar caminho do database
			string dbPath = "$profile:Askal/Database/Datasets/";
			AskalDatabase.SetDatabasePath(dbPath);
			
			Print("[AskalCore] Database path: " + dbPath);
			
			// Inicializar sistema de balance (apenas servidor)
			AskalPlayerBalance.Init();
		}
		else
		{
			Print("[AskalCore] Cliente inicializado (dados recebidos via RPC)");
		}
		
		// Habilitar eventos do CF
		EnableMissionStart();
		EnableMissionFinish();
		
		// Registrar RPCs (sistema otimizado - batches compactos)
		AddLegacyRPC("RequestDatasets", SingleplayerExecutionType.Server);
		AddLegacyRPC("SendDatasetHeader", SingleplayerExecutionType.Client);
		AddLegacyRPC("SendCategoryBatch", SingleplayerExecutionType.Client); // Sistema otimizado
		AddLegacyRPC("SendDatasetsComplete", SingleplayerExecutionType.Client);
		AddLegacyRPC("RequestVirtualStoreConfig", SingleplayerExecutionType.Server);
		AddLegacyRPC("VirtualStoreConfigResponse", SingleplayerExecutionType.Client);
		
		// RPCs de compra/venda (handler será registrado em 4_World para ter acesso a PlayerBase)
	AddLegacyRPC("PurchaseItemResponse", SingleplayerExecutionType.Client);
	AddLegacyRPC("SellItemResponse", SingleplayerExecutionType.Client);
	
	// RPC para health dos itens do inventário
		AddLegacyRPC("RequestInventoryHealth", SingleplayerExecutionType.Server);
		AddLegacyRPC("InventoryHealthResponse", SingleplayerExecutionType.Client);
		AddLegacyRPC("OpenTraderMenu", SingleplayerExecutionType.Client);
	
	Print("[AskalCore] ✅ RPCs registrados");
	Print("[AskalCore] ========================================");
}
	
	// RPC Handler: Cliente solicita datasets
	void RequestDatasets(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			Print("[AskalCore] ⚠️ RequestDatasets chamado fora do servidor");
			return;
		}
		
		if (!sender)
		{
			Print("[AskalCore] ❌ Sender NULL");
			return;
		}
		
		Print("[AskalCore] ========================================");
		Print("[AskalCore] 📥 Cliente solicitou sync: " + sender.GetName());
		
		int totalDatasets = AskalDatabase.GetAllDatasetIDs().Count();
		Print("[AskalCore] Datasets disponíveis: " + totalDatasets);
		
		if (totalDatasets == 0)
		{
			Print("[AskalCore] ❌ Nenhum dataset carregado!");
			Print("[AskalCore] ========================================");
			return;
		}
		
		// Enviar datasets
		AskalDatabaseSync.SendAllDatasetsToClient(sender);
		Print("[AskalCore] ========================================");
	}
	
	// RPC Handler: Cliente solicita configuração da loja virtual
	void RequestVirtualStoreConfig(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;
		
		if (!sender)
		{
			Print("[AskalCore] ❌ RequestVirtualStoreConfig sem sender");
			return;
		}
		
		AskalVirtualStoreConfig config = AskalVirtualStoreSettings.GetConfig();
		if (!config)
			config = new AskalVirtualStoreConfig();
		
		string currencyId = config.GetPrimaryCurrency();
		float buyCoeff = config.BuyCoefficient;
		float sellCoeff = config.SellCoefficient;
		
		ref array<string> setupKeys = new array<string>();
		ref array<int> setupValues = new array<int>();
		if (config.SetupItems)
		{
			for (int i = 0; i < config.SetupItems.Count(); i++)
			{
				string key = config.SetupItems.GetKey(i);
				int mode = config.SetupItems.GetElement(i);
				if (key && key != "")
				{
					setupKeys.Insert(key);
					setupValues.Insert(mode);
				}
			}
		}
		
		Param5<string, float, float, ref array<string>, ref array<int>> data = new Param5<string, float, float, ref array<string>, ref array<int>>(currencyId, buyCoeff, sellCoeff, setupKeys, setupValues);
		GetRPCManager().SendRPC("AskalCoreModule", "VirtualStoreConfigResponse", data, true, sender, NULL);
	}
	
	// RPC Handler: Cliente recebe header de dataset
	void SendDatasetHeader(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		
		Param4<string, string, int, string> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] ❌ Erro ao ler SendDatasetHeader");
			return;
		}
		
		AskalDatabaseSync.RPC_ReceiveDatasetHeader(data.param1, data.param2, data.param3, data.param4);
	}
	
	// RPC Handler: Cliente recebe batch de categoria (sistema otimizado)
	void SendCategoryBatch(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		
		Param1<string> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] [ERRO] Erro ao ler SendCategoryBatch - String corrompida");
			return;
		}
		
		string jsonData = data.param1;
		if (!jsonData || jsonData == "")
		{
			Print("[AskalCore] [ERRO] JSON vazio recebido em SendCategoryBatch");
			return;
		}
		
		// Validar tamanho da string recebida
		int dataLength = jsonData.Length();
		if (dataLength <= 0 || dataLength > 2000)
		{
			Print("[AskalCore] [ERRO] Tamanho inválido de JSON: " + dataLength + " bytes");
			return;
		}
		
		AskalDatabaseSync.RPC_ReceiveCategoryBatch(jsonData);
	}
	
	// RPC Handler: Cliente recebe sinal de conclusão
	void SendDatasetsComplete(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		
		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] ❌ Erro ao ler SendDatasetsComplete");
			return;
		}
		
		int totalCategories = data.param1;
		string warnText = data.param2;
		AskalDatabaseSync.RPC_ReceiveDatasetsComplete(totalCategories, warnText);
	}
	
	// RPC Handler: Cliente recebe configuração da loja virtual
	void VirtualStoreConfigResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		
		Param5<string, float, float, ref array<string>, ref array<int>> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] ❌ Erro ao ler VirtualStoreConfigResponse");
			return;
		}
		
		string currencyId = data.param1;
		float buyCoeff = data.param2;
		float sellCoeff = data.param3;
		array<string> setupKeys = data.param4;
		array<int> setupValues = data.param5;
		
		AskalVirtualStoreSettings.ApplyConfigFromServer(currencyId, buyCoeff, sellCoeff, setupKeys, setupValues);
	}
	
	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
		Print("[AskalCore] ========================================");
		Print("[AskalCore] OnMissionStart() → Inicializando");
		
		// Marcar database como pronto APENAS no servidor
		// Cliente recebe dados via RPC, não precisa carregar do filesystem
		// Verificação segura: GetGame() pode retornar NULL durante inicialização
		if (GetGame() && GetGame().IsServer())
		{
			AskalCoreDatabaseManager.GetInstance().LoadDatabase();
			
			// Verificar datasets
			int totalDatasets = AskalDatabase.GetAllDatasetIDs().Count();
			if (totalDatasets > 0)
			{
				Print("[AskalCore] ✅ Datasets carregados: " + totalDatasets);
			}
			else
			{
				Print("[AskalCore] ⏳ Aguardando MissionServer carregar datasets");
			}
		}
		else
		{
			Print("[AskalCore] Cliente: dados serão recebidos via RPC");
		}
		
		Print("[AskalCore] ========================================");
	}
	
	// Enviar resposta de compra para o cliente (usado pelo módulo 4_World)
	void SendPurchaseResponse(PlayerIdentity identity, bool success, string message)
	{
		if (!identity)
			return;
		
		Param2<bool, string> params = new Param2<bool, string>(success, message);
		GetRPCManager().SendRPC("AskalCoreModule", "PurchaseItemResponse", params, true, identity, NULL);
	}
	
	// RPC Handler: Cliente recebe resposta de compra
	void PurchaseItemResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		
		Param4<bool, string, string, int> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] ❌ Erro ao ler PurchaseItemResponse");
			return;
		}
		
		bool success = data.param1;
		string message = data.param2;
		string itemClass = data.param3;
		int price = data.param4;
		
		if (success)
		{
			Print("[AskalStore] ✅ " + message);
			// Adicionar notificação visual de compra via helper (3_Game pode acessar)
			if (itemClass != "" && price > 0)
			{
				AskalNotificationHelper.AddPurchaseNotification(itemClass, price);
			}
		}
		else
		{
			Print("[AskalStore] ❌ " + message);
		}
	}
	
	// RPC Handler: Cliente recebe resposta de venda
	void SellItemResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		
		Param4<bool, string, string, int> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] ❌ Erro ao ler SellItemResponse");
			return;
		}
		
		bool success = data.param1;
		string message = data.param2;
		string itemClass = data.param3;
		int price = data.param4;
		
		string statusIcon = "[ERRO]";
		if (success)
			statusIcon = "[OK]";
		Print("[AskalCore] SellItemResponse recebida: " + statusIcon + " " + message);
		
		if (success)
		{
			Print("[AskalStore] [OK] " + message);
			// Adicionar notificação visual de venda no cliente (assim como em PurchaseItemResponse)
			// O servidor tem memória separada, então a notificação precisa ser adicionada no cliente
			if (itemClass != "" && price > 0)
			{
				// Obter display name do item para a notificação
				string itemDisplayName = "";
				GetGame().ConfigGetText("CfgVehicles " + itemClass + " displayName", itemDisplayName);
				if (!itemDisplayName || itemDisplayName == "")
					GetGame().ConfigGetText("CfgMagazines " + itemClass + " displayName", itemDisplayName);
				if (!itemDisplayName || itemDisplayName == "")
					itemDisplayName = itemClass;
				
				// Remover prefixos de tradução se existirem
				if (itemDisplayName.IndexOf("$STR_") == 0)
					itemDisplayName = Widget.TranslateString(itemDisplayName);
				
				AskalNotificationHelper.AddSellNotification(itemClass, price, itemDisplayName);
				Print("[AskalCore] 📢 Notificação de venda adicionada no cliente: " + itemDisplayName + " ($" + price.ToString() + ")");
			}
		}
		else
		{
			Print("[AskalStore] [ERRO] " + message);
		}
	}
	
	// RPC Handler: Cliente recebe health dos itens do inventário
	void InventoryHealthResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		
		Param1<ref array<ref Param2<string, float>>> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] ❌ Erro ao ler InventoryHealthResponse");
			return;
		}
		
		ref array<ref Param2<string, float>> healthArray = data.param1;
		if (!healthArray)
		{
			Print("[AskalCore] ⚠️ InventoryHealthResponse recebido sem dados");
			return;
		}
		
		Print("[AskalCore] 📥 InventoryHealthResponse recebido com " + healthArray.Count() + " itens");
		
		// Armazenar health no helper (3_Game pode acessar)
		AskalNotificationHelper.SetInventoryHealth(healthArray);
	}
	
	// RPC Handler: Cliente recebe comando para abrir menu do trader
	void OpenTraderMenu(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		
		Param3<string, ref array<string>, ref array<int>> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] ❌ Erro ao ler OpenTraderMenu");
			return;
		}
		
		string traderName = data.param1;
		ref array<string> setupKeys = data.param2;
		ref array<int> setupValues = data.param3;
		
		Print("[AskalCore] 📥 OpenTraderMenu recebido para trader: " + traderName);
		
		// Contar entradas do SetupItems (evitar operador ternário)
		int setupCount = 0;
		if (setupKeys)
			setupCount = setupKeys.Count();
		Print("[AskalCore] 📦 SetupItems: " + setupCount.ToString() + " entradas");
		
		// Converter arrays de volta para map
		ref map<string, int> setupItems = new map<string, int>();
		if (setupKeys && setupValues && setupKeys.Count() == setupValues.Count())
		{
			for (int i = 0; i < setupKeys.Count(); i++)
			{
				string key = setupKeys.Get(i);
				int value = setupValues.Get(i);
				setupItems.Set(key, value);
			}
		}
		
		// Armazenar no helper para que o menu possa acessar quando for criado
		AskalNotificationHelper.RequestOpenTraderMenu(traderName, setupItems);
		
		Print("[AskalCore] ✅ Trader menu request armazenado, aguardando criação do menu");
	}
	
	override void OnMissionFinish(Class sender, CF_EventArgs args)
	{
		CF_Log.Info("[AskalCore] OnMissionFinish()");
    }
}
