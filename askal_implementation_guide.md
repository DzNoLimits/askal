# Askal Critical Fixes - Implementation Guide

This document provides detailed implementation steps for the critical security fixes identified in the audit.

## 1. Reservation Pattern Implementation

The reservation pattern prevents TOCTOU vulnerabilities by reserving balance before item creation.

### Add to `AskalPlayerBalance.c`:

```c
// Add after existing class members
private static ref map<string, ref map<string, int>> s_Reservations = new map<string, ref map<string, int>>();

// Reserve balance (doesn't deduct yet)
static bool ReserveBalance(string steamId, int amount, string currency = "Askal_Coin")
{
    if (amount <= 0)
        return false;
    
    Lock lock = GetOrCreateLock(steamId);
    lock.Lock();
    
    AskalPlayerData playerData = GetCachedOrLoad(steamId);
    if (!playerData || !playerData.Balance)
    {
        lock.Unlock();
        return false;
    }
    
    int currentBalance = 0;
    if (playerData.Balance.Contains(currency))
        currentBalance = playerData.Balance.Get(currency);
    
    // Check available balance (current - reserved)
    int reserved = GetReservedAmount(steamId, currency);
    int available = currentBalance - reserved;
    
    if (available < amount)
    {
        lock.Unlock();
        return false;
    }
    
    // Add to reservations
    if (!s_Reservations.Contains(steamId))
        s_Reservations.Set(steamId, new map<string, int>());
    
    map<string, int> playerReservations = s_Reservations.Get(steamId);
    int currentReserved = 0;
    if (playerReservations.Contains(currency))
        currentReserved = playerReservations.Get(currency);
    
    playerReservations.Set(currency, currentReserved + amount);
    
    lock.Unlock();
    return true;
}

// Get reserved amount for a currency
private static int GetReservedAmount(string steamId, string currency)
{
    if (!s_Reservations.Contains(steamId))
        return 0;
    
    map<string, int> playerReservations = s_Reservations.Get(steamId);
    if (!playerReservations.Contains(currency))
        return 0;
    
    return playerReservations.Get(currency);
}

// Confirm reservation (actually deduct balance)
static bool ConfirmReservation(string steamId, int amount, string currency = "Askal_Coin")
{
    Lock lock = GetOrCreateLock(steamId);
    lock.Lock();
    
    // Verify reservation exists
    if (!s_Reservations.Contains(steamId))
    {
        lock.Unlock();
        return false;
    }
    
    map<string, int> playerReservations = s_Reservations.Get(steamId);
    if (!playerReservations.Contains(currency))
    {
        lock.Unlock();
        return false;
    }
    
    int reserved = playerReservations.Get(currency);
    if (reserved < amount)
    {
        lock.Unlock();
        return false;
    }
    
    // Remove from reservations
    playerReservations.Set(currency, reserved - amount);
    if (playerReservations.Get(currency) == 0)
        playerReservations.Remove(currency);
    
    // Actually deduct balance
    AskalPlayerData playerData = GetCachedOrLoad(steamId);
    int currentBalance = playerData.Balance.Get(currency);
    playerData.Balance.Set(currency, currentBalance - amount);
    
    lock.Unlock();
    return SavePlayerData(steamId, playerData);
}

// Release reservation (rollback)
static bool ReleaseReservation(string steamId, int amount, string currency = "Askal_Coin")
{
    Lock lock = GetOrCreateLock(steamId);
    lock.Lock();
    
    if (!s_Reservations.Contains(steamId))
    {
        lock.Unlock();
        return false;
    }
    
    map<string, int> playerReservations = s_Reservations.Get(steamId);
    if (!playerReservations.Contains(currency))
    {
        lock.Unlock();
        return false;
    }
    
    int reserved = playerReservations.Get(currency);
    if (reserved < amount)
    {
        lock.Unlock();
        return false;
    }
    
    playerReservations.Set(currency, reserved - amount);
    if (playerReservations.Get(currency) == 0)
        playerReservations.Remove(currency);
    
    lock.Unlock();
    return true;
}
```

## 2. Lock Implementation

DayZ EnforceScript doesn't have a built-in `Lock` class. You need to implement a simple mutex using a boolean flag:

```c
class SimpleLock
{
    private bool m_Locked = false;
    private string m_Owner = "";
    
    void Lock()
    {
        int attempts = 0;
        while (m_Locked && attempts < 1000)
        {
            Sleep(1); // Wait 1ms
            attempts++;
        }
        
        if (m_Locked)
        {
            Print("[SimpleLock] WARNING: Lock timeout!");
            return;
        }
        
        m_Locked = true;
        m_Owner = "Thread_" + GetGame().GetTime().ToString();
    }
    
    void Unlock()
    {
        m_Locked = false;
        m_Owner = "";
    }
    
    bool IsLocked()
    {
        return m_Locked;
    }
}

// Update GetOrCreateLock to use SimpleLock
private static ref map<string, ref SimpleLock> s_PlayerLocks = new map<string, ref SimpleLock>();

private static SimpleLock GetOrCreateLock(string steamId)
{
    if (!s_PlayerLocks.Contains(steamId))
    {
        s_PlayerLocks.Set(steamId, new SimpleLock());
    }
    return s_PlayerLocks.Get(steamId);
}
```

**Note:** EnforceScript is single-threaded, so locks are mainly for preventing re-entrancy and ensuring atomic operations. The real protection comes from the atomic check-then-modify pattern.

## 3. Batch Purchase Atomicity

Update `PurchaseBatchRequest` in `AskalPurchaseModule.c`:

```c
void PurchaseBatchRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
{
    // ... existing validation ...
    
    // Calculate total cost
    int totalCost = 0;
    foreach (AskalPurchaseRequestData request : requests)
    {
        int itemPrice = AskalPurchaseService.ComputeItemTotalPriceWithQuantity(
            request.ItemClass, request.Quantity, request.QuantityType);
        totalCost += itemPrice;
    }
    
    // Pre-validate balance
    if (!AskalPlayerBalance.HasEnoughBalance(steamId, totalCost, currencyId))
    {
        SendPurchaseResponse(sender, false, "", 0, "Insufficient balance for batch");
        return;
    }
    
    // Reserve balance for entire batch
    if (!AskalPlayerBalance.ReserveBalance(steamId, totalCost, currencyId))
    {
        SendPurchaseResponse(sender, false, "", 0, "Failed to reserve balance");
        return;
    }
    
    // Process all items
    array<EntityAI> createdItems = new array<EntityAI>();
    bool allSuccess = true;
    
    for (int i = 0; i < requests.Count(); i++)
    {
        AskalPurchaseRequestData request = requests.Get(i);
        EntityAI item = AskalPurchaseService.CreateSimpleItem(
            GetPlayerFromIdentity(sender), request.ItemClass, 
            request.Quantity, request.QuantityType, request.ContentType);
        
        if (item)
        {
            createdItems.Insert(item);
        }
        else
        {
            allSuccess = false;
            Print("[AskalPurchase] Failed to create item: " + request.ItemClass);
            break;
        }
    }
    
    if (!allSuccess)
    {
        // Rollback: delete all created items
        foreach (EntityAI item : createdItems)
        {
            if (item)
                GetGame().ObjectDelete(item);
        }
        
        // Release reservation
        AskalPlayerBalance.ReleaseReservation(steamId, totalCost, currencyId);
        SendPurchaseResponse(sender, false, "", 0, "Batch purchase failed - items rolled back");
    }
    else
    {
        // Confirm reservation
        AskalPlayerBalance.ConfirmReservation(steamId, totalCost, currencyId);
        SendPurchaseResponse(sender, true, "", totalCost, "Batch purchase successful");
    }
}
```

## 4. Cache Implementation Details

The cache implementation should:

1. **Load on first access** - Cache miss triggers disk read
2. **Update on write** - SavePlayerData updates cache
3. **TTL expiration** - Cache entries expire after 30 seconds
4. **Manual invalidation** - ClearCache() for admin operations

```c
// In GetCachedOrLoad:
private static AskalPlayerData GetCachedOrLoad(string steamId)
{
    float now = GetGame().GetTime();
    
    // Check cache
    if (s_Cache.Contains(steamId))
    {
        float cacheTime = s_CacheTimestamps.Get(steamId);
        if (now - cacheTime < CACHE_TTL)
        {
            return s_Cache.Get(steamId);
        }
        else
        {
            // Cache expired, remove it
            s_Cache.Remove(steamId);
            s_CacheTimestamps.Remove(steamId);
        }
    }
    
    // Cache miss - load from disk
    AskalPlayerData data = LoadPlayerData(steamId);
    if (data)
    {
        s_Cache.Set(steamId, data);
        s_CacheTimestamps.Set(steamId, now);
    }
    
    return data;
}
```

## 5. Flush on Server Shutdown

Add to `MissionServer.c` or create a shutdown handler:

```c
override void OnMissionFinish(Class sender, CF_EventArgs args)
{
    // Flush all pending balance saves
    AskalPlayerBalance.FlushAllPendingSaves();
    
    super.OnMissionFinish(sender, args);
}
```

## 6. Testing the Fixes

After implementing the fixes, run the test scenarios from `askal_testplan.yml`:

1. **Race Condition Test**: Should now prevent double-spend
2. **TOCTOU Test**: Should prevent balance deduction without item creation
3. **Batch Test**: Should be atomic (all succeed or all fail)
4. **Rate Limit Test**: Should reject requests exceeding limit
5. **Performance Test**: Should maintain FPS with caching

## 7. Migration Path

1. **Phase 1**: Add locks and caching (non-breaking)
2. **Phase 2**: Add reservation pattern (requires purchase flow update)
3. **Phase 3**: Add rate limiting (non-breaking)
4. **Phase 4**: Add batch atomicity (requires batch handler update)

## 8. Rollback Plan

If issues occur:
1. Disable new code paths via feature flags
2. Revert to synchronous I/O (temporary)
3. Keep locks active (prevents race conditions)

## 9. Monitoring

Add logging for:
- Cache hit/miss rates
- Lock contention
- Reservation failures
- Rate limit hits

```c
// Example logging
Print("[AskalBalance] Cache hit: " + steamId);
Print("[AskalBalance] Cache miss: " + steamId);
Print("[AskalBalance] Lock acquired: " + steamId);
Print("[AskalBalance] Rate limit hit: " + steamId);
```

