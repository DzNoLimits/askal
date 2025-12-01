// ==========================================
// ActionOpenAskalTraderMenu - UserAction para abrir menu do trader
// Inspirado em ExpansionActionOpenTraderMenu
// ==========================================

class ActionOpenAskalTraderMenu: ActionInteractBase
{
	void ActionOpenAskalTraderMenu()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ATTACHITEM;
		m_Text = "Abrir Loja";
	}
	
	override void CreateConditionComponents()  
	{	
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTCursor;
	}
			
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!player || !target)
			return false;
		
		// Obter objeto do target
		Object targetObj = target.GetObject();
		if (!targetObj)
			return false;
		
		// Declarar variáveis uma única vez
		AskalTraderBase trader = NULL;
		string traderName = "";
		
		// Verificar se é AskalTraderVendingMachine (tipo do objeto é suficiente)
		AskalTraderVendingMachine traderVending = AskalTraderVendingMachine.Cast(targetObj);
		if (traderVending)
		{
			// No cliente, tentar obter nome do trader se disponível
			// Se não estiver disponível, usar nome genérico
			trader = traderVending.GetTraderLogic();
			if (trader)
			{
				traderName = trader.GetTraderName();
				if (traderName != "")
					m_Text = "Abrir Loja - " + traderName;
				else
					m_Text = "Abrir Loja";
			}
			else
			{
				m_Text = "Abrir Loja";
			}
			return true;
		}
		
		// Verificar se é trader genérico (usando método do spawn service)
		// Isso funciona no cliente porque usa EnScript.GetClassVar
		trader = AskalTraderSpawnService.GetTraderFromEntity(EntityAI.Cast(targetObj));
		if (trader)
		{
			traderName = trader.GetTraderName();
			if (traderName != "")
				m_Text = "Abrir Loja - " + traderName;
			else
				m_Text = "Abrir Loja";
			return true;
		}
		
		return false;
	}
	
	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);
		
		PlayerBase player = action_data.m_Player;
		if (!player || !player.GetIdentity())
		{
			Print("[AskalTrader] ❌ Player ou identity inválido");
			return;
		}
		
		// Obter trader do target
		Object targetObj = action_data.m_Target.GetObject();
		if (!targetObj)
		{
			Print("[AskalTrader] ❌ Target object inválido");
			return;
		}
		
		// Verificar se é AskalTraderVendingMachine
		AskalTraderVendingMachine traderVending = AskalTraderVendingMachine.Cast(targetObj);
		AskalTraderBase trader = NULL;
		
		if (traderVending)
		{
			trader = traderVending.GetTraderLogic();
		}
		else
		{
			// Método genérico para compatibilidade
			trader = AskalTraderSpawnService.GetTraderFromEntity(EntityAI.Cast(targetObj));
		}
		
		if (!trader)
		{
			Print("[AskalTrader] ❌ Trader não encontrado no objeto");
			return;
		}
		
		Print("[AskalTrader] ✅ Abrindo menu do trader: " + trader.GetTraderName());
		
		// Obter SetupItems do trader
		map<string, int> setupItems = trader.GetSetupItems();
		
		// Converter map para arrays para enviar via RPC
		ref array<string> setupKeys = new array<string>();
		ref array<int> setupValues = new array<int>();
		
		if (setupItems)
		{
			for (int i = 0; i < setupItems.Count(); i++)
			{
				setupKeys.Insert(setupItems.GetKey(i));
				setupValues.Insert(setupItems.GetElement(i));
			}
		}
		
		Print("[AskalTrader] 📦 SetupItems: " + setupKeys.Count() + " entradas");
		
		// Resolve currency using helper (validates against MarketConfig)
		AskalTraderConfig traderConfig = trader.GetConfig();
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		string acceptedCurrency = AskalMarketHelpers.ResolveCurrencyForTrader(traderConfig, marketConfig);
		
		// Get currency shortname
		string currencyShortName = "";
		if (marketConfig && acceptedCurrency != "")
		{
			AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(acceptedCurrency);
			if (currencyCfg && currencyCfg.ShortName != "")
			{
				currencyShortName = currencyCfg.ShortName;
				// Remove @ prefix if present
				if (currencyShortName.Length() > 0 && currencyShortName.Substring(0, 1) == "@")
					currencyShortName = currencyShortName.Substring(1, currencyShortName.Length() - 1);
			}
		}
		
		// Fallback to currencyId if shortname not found
		if (currencyShortName == "")
			currencyShortName = acceptedCurrency;
		
		Print("[AskalTrader] 💰 AcceptedCurrency: " + acceptedCurrency + " (shortname: " + currencyShortName + ")");
		
		// Enviar RPC para o cliente abrir o menu (nome + SetupItems + AcceptedCurrency + ShortName)
		Param5<string, ref array<string>, ref array<int>, string, string> params = new Param5<string, ref array<string>, ref array<int>, string, string>(trader.GetTraderName(), setupKeys, setupValues, acceptedCurrency, currencyShortName);
		GetRPCManager().SendRPC("AskalMarketModule", "OpenTraderMenu", params, true, player.GetIdentity(), NULL);
	}
}

