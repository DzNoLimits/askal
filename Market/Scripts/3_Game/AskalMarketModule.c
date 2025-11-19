// ==========================================
// AskalMarketModule - Módulo CF do Market
// Registra RPCs de compra/venda, health e trader menu
// ==========================================

// Forward declarations
class Param1<Class T1>;
class Param2<Class T1, Class T2>;
class Param3<Class T1, Class T2, Class T3>;
class Param4<Class T1, Class T2, Class T3, Class T4>;

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
        
        Param3<string, ref array<string>, ref array<int>> data;
        if (!ctx.Read(data))
        {
            Print("[AskalMarket] ❌ Erro ao ler OpenTraderMenu");
            return;
        }
        
        string traderName = data.param1;
        ref array<string> setupKeys = data.param2;
        ref array<int> setupValues = data.param3;
        
        Print("[AskalMarket] 📥 OpenTraderMenu recebido para trader: " + traderName);
        
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
        
        // Armazenar no helper para que o menu possa acessar quando for criado
        AskalNotificationHelper.RequestOpenTraderMenu(traderName, setupItems);
        
        Print("[AskalMarket] ✅ Trader menu request armazenado, aguardando criação do menu");
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

