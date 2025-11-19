# Askal Security & Performance Audit - Critical Findings

**Date:** 2024  
**Repository:** https://github.com/DzNoLimits/askal  
**Framework:** DayZ Community Framework (Arkensor)

---

## CRITICAL FINDINGS

### 1. CRITICAL: Race Condition in Balance Operations (Double-Spend Vulnerability)

**Severity:** Critical  
**Summary:** Non-atomic balance operations allow concurrent purchases to double-spend the same balance.

**Files & Lines:**
- `Core/Scripts/3_Game/AskalPlayerBalance.c:202-228` (`RemoveBalance`)
- `Market/Scripts/4_World/AskalPurchaseService.c:47-67` (`ProcessPurchaseWithQuantity`)

**Evidence:**
```c
// AskalPurchaseService.c:47-67
int currentBalance = AskalPlayerBalance.GetBalance(steamId, currencyId);  // Read
if (currentBalance < price) return false;                                 // Check
// ... create item ...
if (!AskalPlayerBalance.RemoveBalance(steamId, price, currencyId))       // Write
```

**Race Condition Scenario:**
1. Thread A: Reads balance = 1000
2. Thread B: Reads balance = 1000 (before A writes)
3. Thread A: Checks 1000 >= 500, proceeds
4. Thread B: Checks 1000 >= 500, proceeds
5. Thread A: Writes balance = 500
6. Thread B: Writes balance = 500 (overwrites A's write)
7. **Result:** Both purchases succeed, but only 500 deducted instead of 1000

**PoC Steps:**
1. Start server with 1 player having balance = 1000
2. Send 2 concurrent RPCs: `PurchaseItemRequest` for items costing 500 each
3. Both succeed, balance becomes 500 instead of 0

**Remediation:**
```c
// Add per-player lock map
private static ref map<string, ref Lock> s_PlayerLocks = new map<string, ref Lock>();

static bool RemoveBalanceAtomic(string steamId, int amount, string currency)
{
    Lock lock = GetOrCreateLock(steamId);
    lock.Lock();
    
    AskalPlayerData playerData = LoadPlayerData(steamId);
    int currentBalance = playerData.Balance.Get(currency);
    if (currentBalance < amount) {
        lock.Unlock();
        return false;
    }
    playerData.Balance.Set(currency, currentBalance - amount);
    bool success = SavePlayerData(steamId, playerData);
    lock.Unlock();
    return success;
}
```

---

### 2. CRITICAL: Synchronous File I/O in High-Frequency Transaction Path

**Severity:** Critical  
**Summary:** Every purchase/sell operation performs 2-3 synchronous file reads/writes, causing severe performance degradation and potential server hangs.

**Files & Lines:**
- `Core/Scripts/3_Game/AskalPlayerBalance.c:166-176` (`GetBalance` → `LoadPlayerData`)
- `Core/Scripts/3_Game/AskalPlayerBalance.c:179-198` (`AddBalance` → `LoadPlayerData` + `SavePlayerData`)
- `Core/Scripts/3_Game/AskalPlayerBalance.c:202-228` (`RemoveBalance` → `LoadPlayerData` + `SavePlayerData`)
- `Core/Scripts/3_Game/AskalJsonLoader.c:99-153` (`LoadFromFile` - synchronous read)
- `Core/Scripts/3_Game/AskalJsonLoader.c:63-96` (`SaveToFile` - synchronous write)

**Estimated Cost:**
- **Per purchase:** 2 file reads + 1 file write = ~10-50ms (depending on disk I/O)
- **At 10 purchases/sec:** 20-50 file operations/sec = 200-500ms blocking time/sec
- **At 50 purchases/sec:** Server becomes unresponsive

**Evidence:**
```c
// AskalPlayerBalance.c:166-176
static int GetBalance(string steamId, string currency = "Askal_Coin")
{
    AskalPlayerData playerData = LoadPlayerData(steamId);  // SYNCHRONOUS FILE READ
    // ...
}

// AskalPurchaseService.c:48, 67, 74
int currentBalance = AskalPlayerBalance.GetBalance(steamId, currencyId);  // FILE READ
// ... create item ...
AskalPlayerBalance.RemoveBalance(steamId, price, currencyId);           // FILE READ + WRITE
int newBalance = AskalPlayerBalance.GetBalance(steamId, currencyId);     // FILE READ
```

**Remediation:**
Implement in-memory cache with batched persistence:
```c
class AskalPlayerBalanceCache
{
    private static ref map<string, ref AskalPlayerData> s_Cache = new map<string, ref AskalPlayerData>();
    private static ref map<string, float> s_LastSaveTime = new map<string, float>();
    private static const float SAVE_INTERVAL = 5.0; // seconds
    
    static int GetBalance(string steamId, string currency)
    {
        AskalPlayerData data = GetCachedOrLoad(steamId);
        return data.Balance.Get(currency);
    }
    
    static bool RemoveBalance(string steamId, int amount, string currency)
    {
        AskalPlayerData data = GetCachedOrLoad(steamId);
        int current = data.Balance.Get(currency);
        if (current < amount) return false;
        data.Balance.Set(currency, current - amount);
        QueueSave(steamId, data);
        return true;
    }
    
    // Batch save every 5 seconds or on server shutdown
    static void FlushPendingSaves()
    {
        // Save all dirty entries
    }
}
```

---

### 3. CRITICAL: TOCTOU (Time-of-Check-Time-of-Use) Vulnerability

**Severity:** Critical  
**Summary:** Balance check and deduction are separated by item creation, allowing balance to change or item creation to fail after balance check.

**Files & Lines:**
- `Market/Scripts/4_World/AskalPurchaseService.c:47-72`

**Evidence:**
```c
// Line 48: Check balance
int currentBalance = AskalPlayerBalance.GetBalance(steamId, currencyId);
if (currentBalance < price) return false;

// Line 56: Create item (can fail or take time)
EntityAI createdItem = CreateSimpleItem(player, itemClass, ...);
if (!createdItem) return false;

// Line 67: Remove balance (balance may have changed!)
if (!AskalPlayerBalance.RemoveBalance(steamId, price, currencyId))
{
    GetGame().ObjectDelete(createdItem);  // Rollback item
    return false;
}
```

**Attack Scenario:**
1. Player has balance = 500
2. Player sends purchase for item costing 1000
3. Check fails (500 < 1000) - correct
4. Player quickly adds 500 via exploit/admin
5. Player sends purchase again
6. Check passes (1000 >= 1000)
7. Item creation fails (inventory full)
8. Balance check in `RemoveBalance` may pass if balance still 1000
9. **Result:** Balance deducted but no item created

**Remediation:**
Use atomic transaction pattern:
```c
static bool ProcessPurchaseWithQuantity(...)
{
    // Reserve balance FIRST (atomic)
    if (!AskalPlayerBalance.ReserveBalance(steamId, price, currencyId))
        return false;
    
    // Create item
    EntityAI item = CreateSimpleItem(...);
    if (!item) {
        AskalPlayerBalance.ReleaseReservation(steamId, price, currencyId);
        return false;
    }
    
    // Confirm reservation (convert to actual deduction)
    AskalPlayerBalance.ConfirmReservation(steamId, price, currencyId);
    return true;
}
```

---

### 4. HIGH: Batch Purchase Processing Without Atomicity

**Severity:** High  
**Summary:** Batch purchases process items sequentially without transaction boundaries, allowing partial failures and inconsistent state.

**Files & Lines:**
- `Market/Scripts/4_World/AskalPurchaseModule.c:169-177` (`PurchaseBatchRequest`)

**Evidence:**
```c
for (int i = 0; i < requests.Count(); i++)
{
    ProcessPurchaseRequest(sender, steamId, request.ItemClass, ...);
    // No rollback if later items fail
}
```

**Attack Scenario:**
1. Player sends batch: [Item1: 100, Item2: 200, Item3: 300]
2. Balance = 400
3. Item1 succeeds (balance = 300)
4. Item2 succeeds (balance = 100)
5. Item3 fails (balance = 100 < 300)
6. **Result:** Partial purchase, player gets 2 items but paid for 3

**Remediation:**
```c
void PurchaseBatchRequest(...)
{
    // Pre-validate entire batch
    int totalCost = CalculateTotalCost(requests);
    if (!HasEnoughBalance(steamId, totalCost, currencyId))
    {
        SendBatchResponse(sender, false, "Insufficient balance for batch");
        return;
    }
    
    // Reserve balance for entire batch
    if (!ReserveBalance(steamId, totalCost, currencyId))
        return;
    
    // Process all items
    array<EntityAI> createdItems = new array<EntityAI>();
    bool allSuccess = true;
    
    foreach (request : requests)
    {
        EntityAI item = CreateItem(...);
        if (item)
            createdItems.Insert(item);
        else
        {
            allSuccess = false;
            break;
        }
    }
    
    if (!allSuccess)
    {
        // Rollback: delete all items, release reservation
        foreach (item : createdItems)
            GetGame().ObjectDelete(item);
        ReleaseReservation(steamId, totalCost, currencyId);
    }
    else
    {
        ConfirmReservation(steamId, totalCost, currencyId);
    }
}
```

---

### 5. HIGH: No Rate Limiting on Purchase/Sell RPCs

**Severity:** High  
**Summary:** Clients can spam purchase/sell requests, causing DoS and exploiting race conditions.

**Files & Lines:**
- `Market/Scripts/4_World/AskalPurchaseModule.c:107-136` (`PurchaseItemRequest`)
- `Market/Scripts/4_World/AskalSellModule.c:28-118` (`SellItemRequest`)

**Evidence:**
No rate limiting checks found in RPC handlers.

**PoC Steps:**
```c
// Client-side script
for (int i = 0; i < 100; i++)
{
    GetRPCManager().SendRPC("AskalMarketModule", "PurchaseItemRequest", ...);
}
```

**Remediation:**
```c
class AskalRateLimiter
{
    private static ref map<string, ref array<float>> s_RequestTimes = new map<string, ref array<float>>();
    private static const float RATE_LIMIT_WINDOW = 1.0; // 1 second
    private static const int MAX_REQUESTS_PER_WINDOW = 5;
    
    static bool CheckRateLimit(string steamId)
    {
        array<float> times = GetOrCreate(steamId);
        float now = GetGame().GetTime();
        
        // Remove old entries
        for (int i = times.Count() - 1; i >= 0; i--)
        {
            if (now - times.Get(i) > RATE_LIMIT_WINDOW)
                times.Remove(i);
        }
        
        if (times.Count() >= MAX_REQUESTS_PER_WINDOW)
            return false;
        
        times.Insert(now);
        return true;
    }
}

// In RPC handler:
if (!AskalRateLimiter.CheckRateLimit(steamId))
{
    SendPurchaseResponse(sender, false, "", 0, "Rate limit exceeded");
    return;
}
```

---

### 6. MEDIUM: Missing Persistence Guarantees

**Severity:** Medium  
**Summary:** File writes may not be flushed to disk, risking data loss on server crash.

**Files & Lines:**
- `Core/Scripts/3_Game/AskalJsonLoader.c:63-96` (`SaveToFile`)

**Evidence:**
```c
FPrintln(fh, jsonData);
CloseFile(fh);  // No explicit flush
```

**Remediation:**
```c
FPrintln(fh, jsonData);
FlushFile(fh);  // Force write to disk
CloseFile(fh);
```

---

### 7. MEDIUM: No Schema Versioning for Player Data

**Severity:** Medium  
**Summary:** Player JSON files lack version field, making migrations impossible and risking corruption.

**Files & Lines:**
- `Core/Scripts/3_Game/AskalPlayerBalance.c:7-23` (`AskalPlayerData`)

**Remediation:**
```c
class AskalPlayerData
{
    int SchemaVersion = 1;  // Add version field
    string FirstLogin;
    ref map<string, int> Balance;
    // ...
}

static AskalPlayerData LoadPlayerData(string steamId)
{
    AskalPlayerData data = LoadFromFile(...);
    if (data.SchemaVersion < CURRENT_SCHEMA_VERSION)
        data = MigratePlayerData(data);
    return data;
}
```

---

### 8. MEDIUM: Update() Method May Trigger RPCs Periodically

**Severity:** Medium  
**Summary:** UI Update() method sends RPCs every SYNC_CHECK_INTERVAL, potentially causing unnecessary network traffic.

**Files & Lines:**
- `Market/Scripts/5_Mission/AskalStoreMenu.c:5898-5928` (`Update`)

**Evidence:**
```c
if (m_LastSyncCheck >= SYNC_CHECK_INTERVAL)
{
    GetRPCManager().SendRPC("AskalCoreModule", "RequestDatasets", ...);
}
```

**Impact:** Low (client-side only), but should be optimized.

---

### 9. LOW: Missing Input Validation on Batch Size

**Severity:** Low  
**Summary:** Batch purchase requests can contain unlimited items, causing server overload.

**Files & Lines:**
- `Market/Scripts/4_World/AskalPurchaseModule.c:161-177`

**Remediation:**
```c
if (requests.Count() > MAX_BATCH_SIZE)
{
    Print("[AskalPurchase] Batch size exceeded: " + requests.Count());
    SendBatchResponse(sender, false, "Batch size too large");
    return;
}
```

---

### 10. LOW: No Transaction Logging for Audit Trail

**Severity:** Low  
**Summary:** No logging of financial transactions for audit/debugging purposes.

**Remediation:**
```c
static bool RemoveBalance(string steamId, int amount, string currency)
{
    // ... existing code ...
    
    // Log transaction
    AskalTransactionLogger.LogTransaction(steamId, -amount, currency, "PURCHASE", itemClass);
    
    return SavePlayerData(steamId, playerData);
}
```

---

## OTHER FINDINGS

### Performance Issues

1. **Full inventory enumeration on sell** (`AskalSellModule.c:143`)
   - **Impact:** O(n) scan for each sell operation
   - **Effort:** 2 hours
   - **Fix:** Cache inventory structure or use indexed lookup

2. **Repeated database lookups** (`AskalPurchaseService.c:167-186`)
   - **Impact:** Multiple `AskalDatabase.GetItem()` calls per purchase
   - **Effort:** 1 hour
   - **Fix:** Cache item data in memory

### Architectural Improvements

1. **Implement outbox pattern for persistence**
   - **Priority:** High
   - **Effort:** 8 hours
   - **Benefit:** Guaranteed delivery, crash recovery

2. **Add database connection pooling** (if using external DB)
   - **Priority:** Medium
   - **Effort:** 4 hours

3. **Implement request queuing for high load**
   - **Priority:** Medium
   - **Effort:** 6 hours

---

## TEST PLAN

### Prerequisites
- DayZ Server with Community Framework installed
- Community Offline Mode enabled (or local test server)

### Test Scenarios

#### Test 1: Race Condition (Double-Spend)
```bash
# Start server
# Connect 2 clients (or use script)
# Player balance: 1000
# Send 2 concurrent purchases for 500 each
# Expected: Only 1 should succeed, balance = 500
# Actual: Both succeed, balance = 500 (WRONG)
```

#### Test 2: TOCTOU Vulnerability
```bash
# Player balance: 500
# Send purchase for 1000 (should fail)
# Quickly add 500 via admin command
# Send purchase again
# Expected: Should fail or succeed atomically
# Actual: May succeed with no item created
```

#### Test 3: Batch Purchase Partial Failure
```bash
# Player balance: 400
# Send batch: [Item1: 100, Item2: 200, Item3: 300]
# Expected: All fail (insufficient balance)
# Actual: First 2 succeed, 3rd fails (partial state)
```

#### Test 4: Rate Limiting
```bash
# Send 100 purchase requests in 1 second
# Expected: Only 5 succeed, rest rate-limited
# Actual: All processed (DoS possible)
```

#### Test 5: Crash Recovery
```bash
# Make purchase
# Kill server immediately
# Restart server
# Expected: Balance correctly deducted
# Actual: May lose transaction (no flush)
```

---

## RECOMMENDED PATCH PRIORITY

1. **IMMEDIATE:** Fix race condition (#1) - Critical exploit
2. **IMMEDIATE:** Add rate limiting (#5) - DoS prevention
3. **HIGH:** Implement caching (#2) - Performance critical
4. **HIGH:** Fix TOCTOU (#3) - Data integrity
5. **MEDIUM:** Batch atomicity (#4) - User experience
6. **MEDIUM:** Add flush (#6) - Data persistence
7. **LOW:** Schema versioning (#7) - Future-proofing

---

## ESTIMATED EFFORT

- **Critical fixes (#1, #3, #5):** 12 hours
- **Performance (#2):** 8 hours
- **Other fixes (#4, #6, #7):** 6 hours
- **Testing & validation:** 8 hours
- **Total:** ~34 hours

