class ASK_TraderVendingMachine extends House
{
	protected ref AskalTraderBase m_TraderLogic;
	protected string m_ConfigFileName;
	
	void ASK_TraderVendingMachine()
	{
		// Inicialização será feita via SetupTraderStatic do spawn service
		m_TraderLogic = NULL;
		m_ConfigFileName = "";
	}
	
	override void SetActions()
	{
		super.SetActions();
		
		Print("[ASK_TraderVendingMachine] SetActions() chamado");
		
		// Registrar ação de abrir menu do trader
		AddAction(ActionOpenAskalTraderMenu);
		
		Print("[ASK_TraderVendingMachine] Ação ActionOpenAskalTraderMenu adicionada");
	}
	
	// Definir arquivo de configuração (chamado pelo spawn service)
	void SetConfigFileName(string fileName)
	{
		m_ConfigFileName = fileName;
	}
	
	// Carregar configuração do trader
	void LoadTraderConfig(string fileName)
	{
		if (!GetGame().IsServer())
			return;
		
		// Carregar configuração
		AskalTraderConfig config = AskalTraderConfig.Load(fileName);
		if (!config)
		{
			Print("[ASK_TraderVendingMachine] ❌ Falha ao carregar config: " + fileName);
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
		
		Print("[ASK_TraderVendingMachine] ✅ Trader estático configurado: " + config.TraderName);
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

