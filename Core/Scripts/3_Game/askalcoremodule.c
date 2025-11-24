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
		if (AskalCoreHelpers.IsServerSafe())
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
		
		// NOTA: RPCs de compra/venda, health e trader menu foram movidos para Market
		// Market registra seus próprios RPCs via AskalMarketModule
	
	Print("[AskalCore] ✅ RPCs do Core registrados");
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
		int virtualStoreMode = config.VirtualStoreMode;
		
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
		
		Param6<string, float, float, int, ref array<string>, ref array<int>> data = new Param6<string, float, float, int, ref array<string>, ref array<int>>(currencyId, buyCoeff, sellCoeff, virtualStoreMode, setupKeys, setupValues);
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
		
		Print("[AskalCore] ========================================");
		Print("[AskalCore] 📥 VirtualStoreConfigResponse recebido");
		
		Param6<string, float, float, int, ref array<string>, ref array<int>> data;
		if (!ctx.Read(data))
		{
			Print("[AskalCore] ❌ Erro ao ler VirtualStoreConfigResponse");
			Print("[AskalCore] ========================================");
			return;
		}
		
		string currencyId = data.param1;
		float buyCoeff = data.param2;
		float sellCoeff = data.param3;
		int virtualStoreMode = data.param4;
		array<string> setupKeys = data.param5;
		array<int> setupValues = data.param6;
		
		Print("[AskalCore] 📦 VirtualStoreMode: " + virtualStoreMode.ToString());
		
		string setupCountStr = "0";
		if (setupKeys)
			setupCountStr = setupKeys.Count().ToString();
		Print("[AskalCore] 📦 SetupItems: " + setupCountStr + " entradas");
		
		AskalVirtualStoreSettings.ApplyConfigFromServer(currencyId, buyCoeff, sellCoeff, virtualStoreMode, setupKeys, setupValues);
		
		Print("[AskalCore] ✅ Config do Virtual Store aplicada no cliente");
		Print("[AskalCore] ========================================");
	}
	
	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
		Print("[AskalCore] ========================================");
		Print("[AskalCore] OnMissionStart() → Inicializando");
		
		// Marcar database como pronto APENAS no servidor
		// Cliente recebe dados via RPC, não precisa carregar do filesystem
		// Verificação segura: GetGame() pode retornar NULL durante inicialização
		if (AskalCoreHelpers.IsServerSafe())
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
	
	// NOTA: Handlers de compra/venda, health e trader menu foram movidos para Market
	// Market gerencia seus próprios RPCs via AskalMarketModule
	
	override void OnMissionFinish(Class sender, CF_EventArgs args)
	{
		CF_Log.Info("[AskalCore] OnMissionFinish()");
    }
}
