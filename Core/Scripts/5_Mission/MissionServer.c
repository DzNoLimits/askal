// MissionServer do Core - inicializa o database quando a missão inicia
// Este é o ponto de entrada correto para I/O em 5_Mission
modded class MissionServer extends MissionBase
{
    void MissionServer()
    {
        Print("[AskalCore] ========================================");
        Print("[AskalCore] MissionServer do Core inicializado");
        Print("[AskalCore] ========================================");
        
        // Carregar datasets JSON do novo formato
        Print("[AskalCore] ========================================");
        Print("[AskalCore] INICIANDO CARREGAMENTO DE DATASETS JSON");
        Print("[AskalCore] ========================================");
        
        // Verificar caminho antes de carregar
        string dbPath = AskalDatabase.GetDatabasePath();
        Print("[AskalCore] Caminho do database: " + dbPath);
        
        Print("[AskalCore] ANTES de chamar AskalDatabaseLoader.LoadAllDatasets()");
        AskalDatabaseLoader.LoadAllDatasets();
        Print("[AskalCore] DEPOIS de chamar AskalDatabaseLoader.LoadAllDatasets()");
        
        Print("[AskalCore] ========================================");
        Print("[AskalCore] CARREGAMENTO DE DATASETS CONCLUÍDO");
        Print("[AskalCore] ========================================");
        
        // Verificar quantos datasets foram carregados
        int totalDatasets = AskalDatabase.GetAllDatasetIDs().Count();
        Print("[AskalCore] Total de datasets carregados: " + totalDatasets);
        
        Print("[AskalCore] ========================================");
        Print("[AskalCore] Core inicializado - Database pronto para uso");
        Print("[AskalCore] Market deve inicializar seus próprios módulos");
        Print("[AskalCore] ========================================");
    }
    
    // NOTA: Seguindo padrão TraderX - não enviamos dados automaticamente
    // O cliente solicita quando abre o menu via RequestDatasets RPC
    // Isso evita sobrecarga e permite controle melhor do timing
    
    // ITER-1: Flush pending saves on shutdown
    override void OnMissionFinish(Class sender, CF_EventArgs args)
    {
        AskalPlayerBalance.FlushAllPendingSaves();
        super.OnMissionFinish(sender, args);
    }
}

