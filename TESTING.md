# Testing Guide - Multi-Currency Store System

## Overview
This document describes manual testing steps and expected log lines for the multi-currency store system with automatic player balance bootstrap and storeMeta RPC.

## Prerequisites
- Server running with MarketConfig.json containing at least two currencies (e.g., Askal_Money, VirtualStore_Money)
- MarketConfig.json must have Mode, ShortName, StartCurrency for each currency
- Trader configs (e.g., Nizhnoe.json) with AcceptedCurrency specified
- VirtualStore_Config.json with AcceptedCurrency and SetupItems

## Test 1: Player JSON Creation on First Connect

### Steps:
1. Delete existing player JSON file: `$PROFILES/Askal/Database/Players/<steamId>.json`
2. Start server
3. Connect with a new player (or deleted player file)

### Expected Logs:
```
[MarketConfig] Loaded currencies: Askal_Money(mode=1), VirtualStore_Money(mode=2)
[AskalJsonLoader] Player config created for: <steamId> (currencies added: Askal_Money, VirtualStore_Money)
```

### Expected Player JSON:
```json
{
  "FirstLogin": "2025-12-01T15:56:34Z",
  "Balance": {
    "Askal_Money": 10000,
    "VirtualStore_Money": 10
  },
  "CreditCard": 0,
  "PremiumStatusExpireInHours": 0,
  "Permissions": {}
}
```

### Verification:
- Check that Balance contains all currencies from MarketConfig (excluding Mode=0)
- Check that each currency has its StartCurrency value
- Check that FirstLogin is set to current UTC time

## Test 2: Player JSON Update with New Currency

### Steps:
1. Add a new currency to MarketConfig.json (e.g., XmasEvent_Money with StartCurrency=100)
2. Restart server
3. Connect with existing player (who already has Askal_Money and VirtualStore_Money)

### Expected Logs:
```
[MarketConfig] Loaded currencies: Askal_Money(mode=1), VirtualStore_Money(mode=2), XmasEvent_Money(mode=1)
[AskalJsonLoader] Player config created/updated for: <steamId> (currencies added: XmasEvent_Money)
```

### Expected Player JSON:
- Existing balances unchanged
- New currency XmasEvent_Money added with value 100

## Test 3: Trader Menu Open (Nizhnoe)

### Steps:
1. Connect player
2. Open Nizhnoe trader menu

### Expected Logs (Server):
```
[AskalTrader] ✅ Abrindo menu do trader: MÁQUINA DE VENDAS
[AskalStoreMetaBuilder] Using currency Askal_Money for trader MÁQUINA DE VENDAS
[AskalStoreMetaBuilder] Sending storeMeta to player=<steamId> storeId=MÁQUINA DE VENDAS currency=Askal_Money shortName=AKC
```

### Expected Logs (Client):
```
[AskalMarket] 📥 SendStoreMeta recebido: storeId=MÁQUINA DE VENDAS storeName=MÁQUINA DE VENDAS currency=Askal_Money (AKC)
[AskalNotification] 🏪 StoreMeta recebido: MÁQUINA DE VENDAS currency=Askal_Money (AKC) canBuy=true canSell=true
[AskalStore] 🏪 OnShow: StoreMeta recebido - currency=Askal_Money (AKC) canBuy=true canSell=true
```

### Verification (Client UI):
- Widgets `currency_shortname_0`, `currency_shortname_1`, `currency_shortname_2` show "AKC"
- Both buy and sell buttons visible (if item allows both)
- Only buy button visible (if item is buy-only)
- Only sell button visible (if item is sell-only)

## Test 4: VirtualStore Menu Open

### Steps:
1. Connect player
2. Open VirtualStore menu

### Expected Logs (Server):
```
[AskalVirtualStoreConfig] ✅ Config loaded from: ... | AcceptedCurrency: VirtualStore_Money (mode=2) | SetupItems loaded: N
[AskalStoreMetaBuilder] Using currency VirtualStore_Money for VirtualStore
[AskalStoreMetaBuilder] Sending storeMeta to player=<steamId> storeId=VirtualStore currency=VirtualStore_Money shortName=VST
```

### Expected Logs (Client):
```
[AskalMarket] 📥 SendStoreMeta recebido: storeId=VirtualStore storeName=Virtual Store currency=VirtualStore_Money (VST)
[AskalNotification] 🏪 StoreMeta recebido: Virtual Store currency=VirtualStore_Money (VST) canBuy=true canSell=true
[AskalStore] 🏪 OnShow: StoreMeta recebido - currency=VirtualStore_Money (VST) canBuy=true canSell=true
```

### Verification (Client UI):
- Widgets show "VST" instead of "AKC"
- Items from VirtualStore.SetupItems are displayed
- Buttons visibility matches storeMeta permissions

## Test 5: Purchase with Stackable Item

### Steps:
1. Open Nizhnoe trader
2. Select stackable item (e.g., ammo_22)
3. Set slider quantity to 8
4. Click buy button

### Expected Logs (Client):
```
[AskalStore] 📤 RPC purchase sent: item=ammo_22 qty=8 unitPrice=1 total=8 currency=AKC
```

### Expected Logs (Server):
```
[AskalPurchase] Purchase request start for <steamId> | trader=MÁQUINA DE VENDAS | resolvedCurrency=Askal_Money (AKC)
[AskalPurchase] [COMPRA] Solicitação de compra recebida: Player: <steamId> Item: ammo_22 Preço solicitado: 8 Moeda: Askal_Money Quantidade (raw): 8 | Tipo: 2
[AskalPurchase] [INFO] Quantity accepted: 8 for ammo_22 (type: 2, stackable: true, ammo: false)
[AskalPurchase] [STACKABLE] Preço: 1 x 8 = 8
[AskalBalance] RESERVE_OK steamId=<steamId> amount=8 currency=Askal_Money (AKC) available_after=<balance-8>
[AskalPurchase] ORDER_PLACED steamId=<steamId> item=ammo_22 price=8 currency=Askal_Money qty=8
```

### Verification:
- Player receives 8 units of ammo_22
- Balance[Askal_Money] decreased by 8
- Log shows correct currency and quantity

## Test 6: Purchase with Missing Currency

### Steps:
1. Manually edit player JSON to remove VirtualStore_Money from Balance
2. Open VirtualStore
3. Attempt to purchase an item

### Expected Logs:
```
[AskalPurchase] Purchase request start for <steamId> | trader= | resolvedCurrency=VirtualStore_Money (VST)
[AskalPurchase] Currency not found in player balance, attempting refresh: VirtualStore_Money
[AskalJsonLoader] Player config created/updated for: <steamId> (currencies added: VirtualStore_Money)
[AskalPurchase] Purchase attempt steamId=<steamId> with currency=VirtualStore_Money amount=<price> → reserved ok
```

OR if refresh fails:
```
[AskalPurchase] Purchase rejected: player missing currency VirtualStore_Money
```

## Test 7: Purchase with Disabled Currency (Mode=0)

### Steps:
1. Set a currency Mode to 0 in MarketConfig.json
2. Restart server
3. Attempt to use that currency for purchase

### Expected Logs:
```
[AskalPurchase] ❌ Currency disabled: <currencyId>
```

## Test 8: Trader with Invalid AcceptedCurrency

### Steps:
1. Edit trader JSON to set AcceptedCurrency to a currency that doesn't exist in MarketConfig
2. Open that trader

### Expected Logs:
```
[AskalTrader] WARNING: Trader <name> AcceptedCurrency '<invalidId>' not found or disabled, using fallback '<fallbackId>'
[AskalStoreMetaBuilder] Using currency <fallbackId> for trader <name>
```

## Test 9: Backwards Compatibility (Old Client)

### Steps:
1. Use an old client that doesn't support SendStoreMeta RPC
2. Open trader menu

### Expected Behavior:
- Server sends both SendStoreMeta (new) and OpenTraderMenu (legacy)
- Client receives OpenTraderMenu and uses legacy fallback
- Client logs: `[AskalStore] 🏪 OnShow: Processando trader pendente (legacy): <traderName>`
- UI still works but uses legacy currency resolution

## Test 10: VirtualStore SetupItems Display

### Steps:
1. Configure VirtualStore_Config.json with SetupItems
2. Open VirtualStore menu

### Expected Behavior:
- Items matching SetupItems are displayed
- Items not in SetupItems are hidden
- Log shows: `[AskalVirtualStoreConfig] ✅ Config loaded from: ... | SetupItems loaded: N`

## Common Issues and Solutions

### Issue: "No currencies defined in MarketConfig"
**Solution**: Check that MarketConfig.json has Currencies array with at least one currency

### Issue: Currency shortname shows "@MarketConfig.json (44)"
**Solution**: Ensure ShortName is set in MarketConfig.json for that currency

### Issue: VirtualStore shows no items
**Solution**: Check that SetupItems in VirtualStore_Config.json is properly configured and items match dataset entries

### Issue: Wrong currency displayed for trader
**Solution**: Verify trader JSON has AcceptedCurrency set correctly and it exists in MarketConfig

## Log Reference

### Key Log Patterns:
- `[MarketConfig] Loaded currencies: ...` - MarketConfig loaded successfully
- `[AskalJsonLoader] Player config created for: ...` - New player file created
- `[AskalJsonLoader] Player config created/updated for: ...` - Existing player file updated
- `[AskalStoreMetaBuilder] Using currency ... for ...` - Currency resolved for store
- `[AskalStoreMetaBuilder] Sending storeMeta to player=...` - storeMeta sent to client
- `[AskalPurchase] Purchase request start for ...` - Purchase initiated
- `[AskalPurchase] ORDER_PLACED ...` - Purchase completed successfully
- `[AskalPurchase] Purchase rejected: player missing currency ...` - Purchase failed due to missing currency
- `[AskalBalance] RESERVE_OK ...` - Funds reserved successfully

