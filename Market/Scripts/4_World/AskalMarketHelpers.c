// ==========================================
// AskalMarketHelpers - Funções auxiliares compartilhadas
// ==========================================

class AskalMarketHelpers
{
	// Obter PlayerBase de PlayerIdentity (versão melhorada com múltiplas estratégias)
	static PlayerBase GetPlayerFromIdentity(PlayerIdentity identity)
	{
		if (!identity)
		{
			Print("[AskalMarketHelpers] ⚠️ GetPlayerFromIdentity: identity é NULL");
			return NULL;
		}
		
		DayZGame game = DayZGame.Cast(GetGame());
		if (!game)
		{
			Print("[AskalMarketHelpers] ⚠️ GetPlayerFromIdentity: GetGame() retornou NULL");
			return NULL;
		}
		
		// Buscar na lista de players conectados
		array<Man> players = new array<Man>();
		World world = game.GetWorld();
		if (world)
		{
			world.GetPlayerList(players);
			
			Print("[AskalMarketHelpers] 🔍 Buscando player na lista (" + players.Count() + " players conectados)");
			
			foreach (Man man : players)
			{
				PlayerBase candidate = PlayerBase.Cast(man);
				if (candidate)
				{
					PlayerIdentity candidateIdentity = candidate.GetIdentity();
					if (candidateIdentity == identity)
					{
						Print("[AskalMarketHelpers] ✅ Player encontrado via GetPlayerList");
						return candidate;
					}
					
					// Comparação alternativa por SteamId (caso a referência de identity seja diferente)
					string identityId = identity.GetPlainId();
					string candidateId = candidateIdentity.GetPlainId();
					if (identityId != "" && candidateId != "" && identityId == candidateId)
					{
						Print("[AskalMarketHelpers] ✅ Player encontrado via comparação de SteamId");
						return candidate;
					}
				}
			}
		}
		
		Print("[AskalMarketHelpers] ❌ Player não encontrado para identity: " + identity.GetPlainId());
		return NULL;
	}
	
	// Obter display name do item (com fallback)
	static string GetItemDisplayName(string className)
	{
		if (!className || className == "")
			return "";
		
		string displayName = "";
		
		// Tentar CfgVehicles primeiro
		GetGame().ConfigGetText("CfgVehicles " + className + " displayName", displayName);
		
		// Se não encontrou, tentar CfgMagazines
		if (!displayName || displayName == "")
			GetGame().ConfigGetText("CfgMagazines " + className + " displayName", displayName);
		
		// Se ainda não encontrou, usar className
		if (!displayName || displayName == "")
			displayName = className;
		
		// Remover prefixos de tradução se existirem
		if (displayName.IndexOf("$STR_") == 0)
			displayName = Widget.TranslateString(displayName);
		
		return displayName;
	}
	
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
