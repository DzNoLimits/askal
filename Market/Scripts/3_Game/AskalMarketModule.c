// ==========================================
// AskalMarketModule - Módulo CF do Market
// Registra RPCs de compra/venda, health e trader menu
// ==========================================

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
            string traderName = data5.param1;
            ref array<string> setupKeys = data5.param2;
            ref array<int> setupValues = data5.param3;
            string acceptedCurrency = data5.param4;
            string currencyShortName = data5.param5;
            
            Print("[AskalMarket] 📥 OpenTraderMenu recebido para trader: " + traderName + " | Currency: " + acceptedCurrency + " | ShortName: " + currencyShortName);
            
            // Contar entradas do SetupItems
            int setupCount = 0;
            if (setupKeys)
                setupCount = setupKeys.Count();
            Print("[AskalMarket] 📦 SetupItems: " + setupCount.ToString() + " entradas");
            
            // Converter arrays de volta para map
            ref map<string, int> setupItems = new map<string, int>();
            if (setupKeys && setupValues && setupKeys.Count() == setupValues.Count())
            {
                for (int i = 0; i < setupKeys.Count(); i++)
                {
                    string key = setupKeys.Get(i);
                    int value = setupValues.Get(i);
                    setupItems.Set(key, value);
                }
            }
            
            // Armazenar no helper para que o menu possa acessar quando for criado (incluindo AcceptedCurrency e ShortName)
            AskalNotificationHelper.RequestOpenTraderMenu(traderName, setupItems, acceptedCurrency, currencyShortName);
            
            Print("[AskalMarket] ✅ Trader menu request armazenado, aguardando criação do menu");
            return;
        }
        
        // Fallback: Try Param4 (old format without shortname)
        ctx.Rewind();
        Param4<string, ref array<string>, ref array<int>, string> data4;
        if (ctx.Read(data4))
        {
            string traderName = data4.param1;
            ref array<string> setupKeys = data4.param2;
            ref array<int> setupValues = data4.param3;
            string acceptedCurrency = data4.param4;
            
            Print("[AskalMarket] 📥 OpenTraderMenu recebido (legacy format) para trader: " + traderName + " | Currency: " + acceptedCurrency);
            
            // Resolve shortname from MarketConfig
            string currencyShortName = "";
            AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
            if (marketConfig && acceptedCurrency != "")
            {
                AskalCurrencyConfig currencyCfg = marketConfig.GetCurrencyOrNull(acceptedCurrency);
                if (currencyCfg && currencyCfg.ShortName != "")
                {
                    currencyShortName = currencyCfg.ShortName;
                    if (currencyShortName.Length() > 0 && currencyShortName.Substring(0, 1) == "@")
                        currencyShortName = currencyShortName.Substring(1, currencyShortName.Length() - 1);
                }
            }
            if (currencyShortName == "")
                currencyShortName = acceptedCurrency;
            
            // Contar entradas do SetupItems
            int setupCount = 0;
            if (setupKeys)
                setupCount = setupKeys.Count();
            Print("[AskalMarket] 📦 SetupItems: " + setupCount.ToString() + " entradas");
            
            // Converter arrays de volta para map
            ref map<string, int> setupItems = new map<string, int>();
            if (setupKeys && setupValues && setupKeys.Count() == setupValues.Count())
            {
                for (int i = 0; i < setupKeys.Count(); i++)
                {
                    string key = setupKeys.Get(i);
                    int value = setupValues.Get(i);
                    setupItems.Set(key, value);
                }
            }
            
            // Armazenar no helper
            AskalNotificationHelper.RequestOpenTraderMenu(traderName, setupItems, acceptedCurrency, currencyShortName);
            
            Print("[AskalMarket] ✅ Trader menu request armazenado, aguardando criação do menu");
            return;
        }
        
        Print("[AskalMarket] ❌ Erro ao ler OpenTraderMenu");
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

