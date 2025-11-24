// ==========================================
// AskalMarketModuleConstructor - Registra módulos Mission do Market
// ==========================================

modded class JMModuleConstructor
{
	override void RegisterModules( out TTypenameArray modules )
	{
		Print("[AskalMarket] ========================================");
		Print("[AskalMarket] 🔍 AskalMarketModuleConstructor.RegisterModules() chamado");
		
		super.RegisterModules( modules );
		
		// Registrar módulo Mission do Market para input bindings
		Print("[AskalMarket] 📝 Registrando AskalMarketMissionModule...");
		modules.Insert( AskalMarketMissionModule );
		
		Print("[AskalMarket] ✅ AskalMarketMissionModule registrado no JMModuleConstructor");
		Print("[AskalMarket] ========================================");
	}
};

