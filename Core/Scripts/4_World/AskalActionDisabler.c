// ==========================================
// AskalActionDisabler - Desabilita ações indesejadas do jogo
// Remove ActionCheckPulseTarget para traders e opcionalmente para todos
// 
// IMPORTANTE: No DayZ Enforce Script, não existe RemoveAction().
// A única forma de remover ações é fazer override de SetActions()
// e NÃO chamar super.SetActions(), adicionando apenas as ações desejadas.
// ==========================================

class AskalActionDisabler
{
	// Verificar se uma entidade é trader
	static bool IsTrader(EntityAI entity)
	{
		if (!entity)
			return false;
		
		string className = entity.GetType();
		
		// Verificar se é máquina de vendas
		if (className == "ASK_TraderVendingMachine")
			return true;
		
		// Verificar se é trader humano
		if (className.IndexOf("ASK_Trader_") == 0)
			return true;
		
		// Verificar se tem lógica de trader
		AskalTraderBase traderLogic = AskalTraderSpawnService.GetTraderFromEntity(entity);
		if (traderLogic)
			return true;
		
		return false;
	}
	
	// Verificar se deve desabilitar ação de checar pulso
	// Retorna true se a ação deve ser desabilitada
	static bool ShouldDisableCheckPulse(EntityAI entity)
	{
		if (!entity)
			return false;
		
		// Sempre desabilitar para traders
		if (IsTrader(entity))
			return true;
		
		// Opcionalmente, pode desabilitar para todos os SurvivorBase
		// Descomente a linha abaixo se quiser desabilitar globalmente:
		// return true;
		
		return false;
	}
}

