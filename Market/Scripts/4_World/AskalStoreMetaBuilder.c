// ==========================================
// AskalStoreMetaBuilder - Builds storeMeta payload for client
// ==========================================

class AskalStoreItemEntry
{
	string itemClass;
	string displayName;
	bool stackable;
	int maxStack;
	int basePrice;
	int itemType; // 0=normal, 1=magazine, 2=stackable, 3=quantifiable
}

class AskalStoreCurrencyInfo
{
	string id;
	string shortName;
	int mode;
}

class AskalStorePermissions
{
	bool canBuy;
	bool canSell;
	bool batchMode;
}

class AskalStoreMeta
{
	string id;
	string name;
	string type; // "trader" or "virtual"
	ref AskalStoreCurrencyInfo currency;
	ref AskalStorePermissions permissions;
	ref array<ref AskalStoreItemEntry> items;
	float buyCoefficient;
	float sellCoefficient;
}

class AskalStoreMetaBuilder
{
	// Build storeMeta for a trader
	static AskalStoreMeta BuildStoreMetaForTrader(AskalTraderBase trader, PlayerBase player)
	{
		if (!trader || !player)
			return NULL;
		
		AskalStoreMeta storeMeta = new AskalStoreMeta();
		AskalTraderConfig traderConfig = trader.GetConfig();
		if (!traderConfig)
			return NULL;
		
		// Basic info
		storeMeta.id = traderConfig.TraderName;
		storeMeta.name = traderConfig.TraderName;
		storeMeta.type = "trader";
		
		// Resolve currency
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		string acceptedCurrency = AskalMarketHelpers.ResolveCurrencyForTrader(traderConfig, marketConfig);
		
		storeMeta.currency = new AskalStoreCurrencyInfo();
		storeMeta.currency.id = acceptedCurrency;
		
		if (marketConfig && acceptedCurrency != "")
		{
			AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(acceptedCurrency);
			if (currencyCfg)
			{
				storeMeta.currency.shortName = currencyCfg.ShortName;
				if (storeMeta.currency.shortName.Length() > 0 && storeMeta.currency.shortName.Substring(0, 1) == "@")
					storeMeta.currency.shortName = storeMeta.currency.shortName.Substring(1, storeMeta.currency.shortName.Length() - 1);
				storeMeta.currency.mode = currencyCfg.Mode;
			}
		}
		if (storeMeta.currency.shortName == "")
			storeMeta.currency.shortName = acceptedCurrency;
		
		Print("[AskalStoreMetaBuilder] Using currency " + acceptedCurrency + " (ShortName=" + storeMeta.currency.shortName + ") for store " + traderConfig.TraderName);
		
		// Permissions (default: both buy and sell allowed for traders)
		storeMeta.permissions = new AskalStorePermissions();
		storeMeta.permissions.canBuy = true;
		storeMeta.permissions.canSell = true;
		storeMeta.permissions.batchMode = false; // Can be extended later
		
		// Coefficients
		storeMeta.buyCoefficient = traderConfig.BuyCoefficient;
		if (storeMeta.buyCoefficient <= 0)
			storeMeta.buyCoefficient = 1.0;
		storeMeta.sellCoefficient = traderConfig.SellCoefficient;
		if (storeMeta.sellCoefficient <= 0)
			storeMeta.sellCoefficient = 1.0;
		
		// Items - resolve from SetupItems
		storeMeta.items = new array<ref AskalStoreItemEntry>();
		map<string, int> setupItems = trader.GetSetupItems();
		if (setupItems && setupItems.Count() > 0)
		{
			// Resolve SetupItems to concrete item classes
			// This is a simplified version - full resolution would iterate datasets
			// For now, we'll send SetupItems keys and let client resolve
			// TODO: Full item resolution from datasets
		}
		
		return storeMeta;
	}
	
	// Build storeMeta for VirtualStore
	static AskalStoreMeta BuildStoreMetaForVirtualStore(PlayerBase player)
	{
		if (!player)
			return NULL;
		
		AskalStoreMeta storeMeta = new AskalStoreMeta();
		AskalVirtualStoreConfig virtualStoreConfig = AskalVirtualStoreSettings.GetConfig();
		if (!virtualStoreConfig)
			return NULL;
		
		// Basic info
		storeMeta.id = "VirtualStore";
		storeMeta.name = "Virtual Store";
		storeMeta.type = "virtual";
		
		// Resolve currency from VirtualStore config (not global default)
		string acceptedCurrency = virtualStoreConfig.GetPrimaryCurrency();
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		
		if (!acceptedCurrency || acceptedCurrency == "")
		{
			// Fallback to default if VirtualStore doesn't specify
			if (marketConfig)
				acceptedCurrency = marketConfig.GetDefaultCurrencyId();
		}
		
		storeMeta.currency = new AskalStoreCurrencyInfo();
		storeMeta.currency.id = acceptedCurrency;
		
		if (marketConfig && acceptedCurrency != "")
		{
			AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(acceptedCurrency);
			if (currencyCfg)
			{
				storeMeta.currency.shortName = currencyCfg.ShortName;
				if (storeMeta.currency.shortName.Length() > 0 && storeMeta.currency.shortName.Substring(0, 1) == "@")
					storeMeta.currency.shortName = storeMeta.currency.shortName.Substring(1, storeMeta.currency.shortName.Length() - 1);
				storeMeta.currency.mode = currencyCfg.Mode;
			}
		}
		if (storeMeta.currency.shortName == "")
			storeMeta.currency.shortName = acceptedCurrency;
		
		Print("[AskalStoreMetaBuilder] Using currency " + acceptedCurrency + " (ShortName=" + storeMeta.currency.shortName + ") for store VirtualStore");
		
		// Permissions
		storeMeta.permissions = new AskalStorePermissions();
		storeMeta.permissions.canBuy = true;
		storeMeta.permissions.canSell = true;
		storeMeta.permissions.batchMode = false;
		
		// Coefficients
		storeMeta.buyCoefficient = virtualStoreConfig.BuyCoefficient;
		storeMeta.sellCoefficient = virtualStoreConfig.SellCoefficient;
		
		// Items - resolve from SetupItems
		storeMeta.items = new array<ref AskalStoreItemEntry>();
		if (virtualStoreConfig.SetupItems && virtualStoreConfig.SetupItems.Count() > 0)
		{
			// Resolve SetupItems to concrete item classes
			// TODO: Full item resolution from datasets
		}
		
		return storeMeta;
	}
	
	// Send storeMeta to client via RPC
	static void SendStoreMetaToClient(PlayerBase player, AskalStoreMeta storeMeta)
	{
		if (!player || !player.GetIdentity() || !storeMeta)
			return;
		
		// Convert storeMeta to RPC params
		// Using Param structure to send storeMeta data
		// Note: Enforce doesn't support complex nested structures in RPC, so we'll flatten
		
		string currencyId = "";
		string currencyShortName = "";
		int currencyMode = 0;
		if (storeMeta.currency)
		{
			currencyId = storeMeta.currency.id;
			currencyShortName = storeMeta.currency.shortName;
			currencyMode = storeMeta.currency.mode;
		}
		
		bool canBuy = false;
		bool canSell = false;
		bool batchMode = false;
		if (storeMeta.permissions)
		{
			canBuy = storeMeta.permissions.canBuy;
			canSell = storeMeta.permissions.canSell;
			batchMode = storeMeta.permissions.batchMode;
		}
		
		// Send via RPC (using Param structure)
		// Note: DayZ only supports up to Param8, so we'll use Param8 for first 8 params
		// Remaining params (batchMode, buyCoeff, sellCoeff) will be sent as defaults or in a follow-up RPC if needed
		// For now, use Param8 with essential data
		Param8<string, string, string, string, string, int, bool, bool> params = new Param8<string, string, string, string, string, int, bool, bool>(
			storeMeta.id,
			storeMeta.name,
			storeMeta.type,
			currencyId,
			currencyShortName,
			currencyMode,
			canBuy,
			canSell
		);
		
		GetRPCManager().SendRPC("AskalMarketModule", "SendStoreMeta", params, true, player.GetIdentity(), NULL);
		// Note: batchMode, buyCoeff, sellCoeff are not sent in this RPC due to Param8 limit
		// Client will use defaults: batchMode=false, buyCoeff=1.0, sellCoeff=1.0
		Print("[AskalStoreMetaBuilder] Sending storeMeta to player=" + player.GetIdentity().GetPlainId() + " storeId=" + storeMeta.id + " currency=" + currencyId + " shortName=" + currencyShortName);
	}
}

