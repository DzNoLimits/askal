// ==========================================
// AskalMarketModule - Módulo CF do Market
// Registra RPCs de compra/venda, health e trader menu
// ==========================================

// Forward declarations to fix Unknown type errors
class array;

[CF_RegisterModule(AskalMarketModule)]
class AskalMarketModule : CF_ModuleGame
{
    void AskalMarketModule()
    {
        CF_Log.Info("[AskalMarket] Módulo Askal Market inicializado");
    }

    override void OnInit()
    {
        super.OnInit();
        
        Print("[AskalMarket] ========================================");
        Print("[AskalMarket] Inicializando módulo Market");
        
        // Habilitar eventos do CF
        EnableMissionStart();
        EnableMissionFinish();
        
        // Registrar RPCs de compra/venda
        AddLegacyRPC("PurchaseItemRequest", SingleplayerExecutionType.Server);
        AddLegacyRPC("PurchaseBatchRequest", SingleplayerExecutionType.Server);
        AddLegacyRPC("PurchaseItemResponse", SingleplayerExecutionType.Client);
        
        // Registrar RPCs de venda
        AddLegacyRPC("SellItemRequest", SingleplayerExecutionType.Server);
        AddLegacyRPC("SellItemResponse", SingleplayerExecutionType.Client);
        
        // RPC para health dos itens do inventário
        AddLegacyRPC("RequestInventoryHealth", SingleplayerExecutionType.Server);
        AddLegacyRPC("InventoryHealthResponse", SingleplayerExecutionType.Client);
        
        // RPC para abrir menu do trader
        AddLegacyRPC("OpenTraderMenu", SingleplayerExecutionType.Client);
        
        // RPC para enviar storeMeta (new)
        AddLegacyRPC("SendStoreMeta", SingleplayerExecutionType.Client);
        
        Print("[AskalMarket] ✅ RPCs do Market registrados");
        Print("[AskalMarket] ========================================");
    }
    
    // RPC Handler: Cliente recebe resposta de compra
    void PurchaseItemResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;
        
        Param4<bool, string, string, int> data;
        if (!ctx.Read(data))
        {
            Print("[AskalMarket] ❌ Erro ao ler PurchaseItemResponse");
            return;
        }
        
        bool success = data.param1;
        string message = data.param2;
        string itemClass = data.param3;
        int price = data.param4;
        
        if (success)
        {
            Print("[AskalMarket] ✅ " + message);
            // Adicionar notificação visual de compra via helper
            if (itemClass != "" && price > 0)
            {
                AskalNotificationHelper.AddPurchaseNotification(itemClass, price);
            }
        }
        else
        {
            Print("[AskalMarket] ❌ " + message);
            // Adicionar erro ao helper para o menu processar
            AskalNotificationHelper.AddTransactionError(message);
        }
    }
    
    // RPC Handler: Cliente recebe resposta de venda
    void SellItemResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;
        
        Param4<bool, string, string, int> data;
        if (!ctx.Read(data))
        {
            Print("[AskalMarket] ❌ Erro ao ler SellItemResponse");
            return;
        }
        
        bool success = data.param1;
        string message = data.param2;
        string itemClass = data.param3;
        int price = data.param4;
        
        string statusIcon = "[ERRO]";
        if (success)
            statusIcon = "[OK]";
        Print("[AskalMarket] SellItemResponse recebida: " + statusIcon + " " + message);
        
        if (success)
        {
            Print("[AskalMarket] [OK] " + message);
            // Adicionar notificação visual de venda no cliente
            if (itemClass != "" && price > 0)
            {
                // Obter display name do item para a notificação
                string itemDisplayName = "";
                GetGame().ConfigGetText("CfgVehicles " + itemClass + " displayName", itemDisplayName);
                if (!itemDisplayName || itemDisplayName == "")
                    GetGame().ConfigGetText("CfgMagazines " + itemClass + " displayName", itemDisplayName);
                if (!itemDisplayName || itemDisplayName == "")
                    itemDisplayName = itemClass;
                
                // Remover prefixos de tradução se existirem
                if (itemDisplayName.IndexOf("$STR_") == 0)
                    itemDisplayName = Widget.TranslateString(itemDisplayName);
                
                AskalNotificationHelper.AddSellNotification(itemClass, price, itemDisplayName);
                Print("[AskalMarket] 📢 Notificação de venda adicionada no cliente: " + itemDisplayName + " ($" + price.ToString() + ")");
            }
        }
        else
        {
            Print("[AskalMarket] [ERRO] " + message);
        }
    }
    
    // RPC Handler: Cliente recebe health dos itens do inventário
    void InventoryHealthResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;
        
        Param1<ref array<ref Param2<string, float>>> data;
        if (!ctx.Read(data))
        {
            Print("[AskalMarket] ❌ Erro ao ler InventoryHealthResponse");
            return;
        }
        
        ref array<ref Param2<string, float>> healthArray = data.param1;
        if (!healthArray)
        {
            Print("[AskalMarket] ⚠️ InventoryHealthResponse recebido sem dados");
            return;
        }
        
        Print("[AskalMarket] 📥 InventoryHealthResponse recebido com " + healthArray.Count() + " itens");
        
        // Armazenar health no helper
        AskalNotificationHelper.SetInventoryHealth(healthArray);
    }
    
    // RPC Handler: Cliente recebe comando para abrir menu do trader
    void OpenTraderMenu(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;
        
        // Try Param5 first (with shortname), fallback to Param4 for compatibility
        Param5<string, ref array<string>, ref array<int>, string, string> data5;
        if (ctx.Read(data5))
        {
            string traderName5 = data5.param1;
            ref array<string> setupKeys5 = data5.param2;
            ref array<int> setupValues5 = data5.param3;
            string acceptedCurrency5 = data5.param4;
            string currencyShortName5 = data5.param5;
            
            Print("[AskalMarket] 📥 OpenTraderMenu recebido para trader: " + traderName5 + " | Currency: " + acceptedCurrency5 + " | ShortName: " + currencyShortName5);
            
            // Contar entradas do SetupItems
            int setupCount5 = 0;
            if (setupKeys5)
                setupCount5 = setupKeys5.Count();
            Print("[AskalMarket] 📦 SetupItems: " + setupCount5.ToString() + " entradas");
            
            // Converter arrays de volta para map
            ref map<string, int> setupItems5 = new map<string, int>();
            if (setupKeys5 && setupValues5 && setupKeys5.Count() == setupValues5.Count())
            {
                for (int i5 = 0; i5 < setupKeys5.Count(); i5++)
                {
                    string key5 = setupKeys5.Get(i5);
                    int value5 = setupValues5.Get(i5);
                    setupItems5.Set(key5, value5);
                }
            }
            
            // Armazenar no helper para que o menu possa acessar quando for criado (incluindo AcceptedCurrency e ShortName)
            AskalNotificationHelper.RequestOpenTraderMenu(traderName5, setupItems5, acceptedCurrency5, currencyShortName5);
            
            Print("[AskalMarket] ✅ Trader menu request armazenado, aguardando criação do menu");
            return;
        }
        
        // Fallback: Try Param4 (old format without shortname)
        // Note: ParamsReadContext doesn't have Rewind(), so we need to read directly
        Param4<string, ref array<string>, ref array<int>, string> data4;
        if (ctx.Read(data4))
        {
            string traderName4 = data4.param1;
            ref array<string> setupKeys4 = data4.param2;
            ref array<int> setupValues4 = data4.param3;
            string acceptedCurrency4 = data4.param4;
            
            Print("[AskalMarket] 📥 OpenTraderMenu recebido (legacy format) para trader: " + traderName4 + " | Currency: " + acceptedCurrency4);
            
            // Resolve shortname from MarketConfig
            string currencyShortName4 = "";
            AskalMarketConfig marketConfig4 = AskalMarketConfig.GetInstance();
            if (marketConfig4 && acceptedCurrency4 != "")
            {
                AskalCurrencyConfig currencyCfg4 = marketConfig4.GetCurrencyOrNull(acceptedCurrency4);
                if (currencyCfg4 && currencyCfg4.ShortName != "")
                {
                    currencyShortName4 = currencyCfg4.ShortName;
                    if (currencyShortName4.Length() > 0 && currencyShortName4.Substring(0, 1) == "@")
                        currencyShortName4 = currencyShortName4.Substring(1, currencyShortName4.Length() - 1);
                }
            }
            if (currencyShortName4 == "")
                currencyShortName4 = acceptedCurrency4;
            
            // Contar entradas do SetupItems
            int setupCount4 = 0;
            if (setupKeys4)
                setupCount4 = setupKeys4.Count();
            Print("[AskalMarket] 📦 SetupItems: " + setupCount4.ToString() + " entradas");
            
            // Converter arrays de volta para map
            ref map<string, int> setupItems4 = new map<string, int>();
            if (setupKeys4 && setupValues4 && setupKeys4.Count() == setupValues4.Count())
            {
                for (int i4 = 0; i4 < setupKeys4.Count(); i4++)
                {
                    string key4 = setupKeys4.Get(i4);
                    int value4 = setupValues4.Get(i4);
                    setupItems4.Set(key4, value4);
                }
            }
            
            // Armazenar no helper
            AskalNotificationHelper.RequestOpenTraderMenu(traderName4, setupItems4, acceptedCurrency4, currencyShortName4);
            
            Print("[AskalMarket] ✅ Trader menu request armazenado, aguardando criação do menu");
            return;
        }
        
        Print("[AskalMarket] ❌ Erro ao ler OpenTraderMenu");
    }
    
    // RPC Handler: Cliente recebe storeMeta
    void SendStoreMeta(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;
        
        // Param11 doesn't exist in DayZ - use individual reads or Param structure
        // For now, read as individual parameters using a workaround
        // Note: DayZ supports up to Param8, so we'll need to split this or use a different approach
        // Using Param8 for first 8 params, then read remaining manually if needed
        Param8<string, string, string, string, string, int, bool, bool> data8;
        if (!ctx.Read(data8))
        {
            Print("[AskalMarket] ❌ Erro ao ler SendStoreMeta (Param8)");
            return;
        }
        
        string storeId = data8.param1;
        string storeName = data8.param2;
        string storeType = data8.param3;
        string currencyId = data8.param4;
        string currencyShortName = data8.param5;
        int currencyMode = data8.param6;
        bool canBuy = data8.param7;
        bool canSell = data8.param8;
        
        // Read remaining params manually (batchMode, buyCoeff, sellCoeff)
        // Since we can't use Param11, we'll need to read these separately or modify the RPC
        // For now, use defaults and log a warning
        bool batchMode = false;
        float buyCoeff = 1.0;
        float sellCoeff = 1.0;
        
        Print("[AskalMarket] ⚠️ SendStoreMeta: Param11 not supported, using defaults for batchMode/buyCoeff/sellCoeff");
        
        Print("[AskalMarket] 📥 SendStoreMeta recebido: storeId=" + storeId + " storeName=" + storeName + " currency=" + currencyId + " (" + currencyShortName + ")");
        
        // Store in helper for UI to consume
        AskalNotificationHelper.SetStoreMeta(storeId, storeName, storeType, currencyId, currencyShortName, currencyMode, canBuy, canSell, batchMode, buyCoeff, sellCoeff);
    }
    
    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        Print("[AskalMarket] ========================================");
        Print("[AskalMarket] OnMissionStart() → Market inicializado");
        Print("[AskalMarket] ========================================");
    }
    
    override void OnMissionFinish(Class sender, CF_EventArgs args)
    {
        CF_Log.Info("[AskalMarket] OnMissionFinish()");
    }
}

