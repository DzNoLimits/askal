// ==========================================
// AskalMarketMissionModule - Módulo Mission do Market
// Registra input bindings para abrir/fechar Virtual Store
// ==========================================

// Usar o mesmo sistema do COT: JMModuleBase
// Como o Market depende de JM_CF_Scripts (que inclui o COT), podemos usar JMModuleBase

// Criar um módulo Mission específico para o Market
class AskalMarketMissionModule : JMModuleBase
{
    protected float m_LastToggleTime = 0.0;
    
    void AskalMarketMissionModule()
    {
        Print("[AskalMarket] ========================================");
        Print("[AskalMarket] ✅ AskalMarketMissionModule inicializado");
        Print("[AskalMarket] ========================================");
    }
    
    override void RegisterKeyMouseBindings()
    {
        Print("[AskalMarket] 🔍 RegisterKeyMouseBindings() chamado");
        
        super.RegisterKeyMouseBindings();
        
        // Registrar input para abrir/fechar Virtual Store
        Print("[AskalMarket] 📝 Registrando binding: UAAskalMarketToggleVirtualStore");
        Bind(new JMModuleBinding("ToggleVirtualStore", "UAAskalMarketToggleVirtualStore", true));
        
        Print("[AskalMarket] ✅ Input binding registrado: UAAskalMarketToggleVirtualStore");
    }
    
    // Este método será chamado quando o input for pressionado
    // O input será registrado via RegisterKeyMouseBindings
    void ToggleVirtualStore(UAInput input)
    {
        if (!input)
        {
            return;
        }
        
        // Usar LocalClick() para detectar apenas o momento do clique, não o estado contínuo
        // Isso garante que o método seja chamado apenas uma vez por pressionamento
        if (!input.LocalClick())
        {
            return;
        }
        
        float currentTime = GetGame().GetTime();
        
        // Debounce: evitar múltiplas chamadas em menos de 0.2 segundos
        if (currentTime - m_LastToggleTime < 0.2)
        {
            Print("[AskalMarket] ⚠️ Toggle muito rápido, ignorando (debounce)");
            return;
        }
        
        m_LastToggleTime = currentTime;
        
        Print("[AskalMarket] ========================================");
        Print("[AskalMarket] ✅ Input UAAskalMarketToggleVirtualStore pressionado (TOGGLE)");
        
        // Verificar se Virtual Store está habilitado
        if (!AskalVirtualStoreSettings.IsVirtualStoreEnabled())
        {
            Print("[AskalMarket] ⚠️ Virtual Store está desabilitado (VirtualStoreMode = 0)");
            Print("[AskalMarket] ========================================");
            return;
        }
        
        // Verificar se já existe um menu aberto (usando instância estática)
        AskalStoreMenu activeMenu = AskalStoreMenu.GetInstance();
        bool menuIsOpen = false;
        
        if (activeMenu)
        {
            Widget menuRoot = activeMenu.GetLayoutRoot();
            if (menuRoot && menuRoot.IsVisible())
            {
                menuIsOpen = true;
                Print("[AskalMarket] 🔍 Menu detectado como aberto via GetInstance()");
            }
        }
        
        // Verificar também via UIManager
        if (!menuIsOpen)
        {
            UIManager uiManager = GetGame().GetUIManager();
            if (uiManager)
            {
                UIScriptedMenu activeUIMenu = UIScriptedMenu.Cast(uiManager.GetMenu());
                if (activeUIMenu && AskalStoreMenu.Cast(activeUIMenu))
                {
                    menuIsOpen = true;
                    Print("[AskalMarket] 🔍 Menu detectado como aberto via UIManager");
                }
            }
        }
        
        // Obter MissionGameplay para acessar o menu
        MissionGameplay missionGP = MissionGameplay.Cast(GetGame().GetMission());
        if (!missionGP)
        {
            Print("[AskalMarket] ❌ MissionGameplay não encontrado");
            Print("[AskalMarket] ========================================");
            return;
        }
        
        if (menuIsOpen)
        {
            // Menu está aberto, fechar
            Print("[AskalMarket] 🔍 Menu está aberto, fechando...");
            
            if (activeMenu)
            {
                activeMenu.Close();
                Print("[AskalMarket] ✅ Close() chamado no menu ativo");
            }
            
            // Limpar referência no MissionGameplay
            missionGP.OnMenuClosed();
            Print("[AskalMarket] ✅ Referência limpa no MissionGameplay");
        }
        else
        {
            // Menu está fechado, abrir
            Print("[AskalMarket] 🔍 Menu está fechado, abrindo...");
            missionGP.ToggleToolsMenu();
        }
        
        Print("[AskalMarket] ========================================");
    }
}
