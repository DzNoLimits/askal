// ==========================================
// MissionServer do Market - inicializa módulos do Market
// Depende do Core estar carregado
// ==========================================

modded class MissionServer extends MissionBase
{
    void MissionServer()
    {
        Print("[AskalMarket] ========================================");
        Print("[AskalMarket] MissionServer do Market inicializado");
        Print("[AskalMarket] ========================================");
        
        // Aguardar um pouco para garantir que o Core já carregou o database
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(InitializeMarketModules, 100, false);
    }
    
    void InitializeMarketModules()
    {
        Print("[AskalMarket] ========================================");
        Print("[AskalMarket] INICIANDO MÓDULOS DO MARKET");
        Print("[AskalMarket] ========================================");
        
        // Verificar se o Core já carregou o database
        int totalDatasets = AskalDatabase.GetAllDatasetIDs().Count();
        if (totalDatasets == 0)
        {
            Print("[AskalMarket] ⚠️ Database ainda não carregado, aguardando...");
            // Tentar novamente em 500ms
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(InitializeMarketModules, 500, false);
            return;
        }
        
        Print("[AskalMarket] ✅ Database carregado (" + totalDatasets + " datasets)");
        
        // Inicializar módulos de compras e vendas (4_World)
        Print("[AskalMarket] Inicializando módulos de compra/venda...");
        AskalPurchaseModule.GetInstance();
        AskalSellModule.GetInstance();
        
        // Spawnar traders estáticos
        Print("[AskalMarket] ========================================");
        Print("[AskalMarket] SPAWNANDO TRADERS ESTÁTICOS");
        Print("[AskalMarket] ========================================");
        AskalTraderSpawnService.SpawnAllTraders();
        
        Print("[AskalMarket] ========================================");
        Print("[AskalMarket] MARKET INICIALIZADO COM SUCESSO");
        Print("[AskalMarket] ========================================");
    }
}

