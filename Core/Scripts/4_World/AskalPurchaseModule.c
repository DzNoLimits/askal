// ==========================================
// AskalPurchaseModule - Módulo 4_World para processar compras
// Escuta RPCs diretamente para ter acesso a PlayerBase
// ==========================================

class AskalPurchaseModule
{
	private static ref AskalPurchaseModule s_Instance;
	
	void AskalPurchaseModule()
	{
		Print("[AskalPurchase] ========================================");
		Print("[AskalPurchase] Módulo de compras inicializado");
		Print("[AskalPurchase] Registrando RPC handler de compra");
		
		// Registrar RPC handler aqui (4_World tem acesso a PlayerBase)
		if (GetGame().IsServer())
		{
			GetRPCManager().AddRPC("AskalPurchaseModule", "PurchaseItemRequest", this, SingleplayerExecutionType.Server);
			GetRPCManager().AddRPC("AskalPurchaseModule", "PurchaseBatchRequest", this, SingleplayerExecutionType.Server);
			Print("[AskalPurchase] ✅ RPC handler registrado");
		}
		
		Print("[AskalPurchase] ========================================");
	}
	
	static AskalPurchaseModule GetInstance()
	{
		if (!s_Instance)
		{
			s_Instance = new AskalPurchaseModule();
		}
		return s_Instance;
	}
	
	protected string ResolveSteamId(string steamId, PlayerIdentity sender)
	{
		if (steamId && steamId != "")
			return steamId;
		
		if (sender)
		{
			string resolved = sender.GetPlainId();
			if (!resolved || resolved == "")
				resolved = sender.GetId();
			return resolved;
		}
		
		return "";
	}
	
	protected void ProcessPurchaseRequest(PlayerIdentity sender, string steamId, string itemClass, int requestedPrice, string currencyId, float itemQuantity, int quantityType, int contentType, string traderName = "")
	{
		Print("[AskalPurchase] [PROCESSAR] Iniciando processamento de compra...");
		
		if (!itemClass || itemClass == "")
		{
			Print("[AskalPurchase] [ERRO] ItemClass vazio na requisição de compra");
			SendPurchaseResponse(sender, false, itemClass, 0);
			return;
		}
		
		// Resolve accepted currency (trader -> virtual store -> default)
		AskalCurrencyConfig resolvedCurrencyCfg = NULL;
		string resolvedCurrencyId = "";
		
		// If no trader specified, get Virtual Store currency
		string virtualStoreCurrency = "";
		if (!traderName || traderName == "" || traderName == "Trader_Default")
		{
			AskalVirtualStoreConfig virtualStoreConfig = AskalVirtualStoreSettings.GetConfig();
			if (virtualStoreConfig)
				virtualStoreCurrency = virtualStoreConfig.GetPrimaryCurrency();
		}
		
		if (!AskalMarketConfig.ResolveAcceptedCurrency(traderName, virtualStoreCurrency, resolvedCurrencyId, resolvedCurrencyCfg))
		{
			Print("[AskalPurchase] [ERRO] Falha ao resolver currency para trader: " + traderName);
			SendPurchaseResponse(sender, false, itemClass, 0);
			return;
		}
		
		currencyId = resolvedCurrencyId;
		
		// VALIDAÇÃO: Verificar se item pode ser comprado neste trader
		if (traderName && traderName != "")
		{
			if (!AskalTraderValidationHelper.CanBuyItem(traderName, itemClass))
			{
				Print("[AskalPurchase] [ERRO] Item não pode ser comprado neste trader: " + itemClass + " | Trader: " + traderName);
				SendPurchaseResponse(sender, false, itemClass, 0);
				return;
			}
		}
		
		Print("[AskalPurchase] [COMPRA] Solicitação de compra recebida:");
		Print("[AskalPurchase]   Player: " + steamId);
		Print("[AskalPurchase]   Item: " + itemClass);
		Print("[AskalPurchase]   Preço solicitado: " + requestedPrice);
		Print("[AskalPurchase]   Moeda: " + currencyId);
		Print("[AskalPurchase]   Trader: " + traderName);
		Print("[AskalPurchase]   Quantidade: " + itemQuantity + " | Tipo: " + quantityType + " | Conteúdo: " + contentType);
		
		Print("[AskalPurchase] [PROCESSAR] Chamando AskalPurchaseService.ProcessPurchaseWithQuantity...");
		bool success = AskalPurchaseService.ProcessPurchaseWithQuantity(sender, steamId, itemClass, requestedPrice, currencyId, itemQuantity, quantityType, contentType, traderName);
		
		Print("[AskalPurchase] [PROCESSAR] ProcessPurchaseWithQuantity retornou: " + success);
		
		string resultMessage = "Compra realizada com sucesso";
		if (!success)
			resultMessage = "Falha ao processar compra (verifique logs do servidor)";
		
		string statusText = "ERRO";
		if (success)
			statusText = "SUCESSO";
		Print("[AskalPurchase] [RESULTADO] " + statusText + " - " + resultMessage);
		
		Print("[AskalPurchase] [PROCESSAR] Enviando resposta ao cliente...");
		SendPurchaseResponse(sender, success, itemClass, requestedPrice);
		Print("[AskalPurchase] [PROCESSAR] Resposta enviada");
	}
	
	// RPC Handler: Servidor processa requisição de compra
	void PurchaseItemRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Print("[AskalPurchase] [RPC] PurchaseItemRequest recebido - Type: " + type + " | IsServer: " + GetGame().IsServer());
		
		if (type != CallType.Server)
		{
			Print("[AskalPurchase] [ERRO] PurchaseItemRequest chamado fora do servidor");
			return;
		}
		
		if (!sender)
		{
			Print("[AskalPurchase] [ERRO] Sender NULL");
			return;
		}
		
		Print("[AskalPurchase] [RPC] Lendo parâmetros do RPC...");
		Param8<string, string, int, string, float, int, int, string> data;
		if (!ctx.Read(data))
		{
			Print("[AskalPurchase] [ERRO] Erro ao ler PurchaseItemRequest");
			return;
		}
		
		Print("[AskalPurchase] [RPC] Parâmetros lidos com sucesso");
		string steamId = ResolveSteamId(data.param1, sender);
		string traderName = data.param8;
		Print("[AskalPurchase] [RPC] SteamId resolvido: " + steamId + " | Trader: " + traderName);
		ProcessPurchaseRequest(sender, steamId, data.param2, data.param3, data.param4, data.param5, data.param6, data.param7, traderName);
	}
	
	void PurchaseBatchRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			Print("[AskalPurchase] ⚠️ PurchaseBatchRequest chamado fora do servidor");
			return;
		}
		
		if (!sender)
		{
			Print("[AskalPurchase] ❌ Sender NULL em PurchaseBatchRequest");
			return;
		}
		
		Param3<string, string, ref array<ref AskalPurchaseRequestData>> data;
		if (!ctx.Read(data))
		{
			Print("[AskalPurchase] ❌ Erro ao ler PurchaseBatchRequest");
			return;
		}
		
		string steamId = ResolveSteamId(data.param1, sender);
		string currencyId = data.param2;
		ref array<ref AskalPurchaseRequestData> requests = data.param3;
		if (!requests || requests.Count() == 0)
		{
			Print("[AskalPurchase] ⚠️ PurchaseBatchRequest recebido sem itens");
			return;
		}
		
		Print("[AskalPurchase] 💼 Processando lote de compras: " + requests.Count() + " itens");
		for (int i = 0; i < requests.Count(); i++)
		{
			AskalPurchaseRequestData request = requests.Get(i);
			if (!request)
				continue;
			
			// Batch não tem traderName por enquanto (usar "" para compatibilidade)
			ProcessPurchaseRequest(sender, steamId, request.ItemClass, request.Price, currencyId, request.Quantity, request.QuantityType, request.ContentType, "");
		}
	}
	
	// Enviar resposta de compra para o cliente
	void SendPurchaseResponse(PlayerIdentity identity, bool success, string itemClass = "", int price = 0)
	{
		if (!identity)
			return;
		
		string message = "Erro ao processar compra";
		if (success)
		{
			message = "Compra realizada com sucesso";
		}
		
		Param4<bool, string, string, int> params = new Param4<bool, string, string, int>(success, message, itemClass, price);
		GetRPCManager().SendRPC("AskalCoreModule", "PurchaseItemResponse", params, true, identity, NULL);
	}
}

