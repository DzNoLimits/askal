// ==========================================
// AskalTraderBase - Lógica central do trader
// Gerencia configuração e estado do trader
// ==========================================

class AskalTraderBase
{
	protected ref AskalTraderConfig m_Config;
	protected EntityAI m_TraderEntity;
	
	void AskalTraderBase()
	{
	}
	
	// Definir entidade do trader
	void SetTraderEntity(EntityAI entity)
	{
		m_TraderEntity = entity;
	}
	
	// Obter entidade do trader
	EntityAI GetTraderEntity()
	{
		return m_TraderEntity;
	}
	
	// Carregar configuração
	void LoadConfig(AskalTraderConfig config)
	{
		m_Config = config;
	}
	
	// Obter configuração
	AskalTraderConfig GetConfig()
	{
		return m_Config;
	}
	
	// Verificar se é trader
	bool IsTrader()
	{
		return true;
	}
	
	// Obter nome do trader
	string GetTraderName()
	{
		if (m_Config)
			return m_Config.TraderName;
		return "";
	}
	
	// Obter moedas aceitas
	map<string, int> GetAcceptedCurrencies()
	{
		if (m_Config && m_Config.AcceptedCurrencyMap)
			return m_Config.AcceptedCurrencyMap;
		return new map<string, int>();
	}
	
	// Obter setup de itens
	map<string, int> GetSetupItems()
	{
		if (m_Config && m_Config.SetupItems)
			return m_Config.SetupItems;
		return new map<string, int>();
	}
}

