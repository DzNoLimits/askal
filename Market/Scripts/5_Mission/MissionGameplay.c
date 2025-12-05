modded class MissionGameplay extends MissionBase
{
	protected ref AskalStoreMenu m_ToolsMenu;
	protected bool m_SyncRequested = false;
	
	void MissionGameplay()
	{
		Print("[AskalMarket] ========================================");
		Print("[AskalMarket] MissionGameplay CLIENT inicializado");
		Print("[AskalMarket] ========================================");
		
		// Solicitar sincronização do database ao servidor
		// Padrão TraderX: cliente solicita dados quando inicia
		if (GetGame().IsMultiplayer() && GetGame().IsClient())
		{
			Print("[AskalMarket] 📤 Solicitando sincronização do database...");
			GetRPCManager().SendRPC("AskalCoreModule", "RequestDatasets", NULL, true, NULL, NULL);
			m_SyncRequested = true;
			Print("[AskalMarket] ✅ RPC RequestDatasets enviado");
			
			// Solicitar MarketConfig do servidor
			Print("[AskalMarket] 📤 Solicitando MarketConfig do servidor...");
			GetRPCManager().SendRPC("AskalCoreModule", "RequestMarketConfig", NULL, true, NULL, NULL);
			Print("[AskalMarket] ✅ RPC RequestMarketConfig enviado");
		}
		
		Print("[AskalMarket] ========================================");
	}
	
	override void OnMissionStart()
	{
		super.OnMissionStart();
		
		Print("[AskalMarket] ========================================");
		Print("[AskalMarket] OnMissionStart()");
		
		if (GetGame().IsMultiplayer() && GetGame().IsClient())
		{
			Print("[AskalMarket] Cliente aguardando sincronização via RPC");
		}
		else if (GetGame().IsServer())
		{
			Print("[AskalMarket] Servidor pronto");
		}
		else
		{
			Print("[AskalMarket] Singleplayer mode");
		}
		
		Print("[AskalMarket] ========================================");
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		
		if (!GetGame().IsClient())
			return;
		
		// Verificar input customizado para abrir/fechar menu
		UAInput input = GetUApi().GetInputByName("UAAskalMarketToggle");
		if (input && input.LocalPress())
		{
			// Verificar se há outros menus abertos (inventário, pause, map, etc)
			UIScriptedMenu currentMenu = GetGame().GetUIManager().GetMenu();
			if (currentMenu && currentMenu != m_ToolsMenu)
			{
				// Outro menu está aberto, não abrir o market
				return;
			}
			
			// Verificar se o player está em uma ação que impede abrir o menu
			// (ex: inventário aberto, menu de pause, etc)
			// A verificação de GetMenu() acima já cobre a maioria dos casos
			
			// Verificar sincronização
			if (GetGame().IsMultiplayer())
			{
				bool isSynced = AskalDatabaseSync.IsClientSynced();
				if (!isSynced)
				{
					PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
					if (player)
					{
						player.MessageStatus("Aguardando sincronização do servidor...");
					}
					
					if (!m_SyncRequested)
					{
						GetRPCManager().SendRPC("AskalCoreModule", "RequestDatasets", NULL, true, NULL, NULL);
						m_SyncRequested = true;
					}
					return;
				}
			}
			
			// Toggle menu
			ToggleToolsMenu();
		}
		
		// Verificar se há solicitação de abertura de menu do trader pendente
		string pendingTraderMenu = AskalNotificationHelper.GetPendingTraderMenu();
		if (pendingTraderMenu && pendingTraderMenu != "")
		{
			Print("[AskalMarket] 🏪 Trader pendente detectado: " + pendingTraderMenu);
			
			// Se o menu já existe, apenas chamar OpenTraderMenu
			if (m_ToolsMenu)
			{
				Print("[AskalMarket] ✅ Menu já existe, chamando OpenTraderMenu()");
				m_ToolsMenu.OpenTraderMenu(pendingTraderMenu);
			}
			else
			{
				Print("[AskalMarket] 📦 Criando novo menu para trader: " + pendingTraderMenu);
				m_ToolsMenu = new AskalStoreMenu();
				if (m_ToolsMenu)
				{
					GetGame().GetUIManager().ShowScriptedMenu(m_ToolsMenu, NULL);
					Print("[AskalMarket] ✅ Menu do trader criado e exibido");
				}
				else
				{
					Print("[AskalMarket] ❌ Falha ao criar AskalStoreMenu");
				}
			}
			
			// Limpar o trader pendente
			AskalNotificationHelper.ClearPendingTraderMenu();
		}
	}
	
	void ToggleToolsMenu()
	{
		Print("[AskalMarket] ToggleToolsMenu()");
		
		if (m_ToolsMenu)
		{
			Print("[AskalMarket] Fechando menu");
			m_ToolsMenu.Close();
			m_ToolsMenu = NULL;
		}
		else
		{
			Print("[AskalMarket] Abrindo menu");
		m_ToolsMenu = new AskalStoreMenu();
		if (m_ToolsMenu)
		{
			GetGame().GetUIManager().ShowScriptedMenu(m_ToolsMenu, NULL);
				Print("[AskalMarket] ✅ Menu aberto");
		}
		else
		{
				Print("[AskalMarket] ❌ Erro ao criar menu");
		}
	}
	}


	override void OnMissionFinish()
	{
		super.OnMissionFinish();
		
		if (m_ToolsMenu)
	{
			m_ToolsMenu.Close();
			m_ToolsMenu = NULL;
		}
		
		Print("[AskalMarket] OnMissionFinish()");
	}
}
