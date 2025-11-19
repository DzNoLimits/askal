// ==========================================
// AskalCoreHelpers - Funções auxiliares compartilhadas do Core
// ==========================================

class AskalCoreHelpers
{
	// Verificar se está rodando no servidor de forma segura (verifica NULL)
	static bool IsServerSafe()
	{
		DayZGame game = DayZGame.Cast(GetGame());
		if (!game)
			return false;
		return game.IsServer();
	}
	
	// Verificar se está rodando no cliente de forma segura (verifica NULL)
	static bool IsClientSafe()
	{
		DayZGame game = DayZGame.Cast(GetGame());
		if (!game)
			return false;
		return game.IsClient();
	}
}

