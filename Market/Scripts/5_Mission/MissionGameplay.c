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
	
	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);
		
		// Tecla O - Toggle menu da loja
		if (key == KeyCode.KC_O)
		{
			Print("[AskalMarket] ========================================");
			Print("[AskalMarket] Tecla O pressionada");
		
			// Verificar sincronização
			if (GetGame().IsMultiplayer() && GetGame().IsClient())
			{
				bool isSynced = AskalDatabaseSync.IsClientSynced();
				int datasetCount = AskalDatabaseClientCache.GetInstance().GetDatasets().Count();
				
				Print("[AskalMarket] Estado:");
				
				string syncStatus = "NÃO";
				if (isSynced)
				{
					syncStatus = "SIM";
		}
		
				Print("[AskalMarket]   Sincronizado: " + syncStatus);
				Print("[AskalMarket]   Datasets: " + datasetCount);
				
				if (!isSynced)
		{
					Print("[AskalMarket] ⚠️ Database ainda não sincronizado");
					
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			if (player)
			{
				player.MessageStatus("Aguardando sincronização do servidor...");
			}
					
					// Tentar solicitar novamente
					if (!m_SyncRequested)
					{
						Print("[AskalMarket] 📤 Re-solicitando sincronização...");
						GetRPCManager().SendRPC("AskalCoreModule", "RequestDatasets", NULL, true, NULL, NULL);
						m_SyncRequested = true;
					}
					
					Print("[AskalMarket] ========================================");
			return;
		}
			}
		
			// Abrir/fechar menu
			ToggleToolsMenu();
			Print("[AskalMarket] ========================================");
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

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		
		// Verificar se há solicitação de abertura de menu do trader pendente
		// Isso permite que o menu seja criado quando o RPC é recebido
		if (GetGame().IsClient())
		{
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
