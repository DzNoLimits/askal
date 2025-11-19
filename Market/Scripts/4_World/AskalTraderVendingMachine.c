class AskalTraderVendingMachine extends House
{
	protected ref AskalTraderBase m_TraderLogic;
	protected string m_ConfigFileName;
	
	void AskalTraderVendingMachine()
	{
		// Inicialização será feita via SetupTraderStatic do spawn service
		m_TraderLogic = NULL;
		m_ConfigFileName = "";
	}
	
	override void SetActions()
	{
		super.SetActions();
		
		Print("[AskalTraderVendingMachine] SetActions() chamado");
		
		// Registrar ação de abrir menu do trader
		AddAction(ActionOpenAskalTraderMenu);
		
		Print("[AskalTraderVendingMachine] Ação ActionOpenAskalTraderMenu adicionada");
	}
	
	// Definir arquivo de configuração (chamado pelo spawn service)
	void SetConfigFileName(string fileName)
	{
		m_ConfigFileName = fileName;
	}
	
	// Carregar configuração do trader
	void LoadTraderConfig(string fileName)
	{
		if (!AskalMarketHelpers.IsServerSafe())
			return;
		
		// Carregar configuração
		AskalTraderConfig config = AskalTraderConfig.Load(fileName);
		if (!config)
		{
			Print("[AskalTraderVendingMachine] ❌ Falha ao carregar config: " + fileName);
			return;
		}
		
		// Criar lógica do trader
		m_TraderLogic = new AskalTraderBase();
		m_TraderLogic.SetTraderEntity(this);
		m_TraderLogic.LoadConfig(config);
		
		// Marcar como trader
		AskalTraderSpawnService.MarkAsTrader(this, m_TraderLogic);
		
		// Adicionar à lista de traders spawnados
		AskalTraderSpawnService.AddTraderToList(m_TraderLogic);
		
		Print("[AskalTraderVendingMachine] ✅ Trader estático configurado: " + config.TraderName);
	}
	
	// Obter lógica do trader
	AskalTraderBase GetTraderLogic()
	{
		return m_TraderLogic;
	}
	
	// Verificar se é trader
	bool IsAskalTrader()
	{
		return m_TraderLogic != NULL;
	}
}

