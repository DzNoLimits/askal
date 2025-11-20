// ==========================================
// AskalPurchaseModule - Módulo 4_World para processar compras
// Escuta RPCs diretamente para ter acesso a PlayerBase
// ==========================================

class AskalPurchaseModule
{
	// ITER-1: Rate limiting per player
	private static ref map<string, ref array<float>> s_RequestTimes = new map<string, ref array<float>>();
	private static const float RATE_LIMIT_WINDOW = 10.0; // 10 seconds
	private static const int MAX_REQUESTS_PER_WINDOW = 5; // Max 5 requests per 10 seconds
	private static bool s_MarketEnabled = true; // Config toggle for emergency disable
	
	private static ref AskalPurchaseModule s_Instance;
	
	void AskalPurchaseModule()
	{
		Print("[AskalPurchase] ========================================");
		Print("[AskalPurchase] Módulo de compras inicializado");
		Print("[AskalPurchase] Registrando RPC handler de compra");
		
		// Registrar RPC handler aqui (4_World tem acesso a PlayerBase)
		// NOTA: RPCs são registrados pelo AskalMarketModule, apenas adicionamos handlers
		if (AskalMarketHelpers.IsServerSafe())
		{
			GetRPCManager().AddRPC("AskalMarketModule", "PurchaseItemRequest", this, SingleplayerExecutionType.Server);
			GetRPCManager().AddRPC("AskalMarketModule", "PurchaseBatchRequest", this, SingleplayerExecutionType.Server);
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
	
	// ITER-1: Rate limiting check
	private static bool CheckRateLimit(string steamId)
	{
		if (!s_MarketEnabled)
		{
			Print("[AskalPurchase] RATE_LIMIT steamId=" + steamId + " reason=market_disabled");
			return false;
		}
		
		if (!s_RequestTimes.Contains(steamId))
		{
			s_RequestTimes.Set(steamId, new array<float>());
		}
		
		array<float> times = s_RequestTimes.Get(steamId);
		float now = GetGame().GetTime();
		
		// Remove old entries outside window
		for (int i = times.Count() - 1; i >= 0; i--)
		{
			if (now - times.Get(i) > RATE_LIMIT_WINDOW)
				times.Remove(i);
		}
		
		if (times.Count() >= MAX_REQUESTS_PER_WINDOW)
		{
			Print("[AskalPurchase] RATE_LIMIT steamId=" + steamId + " count=" + times.Count() + " window=" + RATE_LIMIT_WINDOW);
			return false;
		}
		
		times.Insert(now);
		return true;
	}
	
	// ITER-1: Emergency disable market (for rollback)
	static void SetMarketEnabled(bool enabled)
	{
		s_MarketEnabled = enabled;
		Print("[AskalPurchase] Market enabled: " + enabled);
	}
	
	protected void ProcessPurchaseRequest(PlayerIdentity sender, string steamId, string itemClass, int requestedPrice, string currencyId, float itemQuantity, int quantityType, int contentType, string traderName = "")
	{
		// ITER-1: Rate limiting check FIRST
		if (!CheckRateLimit(steamId))
		{
			Print("[AskalPurchase] [RATE_LIMIT] Request rejected for: " + steamId);
			SendPurchaseResponse(sender, false, itemClass, 0, "Rate limit exceeded. Please wait.");
			return;
		}
		
		Print("[AskalPurchase] [PROCESSAR] Iniciando processamento de compra...");
		
		if (!itemClass || itemClass == "")
		{
			Print("[AskalPurchase] [ERRO] ItemClass vazio na requisição de compra");
			SendPurchaseResponse(sender, false, itemClass, 0);
			return;
		}
		
		if (!currencyId || currencyId == "")
			currencyId = "Askal_Coin";
		
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
		
		// Processar compra (cálculo de preço autoritativo e validações são feitas dentro do serviço)
		Print("[AskalPurchase] [PROCESSAR] Chamando AskalPurchaseService.ProcessPurchaseWithQuantity...");
		bool success = AskalPurchaseService.ProcessPurchaseWithQuantity(sender, steamId, itemClass, requestedPrice, currencyId, itemQuantity, quantityType, contentType);
		
		Print("[AskalPurchase] [PROCESSAR] ProcessPurchaseWithQuantity retornou: " + success);
		
		string resultMessage = "Compra realizada com sucesso";
		if (!success)
			resultMessage = "Falha ao processar compra (sem espaço no inventário ou outro erro)";
		
		string statusText = "ERRO";
		if (success)
			statusText = "SUCESSO";
		Print("[AskalPurchase] [RESULTADO] " + statusText + " - " + resultMessage);
		
		Print("[AskalPurchase] [PROCESSAR] Enviando resposta ao cliente...");
		SendPurchaseResponse(sender, success, itemClass, requestedPrice, resultMessage);
		Print("[AskalPurchase] [PROCESSAR] Resposta enviada");
	}
	
	// RPC Handler: Servidor processa requisição de compra
	void PurchaseItemRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Print("[AskalPurchase] [RPC] PurchaseItemRequest recebido - Type: " + type + " | IsServer: " + AskalMarketHelpers.IsServerSafe());
		
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
	void SendPurchaseResponse(PlayerIdentity identity, bool success, string itemClass = "", int price = 0, string customMessage = "")
	{
		if (!identity)
			return;
		
		string message = customMessage;
		if (!message || message == "")
		{
			if (success)
				message = "Compra realizada com sucesso";
			else
				message = "Erro ao processar compra";
		}
		
		Param4<bool, string, string, int> params = new Param4<bool, string, string, int>(success, message, itemClass, price);
		GetRPCManager().SendRPC("AskalMarketModule", "PurchaseItemResponse", params, true, identity, NULL);
	}
}

