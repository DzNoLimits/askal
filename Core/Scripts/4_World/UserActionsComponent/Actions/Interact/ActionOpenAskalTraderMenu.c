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
		
		// Verificar se é ASK_TraderVendingMachine (tipo do objeto é suficiente)
		ASK_TraderVendingMachine traderVending = ASK_TraderVendingMachine.Cast(targetObj);
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
		
		// Verificar se é trader humano (ASK_TraderHumanBase)
		ASK_TraderHumanBase traderHuman = ASK_TraderHumanBase.Cast(targetObj);
		if (traderHuman)
		{
			trader = traderHuman.GetTraderLogic();
			if (trader)
			{
				traderName = trader.GetTraderName();
				if (traderName != "")
					m_Text = "Abrir Loja - " + traderName;
				else
					m_Text = "Abrir Loja";
				return true;
			}
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
		
		// Verificar se é ASK_TraderVendingMachine
		ASK_TraderVendingMachine traderVending = ASK_TraderVendingMachine.Cast(targetObj);
		AskalTraderBase trader = NULL;
		
		if (traderVending)
		{
			trader = traderVending.GetTraderLogic();
		}
		else
		{
			// Verificar se é trader humano (ASK_TraderHumanBase)
			ASK_TraderHumanBase traderHuman = ASK_TraderHumanBase.Cast(targetObj);
			if (traderHuman)
			{
				trader = traderHuman.GetTraderLogic();
			}
			
			// Se não encontrou, usar método genérico para compatibilidade
			if (!trader)
			{
				trader = AskalTraderSpawnService.GetTraderFromEntity(EntityAI.Cast(targetObj));
			}
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
		
		// Enviar RPC para o cliente abrir o menu (nome + SetupItems)
		Param3<string, ref array<string>, ref array<int>> params = new Param3<string, ref array<string>, ref array<int>>(trader.GetTraderName(), setupKeys, setupValues);
		GetRPCManager().SendRPC("AskalCoreModule", "OpenTraderMenu", params, true, player.GetIdentity(), NULL);
	}
}

