// ==========================================
// AskalMarketHelpers - Funções auxiliares compartilhadas
// ==========================================

// Player metadata for market operations
class PlayerMeta
{
	bool playerConfigValid;
	string configLoadReason;
	
	void PlayerMeta()
	{
		playerConfigValid = false;
		configLoadReason = "";
	}
}

class AskalMarketHelpers
{
	private static ref map<string, ref PlayerMeta> s_PlayerMeta = new map<string, ref PlayerMeta>();
	private static ref map<string, bool> s_LoadLocks = new map<string, bool>();
	
	// Get or create PlayerMeta for a steamId
	static PlayerMeta GetPlayerMeta(string steamId)
	{
		if (!steamId || steamId == "")
			return NULL;
		
		if (!s_PlayerMeta.Contains(steamId))
			s_PlayerMeta.Set(steamId, new PlayerMeta());
		
		return s_PlayerMeta.Get(steamId);
	}
	
	// Set player config validation state
	static void SetPlayerConfigValid(string steamId, bool valid, string reason = "")
	{
		PlayerMeta meta = GetPlayerMeta(steamId);
		if (meta)
		{
			meta.playerConfigValid = valid;
			meta.configLoadReason = reason;
		}
	}
	
	// Check if player config is valid
	static bool IsPlayerConfigValid(string steamId)
	{
		PlayerMeta meta = GetPlayerMeta(steamId);
		if (!meta)
			return false;
		return meta.playerConfigValid;
	}
	
	// Get config load reason
	static string GetConfigLoadReason(string steamId)
	{
		PlayerMeta meta = GetPlayerMeta(steamId);
		if (!meta)
			return "";
		return meta.configLoadReason;
	}
	
	// Try to acquire load lock (returns true if acquired, false if already locked)
	static bool TryAcquireLoadLock(string steamId)
	{
		if (!steamId || steamId == "")
			return false;
		
		if (s_LoadLocks.Contains(steamId) && s_LoadLocks.Get(steamId))
			return false; // Already locked
		
		s_LoadLocks.Set(steamId, true);
		return true;
	}
	
	// Release load lock
	static void ReleaseLoadLock(string steamId)
	{
		if (!steamId || steamId == "")
			return;
		
		s_LoadLocks.Set(steamId, false);
	}
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
	
	// Resolve currency ID for an item (optional: can be extended to support per-item currency)
	// For now, returns default currency from config
	static string ResolveItemCurrency(string itemClass = "", string traderName = "")
	{
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		if (!marketConfig)
			return AskalMarketConstants.DEFAULT_CURRENCY_ID;
		
		// Future: Check if item config specifies a currency
		// Future: Check if trader config overrides currency
		
		// For now, return default currency
		return marketConfig.GetDefaultCurrencyId();
	}
	
	// Resolve currency for trader/VirtualStore (prioritizes trader-specific, then VirtualStore, then default)
	// Returns currency ID and logs resolution
	static string ResolveCurrencyForTrader(AskalTraderConfig traderConfig, AskalMarketConfig fallbackMarketConfig)
	{
		AskalMarketConfig marketConfig = fallbackMarketConfig;
		if (!marketConfig)
			marketConfig = AskalMarketConfig.GetInstance();
		
		if (!marketConfig)
		{
			Print("[AskalMarketHelpers] ERROR: MarketConfig not available");
			return AskalMarketConstants.DEFAULT_CURRENCY_ID;
		}
		
		string resolvedCurrency = "";
		string traderName = "";
		int currencyMode = -1;
		
		// PRIORITY 1: Trader-specific AcceptedCurrency
		if (traderConfig && traderConfig.AcceptedCurrency && traderConfig.AcceptedCurrency != "")
		{
			string traderCurrency = traderConfig.AcceptedCurrency;
			traderName = traderConfig.TraderName;
			
			// Validate currency exists in MarketConfig
			AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(traderCurrency);
			if (currencyCfg)
			{
				resolvedCurrency = traderCurrency;
				currencyMode = currencyCfg.Mode;
				Print("[AskalMarketHelpers] Resolved currency for trader " + traderName + " => " + resolvedCurrency + " (mode=" + currencyMode + ")");
				return resolvedCurrency;
			}
			else
			{
				Print("[AskalMarketHelpers] ERROR: Currency " + traderCurrency + " not defined in MarketConfig for trader " + traderName);
			}
		}
		
		// PRIORITY 2: VirtualStore AcceptedCurrency
		AskalVirtualStoreConfig virtualStoreConfig = AskalVirtualStoreSettings.GetConfig();
		if (virtualStoreConfig)
		{
			string virtualCurrency = virtualStoreConfig.GetPrimaryCurrency();
			if (virtualCurrency && virtualCurrency != "")
			{
				// Validate currency exists in MarketConfig
				AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(virtualCurrency);
				if (currencyCfg)
				{
					resolvedCurrency = virtualCurrency;
					currencyMode = currencyCfg.Mode;
					Print("[AskalMarketHelpers] Resolved currency for VirtualStore => " + resolvedCurrency + " (mode=" + currencyMode + ")");
					return resolvedCurrency;
				}
				else
				{
					Print("[AskalMarketHelpers] ERROR: Currency " + virtualCurrency + " not defined in MarketConfig for VirtualStore");
				}
			}
		}
		
		// PRIORITY 3: MarketConfig default
		resolvedCurrency = marketConfig.GetDefaultCurrencyId();
		if (resolvedCurrency && resolvedCurrency != "")
		{
			AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(resolvedCurrency);
			if (currencyCfg)
				currencyMode = currencyCfg.Mode;
			Print("[AskalMarketHelpers] Resolved currency using default => " + resolvedCurrency + " (mode=" + currencyMode + ")");
			return resolvedCurrency;
		}
		
		// FALLBACK: First available currency
		array<string> allKeys = marketConfig.GetAllCurrencyKeys();
		if (allKeys.Count() > 0)
		{
			resolvedCurrency = allKeys.Get(0);
			AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(resolvedCurrency);
			if (currencyCfg)
				currencyMode = currencyCfg.Mode;
			Print("[AskalMarketHelpers] Resolved currency using first available => " + resolvedCurrency + " (mode=" + currencyMode + ")");
			return resolvedCurrency;
		}
		
		// LAST RESORT: Hardcoded default
		Print("[AskalMarketHelpers] ERROR: No currencies available, using hardcoded default");
		return AskalMarketConstants.DEFAULT_CURRENCY_ID;
	}
	
	// Call this on player connect/OnStoreLoad to load/create/migrate player config
	// This should be called from 4_World context (has access to PlayerMeta)
	static void OnPlayerConnect(string steamId)
	{
		if (!steamId || steamId == "")
			return;
		
		// Load or create player config synchronously
		AskalPlayerData playerData;
		bool success;
		string reason;
		AskalPlayerConfigLoader.LoadOrCreatePlayerConfigSync(steamId, playerData, success, reason);
		
		if (success && playerData)
		{
			// Initialize in-memory balances
			AskalPlayerBalance.InitializePlayerBalances(steamId, playerData);
			// Mark as valid
			SetPlayerConfigValid(steamId, true, reason);
			AskalPlayerBalance.SetPlayerConfigValidInternal(steamId, true);
			Print("[AskalMarketHelpers] Player " + steamId + " config loaded/created successfully on connect");
		}
		else
		{
			// Mark as invalid
			SetPlayerConfigValid(steamId, false, reason);
			AskalPlayerBalance.SetPlayerConfigValidInternal(steamId, false);
			Print("[AskalMarketHelpers] Player " + steamId + " config load/create failed on connect — reason: " + reason);
		}
	}
}
