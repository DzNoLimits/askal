# StoreMeta RPC Documentation

## Overview
The `SendStoreMeta` RPC provides a unified server→client payload for store/trader metadata, including currency information, permissions, and item lists. This ensures consistent UI rendering and eliminates client-side currency resolution logic.

## RPC Details

### Server → Client: `SendStoreMeta`

**Module**: `AskalMarketModule`  
**Direction**: Server → Client  
**Type**: `SingleplayerExecutionType.Client`

### Parameters
The RPC uses `Param11` with the following structure:

```cpp
Param11<string, string, string, string, string, int, bool, bool, bool, float, float>
```

1. `storeId` (string) - Unique identifier for the store (trader name or "VirtualStore")
2. `storeName` (string) - Display name for the store
3. `storeType` (string) - "trader" or "virtual"
4. `currencyId` (string) - Currency ID (e.g., "Askal_Money", "VirtualStore_Money")
5. `currencyShortName` (string) - Currency shortname (e.g., "AKC", "VST")
6. `currencyMode` (int) - Currency mode: 0=Disabled, 1=Physical, 2=Virtual
7. `canBuy` (bool) - Whether buy operations are allowed
8. `canSell` (bool) - Whether sell operations are allowed
9. `batchMode` (bool) - Whether batch operations are enabled
10. `buyCoefficient` (float) - Buy price multiplier
11. `sellCoefficient` (float) - Sell price multiplier

## StoreMeta Schema (Conceptual)

The server builds a `AskalStoreMeta` object internally, which is then flattened for RPC transmission:

```cpp
class AskalStoreMeta
{
    string id;                    // storeId
    string name;                  // storeName
    string type;                  // "trader" or "virtual"
    AskalStoreCurrencyInfo currency;
    AskalStorePermissions permissions;
    array<AskalStoreItemEntry> items;  // Future: resolved items
    float buyCoefficient;
    float sellCoefficient;
}

class AskalStoreCurrencyInfo
{
    string id;                    // currencyId
    string shortName;            // currencyShortName
    int mode;                    // currencyMode
}

class AskalStorePermissions
{
    bool canBuy;                 // canBuy
    bool canSell;                // canSell
    bool batchMode;              // batchMode
}
```

## When StoreMeta is Sent

### Trader Menu Open
When a player opens a trader menu (via `ActionOpenAskalTraderMenu`):
1. Server resolves trader's `AcceptedCurrency` using `ResolveCurrencyForTrader()`
2. Server builds `storeMeta` using `AskalStoreMetaBuilder.BuildStoreMetaForTrader()`
3. Server sends `SendStoreMeta` RPC to client
4. Server also sends legacy `OpenTraderMenu` RPC for backwards compatibility

### VirtualStore Menu Open
When a player requests VirtualStore config (via `RequestVirtualStoreConfig`):
1. Server resolves VirtualStore's `AcceptedCurrency` from config (not global default)
2. Server builds `storeMeta` using `AskalStoreMetaBuilder.BuildStoreMetaForVirtualStore()`
3. Server sends `SendStoreMeta` RPC to client
4. Server also sends legacy `VirtualStoreConfigResponse` RPC for backwards compatibility

## Client-Side Handling

### RPC Handler
The client receives `SendStoreMeta` in `AskalMarketModule.SendStoreMeta()`:
1. Extracts all parameters from `Param11`
2. Stores in `AskalNotificationHelper.SetStoreMeta()`
3. Logs receipt: `[AskalMarket] 📥 SendStoreMeta recebido: ...`

### UI Update
When `AskalStoreMenu.OnShow()` is called:
1. Checks for `storeMeta` via `AskalNotificationHelper.HasStoreMeta()`
2. If present:
   - Updates `m_ActiveCurrencyId` and `m_CurrentCurrencyShortName`
   - Calls `UpdateCurrencyShortnameWidgets()` to populate widgets
   - Calls `UpdateActionButtonsFromStoreMeta()` to set button visibility
   - Clears `storeMeta` after use
3. If not present:
   - Falls back to legacy `OpenTraderMenu` handling
   - Logs: `[AskalStore] 🏪 OnShow: Processando trader pendente (legacy): ...`

## Currency Resolution Priority

The server resolves currency in this order:
1. **Trader**: `traderConfig.AcceptedCurrency` (if trader is active)
2. **VirtualStore**: `virtualStoreConfig.GetPrimaryCurrency()` (if VirtualStore is active)
3. **Default**: `marketConfig.GetDefaultCurrencyId()`
4. **Fallback**: First available currency in MarketConfig

This ensures each store uses its own currency configuration, not a global default.

## Button Visibility Logic

Based on `storeMeta.permissions`:

- `canBuy && canSell`: Show both buy and sell buttons (grouped layout)
- `canBuy && !canSell`: Show only buy button (single layout)
- `!canBuy && canSell`: Show only sell button (single layout)
- `!canBuy && !canSell`: Hide all buttons, show warning

## Backwards Compatibility

If the client doesn't support `SendStoreMeta`:
- Server still sends legacy `OpenTraderMenu` RPC
- Client uses existing currency resolution logic
- UI works but may show incorrect currency if resolution fails
- Server logs: `[AskalStore] WARN - server payload missing` (if detectable)

## Example Flow

### Trader Open (Nizhnoe)
```
Server:
  [AskalTrader] ✅ Abrindo menu do trader: MÁQUINA DE VENDAS
  [AskalStoreMetaBuilder] Using currency Askal_Money for trader MÁQUINA DE VENDAS
  [AskalStoreMetaBuilder] Sending storeMeta to player=765611... storeId=MÁQUINA DE VENDAS currency=Askal_Money shortName=AKC

Client:
  [AskalMarket] 📥 SendStoreMeta recebido: storeId=MÁQUINA DE VENDAS currency=Askal_Money (AKC)
  [AskalStore] 🏪 OnShow: StoreMeta recebido - currency=Askal_Money (AKC) canBuy=true canSell=true
  [AskalStore] UI UpdateActionButtonsFromStoreMeta: canBuy=true canSell=true showDual=true
```

### VirtualStore Open
```
Server:
  [AskalStoreMetaBuilder] Using currency VirtualStore_Money for VirtualStore
  [AskalStoreMetaBuilder] Sending storeMeta to player=765611... storeId=VirtualStore currency=VirtualStore_Money shortName=VST

Client:
  [AskalMarket] 📥 SendStoreMeta recebido: storeId=VirtualStore currency=VirtualStore_Money (VST)
  [AskalStore] 🏪 OnShow: StoreMeta recebido - currency=VirtualStore_Money (VST) canBuy=true canSell=true
```

## Future Enhancements

- **Item Resolution**: Currently `items` array is empty. Future implementation will resolve `SetupItems` to concrete item classes and send full item metadata.
- **Batch Mode**: `batchMode` flag is currently hardcoded to `false`. Future implementation will enable batch operations based on store configuration.

