// ==========================================
// AskalSellModule - Módulo RPC para venda de itens (ULTRA SIMPLIFICADO)
// ==========================================

class AskalSellModule
{
	protected static ref AskalSellModule s_Instance;
	
	void AskalSellModule()
	{
		if (GetGame().IsServer())
		{
			GetRPCManager().AddRPC("AskalSellModule", "SellItemRequest", this, SingleplayerExecutionType.Server);
			GetRPCManager().AddRPC("AskalCoreModule", "RequestInventoryHealth", this, SingleplayerExecutionType.Server);
		}
	}
	
	static AskalSellModule GetInstance()
	{
		if (!s_Instance)
		{
			s_Instance = new AskalSellModule();
		}
		return s_Instance;
	}
	
	// RPC Handler: Servidor processa requisição de venda
	void SellItemRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Print("[AskalSell] [RPC] SellItemRequest recebido");
		
		if (type != CallType.Server || !sender)
		{
			Print("[AskalSell] [ERRO] Tipo invalido ou sender NULL");
			SendSellResponse(sender, false, "Erro na requisição", "", 0);
			return;
		}
		
		// Param5: steamId, itemClassName, currencyId, transactionMode, traderOrVirtualStoreID
		Param5<string, string, string, int, string> data;
		if (!ctx.Read(data))
		{
			Print("[AskalSell] [ERRO] Falha ao ler parametros do RPC");
			SendSellResponse(sender, false, "Erro ao ler parâmetros", "", 0);
			return;
		}
		
		string steamId = data.param1;
		string itemClassName = data.param2;
		string currencyId = data.param3;
		int transactionMode = data.param4;
		string traderName = data.param5; // traderOrVirtualStoreID
		
		Print("[AskalSell] [RPC] Item: " + itemClassName + " | Currency: " + currencyId + " | Mode: " + transactionMode + " | Trader: " + traderName);
		
		if (!steamId || steamId == "")
		{
			steamId = sender.GetPlainId();
			if (!steamId || steamId == "")
				steamId = sender.GetId();
		}
		
		if (!currencyId || currencyId == "")
			currencyId = "Askal_Coin";
		
		// VALIDAÇÃO: Verificar se item pode ser vendido neste trader
		if (traderName && traderName != "" && traderName != "Trader_Default")
		{
			if (!AskalTraderValidationHelper.CanSellItem(traderName, itemClassName))
			{
				Print("[AskalSell] [ERRO] Item não pode ser vendido neste trader: " + itemClassName + " | Trader: " + traderName);
				SendSellResponse(sender, false, "Item não pode ser vendido neste trader", itemClassName, 0);
				return;
			}
		}
		
		// Busca o player
		Print("[AskalSell] [RPC] Buscando player...");
		PlayerBase player = AskalSellService.GetPlayerFromIdentity(sender);
		if (!player)
		{
			Print("[AskalSell] [ERRO] Player nao encontrado");
			SendSellResponse(sender, false, "Player não encontrado", itemClassName, 0);
			return;
		}
		Print("[AskalSell] [RPC] Player encontrado: " + player.GetIdentity().GetName());
		
		// Busca item no inventário (case-insensitive)
		Print("[AskalSell] [RPC] Buscando item no inventario: " + itemClassName);
		EntityAI itemToSell = FindItemInInventory(player, itemClassName);
		if (!itemToSell)
		{
			Print("[AskalSell] [ERRO] Item nao encontrado no inventario");
			SendSellResponse(sender, false, "Item não encontrado no inventário: " + itemClassName, itemClassName, 0);
			return;
		}
		Print("[AskalSell] [RPC] Item encontrado: " + itemToSell.GetType());
		
		// Verificar se item tem cargo ANTES de processar venda
		if (AskalSellService.HasCargoItemsRecursive(itemToSell))
		{
			Print("[AskalSell] [ERRO] Item tem cargo - venda bloqueada");
			SendSellResponse(sender, false, "Item ocupado, esvazie para vender", itemClassName, 0);
			return;
		}
		
		// Processa venda
		Print("[AskalSell] [RPC] Processando venda...");
		int sellPrice = 0;
		bool success = AskalSellService.ProcessSell(sender, steamId, itemToSell, currencyId, transactionMode, sellPrice);
		
		if (success)
		{
			Print("[AskalSell] [RPC] Venda processada com sucesso - Preço: " + sellPrice);
			SendSellResponse(sender, true, "Venda realizada com sucesso", itemClassName, sellPrice);
		}
		else
		{
			Print("[AskalSell] [ERRO] Falha ao processar venda (ver ProcessSell)");
			// Verificar se foi por causa de cargo (pode ter sido detectado no ProcessSell também)
			if (AskalSellService.HasCargoItemsRecursive(itemToSell))
			{
				SendSellResponse(sender, false, "Item ocupado, esvazie para vender", itemClassName, 0);
			}
			else
			{
				SendSellResponse(sender, false, "Falha ao processar venda", itemClassName, 0);
			}
		}
	}
	
	// Busca item no inventário (case-insensitive, simplificado)
	EntityAI FindItemInInventory(PlayerBase player, string itemClassName)
	{
		if (!player || !itemClassName || itemClassName == "")
			return NULL;
		
		string searchLower = itemClassName;
		searchLower.ToLower();
		
		// 1. Verifica item nas mãos primeiro
		EntityAI itemInHands = player.GetHumanInventory().GetEntityInHands();
		if (itemInHands)
		{
			string handsType = itemInHands.GetType();
			string handsLower = handsType;
			handsLower.ToLower();
			
			if (handsLower == searchLower && CanRemove(itemInHands))
				return itemInHands;
		}
		
		// 2. Enumera todo o inventário
		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		
		foreach (EntityAI item : items)
		{
			if (!item)
				continue;
			
			string itemType = item.GetType();
			string itemLower = itemType;
			itemLower.ToLower();
			
			if (itemLower == searchLower && CanRemove(item))
				return item;
		}
		
		return NULL;
	}
	
	// Verifica se item pode ser removido (simplificado)
	bool CanRemove(EntityAI item)
	{
		if (!item || !item.GetInventory())
			return false;
		
		return item.GetInventory().CanRemoveEntity();
	}
	
	// Enviar resposta de venda para o cliente
	void SendSellResponse(PlayerIdentity identity, bool success, string message = "", string itemClass = "", int price = 0)
	{
		if (!identity)
			return;
		
		if (!message || message == "")
		{
			if (success)
				message = "Venda realizada com sucesso";
			else
				message = "Erro ao processar venda";
		}
		
		Param4<bool, string, string, int> params = new Param4<bool, string, string, int>(success, message, itemClass, price);
		GetRPCManager().SendRPC("AskalCoreModule", "SellItemResponse", params, true, identity, NULL);
	}
	
	// RPC Handler: Servidor processa requisição de health dos itens do inventário
	void RequestInventoryHealth(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
			return;
		
		Print("[AskalSell] [RPC] RequestInventoryHealth recebido de: " + sender.GetName());
		
		PlayerBase player = AskalSellService.GetPlayerFromIdentity(sender);
		if (!player)
		{
			Print("[AskalSell] [ERRO] Player não encontrado para RequestInventoryHealth");
			return;
		}
		
		// Escanear inventário e coletar health de todos os itens
		array<EntityAI> inventoryItems = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, inventoryItems);
		
		// Criar map de className -> healthPercent
		ref map<string, float> healthMap = new map<string, float>();
		
		foreach (EntityAI item : inventoryItems)
		{
			if (!item)
				continue;
			
			string className = item.GetType();
			string normalizedClass = className;
			normalizedClass.ToLower();
			
			// Obter health (0.0 a 1.0)
			float health01 = item.GetHealth01();
			float healthPercent = health01 * 100.0;
			
			// Garantir que está entre 0 e 100
			if (healthPercent < 0)
				healthPercent = 0;
			if (healthPercent > 100)
				healthPercent = 100;
			
			// Armazenar (sobrescreve se já existe - pega o primeiro encontrado)
			if (!healthMap.Contains(normalizedClass))
			{
				healthMap.Insert(normalizedClass, healthPercent);
			}
		}
		
		Print("[AskalSell] [RPC] Health coletado para " + healthMap.Count() + " tipos de itens");
		
		// Enviar resposta ao cliente
		SendInventoryHealthResponse(sender, healthMap);
	}
	
	// Enviar resposta de health para o cliente
	void SendInventoryHealthResponse(PlayerIdentity identity, ref map<string, float> healthMap)
	{
		if (!identity || !healthMap)
			return;
		
		// Converter map para array de pares (className, healthPercent)
		ref array<ref Param2<string, float>> healthArray = new array<ref Param2<string, float>>();
		
		for (int i = 0; i < healthMap.Count(); i++)
		{
			string className = healthMap.GetKey(i);
			float healthPercent = healthMap.GetElement(i);
			healthArray.Insert(new Param2<string, float>(className, healthPercent));
		}
		
		Param1<ref array<ref Param2<string, float>>> params = new Param1<ref array<ref Param2<string, float>>>(healthArray);
		GetRPCManager().SendRPC("AskalCoreModule", "InventoryHealthResponse", params, true, identity, NULL);
		
		Print("[AskalSell] [RPC] InventoryHealthResponse enviado com " + healthArray.Count() + " itens");
	}
}
