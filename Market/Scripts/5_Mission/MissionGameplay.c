modded class MissionGameplay extends MissionBase
{
	protected ref AskalStoreMenu m_ToolsMenu;
	protected bool m_SyncRequested = false;
	protected bool m_ToggleInProgress = false;
	protected float m_MenuCreatedTime = 0.0;
	
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
			
			// Solicitar configuração do Virtual Store
			Print("[AskalMarket] 📤 Solicitando configuração do Virtual Store...");
			GetRPCManager().SendRPC("AskalCoreModule", "RequestVirtualStoreConfig", NULL, true, NULL, NULL);
			Print("[AskalMarket] ✅ RPC RequestVirtualStoreConfig enviado");
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
	
	// Atalho "O" removido - agora o menu é aberto através do botão no menu in-game (ESC)
	// override void OnKeyPress(int key) removido
	
	void ToggleToolsMenu()
	{
		// Proteção contra múltiplas chamadas simultâneas
		if (m_ToggleInProgress)
		{
			Print("[AskalMarket] ⚠️ ToggleToolsMenu já em progresso, ignorando");
			return;
		}
		
		m_ToggleInProgress = true;
		
		// Verificar se menu já existe e está aberto (múltiplas verificações)
		bool menuIsOpen = false;
		AskalStoreMenu menuToClose = NULL;
		Widget menuRoot;
		
		// Verificação 1: Instância estática
		AskalStoreMenu staticInstance = AskalStoreMenu.GetInstance();
		if (staticInstance)
		{
			menuRoot = staticInstance.GetLayoutRoot();
			if (menuRoot && menuRoot.IsVisible())
				{
				menuIsOpen = true;
				menuToClose = staticInstance;
				Print("[AskalMarket] 🔍 Menu detectado como aberto via GetInstance()");
			}
		}
		
		// Verificação 2: Referência local
		if (!menuIsOpen && m_ToolsMenu)
		{
			menuRoot = m_ToolsMenu.GetLayoutRoot();
			if (menuRoot && menuRoot.IsVisible())
			{
				menuIsOpen = true;
				menuToClose = m_ToolsMenu;
				Print("[AskalMarket] 🔍 Menu detectado como aberto via m_ToolsMenu");
			}
			else if (menuRoot)
		{
				// Menu existe mas não está visível, limpar referência
				Print("[AskalMarket] ⚠️ Menu existe mas não está visível, limpando referência");
				m_ToolsMenu = NULL;
			}
		}
		
		// Verificação 3: UIManager
		if (!menuIsOpen)
		{
			UIManager uiManager = GetGame().GetUIManager();
			if (uiManager)
			{
				UIScriptedMenu activeUIMenu = UIScriptedMenu.Cast(uiManager.GetMenu());
				if (activeUIMenu)
					{
					AskalStoreMenu castedMenu = AskalStoreMenu.Cast(activeUIMenu);
					if (castedMenu)
					{
						menuIsOpen = true;
						menuToClose = castedMenu;
						Print("[AskalMarket] 🔍 Menu detectado como aberto via UIManager");
					}
				}
		}
			}
		
		// Se menu está aberto, fechar
		if (menuIsOpen && menuToClose)
		{
			Print("[AskalMarket] ========================================");
			Print("[AskalMarket] ToggleToolsMenu: Menu está aberto, fechando...");
	
			// Fechar usando o método do menu
			menuToClose.Close();
		
			// Limpar referências
			m_ToolsMenu = NULL;
			OnMenuClosed();
			
			Print("[AskalMarket] ✅ Menu fechado via toggle");
			Print("[AskalMarket] ========================================");
			m_ToggleInProgress = false;
			return;
		}
		
		// Menu está fechado, verificar se Virtual Store está habilitado
		if (!AskalVirtualStoreSettings.IsVirtualStoreEnabled())
		{
			Print("[AskalMarket] ⚠️ Virtual Store está desabilitado");
			m_ToggleInProgress = false;
			return;
		}
		
		// Criar e abrir menu
		Print("[AskalMarket] ========================================");
		Print("[AskalMarket] ToggleToolsMenu: Abrindo menu do Virtual Store");
		m_ToolsMenu = new AskalStoreMenu();
		if (m_ToolsMenu)
		{
			GetGame().GetUIManager().ShowScriptedMenu(m_ToolsMenu, NULL);
			Print("[AskalMarket] ✅ Menu do Virtual Store aberto");
		}
		else
		{
				Print("[AskalMarket] ❌ Erro ao criar menu");
		}
		Print("[AskalMarket] ========================================");
		
		m_ToggleInProgress = false;
	}
	
	// Método para limpar referência quando menu é fechado
	void OnMenuClosed()
	{
		if (m_ToolsMenu)
		{
			m_ToolsMenu = NULL;
			Print("[AskalMarket] ✅ Referência do menu limpa (OnMenuClosed)");
	}
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		
		#ifndef SERVER
		// REMOVIDO: Verificação de visibilidade estava causando problemas
		// O menu se fecha naturalmente quando o usuário pressiona ESC ou fecha
		// A referência será limpa quando o menu for fechado explicitamente
		#endif
		
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
