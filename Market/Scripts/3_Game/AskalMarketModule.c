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
        
        // Converter arrays de volta para map e normalizar chaves (adicionar prefixos CAT_/DS_ se necessário)
        ref map<string, int> setupItems = new map<string, int>();
        if (setupKeys && setupValues && setupKeys.Count() == setupValues.Count())
        {
            for (int i = 0; i < setupKeys.Count(); i++)
            {
                string key = setupKeys.Get(i);
                int value = setupValues.Get(i);
                
                // Normalizar chave: adicionar prefixos CAT_ ou DS_ se necessário
                string normalizedKey = NormalizeSetupItemKey(key);
                setupItems.Set(normalizedKey, value);
                
                Print("[AskalMarket] 📦 SetupItem: " + key + " → " + normalizedKey + " = " + value);
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
    
    // Normalizar chave do SetupItems: adicionar prefixos CAT_ ou DS_ se necessário
    // Prioridade: Item individual > Categoria > Dataset
    // Categorias geralmente começam com número (ex: "2_Drinks")
    // Datasets geralmente têm formato específico ou começam com letras
    // "ALL" permanece como está
    static string NormalizeSetupItemKey(string key)
    {
        if (!key || key == "")
            return key;
        
        // "ALL" permanece como está
        if (key == "ALL")
            return key;
        
        // Se já tem prefixo, retornar como está
        if (key.IndexOf("CAT_") == 0 || key.IndexOf("DS_") == 0)
            return key;
        
        // Verificar se é categoria (geralmente começa com número seguido de underscore)
        // Exemplos: "2_Drinks", "1_Food", "3_Weapons"
        // Padrão: número + underscore + texto
        bool isCategory = false;
        if (key.Length() > 2)
        {
            string firstChar = key.Substring(0, 1);
            string secondChar = key.Substring(1, 1);
            
            // Verificar se começa com número (0-9) seguido de underscore
            // Usar comparação de string para evitar problemas com ToInt()
            bool isDigit = false;
            if (firstChar == "0")
                isDigit = true;
            else if (firstChar == "1")
                isDigit = true;
            else if (firstChar == "2")
                isDigit = true;
            else if (firstChar == "3")
                isDigit = true;
            else if (firstChar == "4")
                isDigit = true;
            else if (firstChar == "5")
                isDigit = true;
            else if (firstChar == "6")
                isDigit = true;
            else if (firstChar == "7")
                isDigit = true;
            else if (firstChar == "8")
                isDigit = true;
            else if (firstChar == "9")
                isDigit = true;
            
            if (isDigit && secondChar == "_")
            {
                isCategory = true;
            }
        }
        
        if (isCategory)
        {
            return "CAT_" + key;
        }
        
        // Verificar se é dataset (geralmente começa com letras ou tem formato específico)
        // Por padrão, se não é categoria e não é "ALL", assumir que é dataset
        // Mas vamos ser mais conservadores: apenas adicionar DS_ se parecer um dataset
        // Por enquanto, vamos assumir que se não é categoria, pode ser dataset ou item individual
        // Items individuais (classnames) geralmente começam com letra maiúscula e não têm underscore no início
        // Datasets podem ter formato variado
        
        // Verificar se provavelmente é item individual (classname)
        // Items individuais geralmente começam com letra e não têm underscore no início
        // Se não começa com número (já verificamos acima) e não tem underscore, provavelmente é item
        bool hasUnderscore = (key.IndexOf("_") != -1);
        
        if (!hasUnderscore)
        {
            // Provavelmente é item individual (classname), retornar como está
            return key;
        }
        
        // Caso contrário, assumir que é dataset e adicionar prefixo DS_
        return "DS_" + key;
    }
}

