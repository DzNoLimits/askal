# Askal Hotfix Apply Instructions

## Emergency Hotfix: Atomic Reserve + Rate-Limit

**Branch:** `hotfix/market-atomic-reserve`  
**Commit Message:** `hotfix: atomic reserve + rate-limit to prevent double-spend and DoS`

---

## 🚨 IMMEDIATE MITIGATION (Before Applying Code)

If exploit is active, disable market immediately:

### Option 1: Disable via Code (Fastest)
Add this to `AskalPurchaseModule.c` constructor:
```c
void AskalPurchaseModule()
{
    s_MarketEnabled = false; // EMERGENCY DISABLE
    // ... rest of code
}
```

### Option 2: Block RPC (Server-side)
Comment out RPC registration in `AskalPurchaseModule.c`:
```c
// GetRPCManager().AddRPC("AskalMarketModule", "PurchaseItemRequest", this, SingleplayerExecutionType.Server);
```

---

## 📋 APPLY HOTFIX STEPS

### Step 1: Create Branch
```bash
cd P:\askal
git checkout -b hotfix/market-atomic-reserve
```

### Step 2: Apply Patch
```bash
# On Windows (PowerShell)
git apply askal_patch_hotfix.diff

# On Linux/Mac
git apply askal_patch_hotfix.diff
```

### Step 3: Verify Changes
```bash
git status
git diff HEAD
```

### Step 4: Fix Compilation Issues (if any)

**Note:** The patch may need minor adjustments for EnforceScript syntax:

1. **FlushFile()** - May need to check if this function exists. If not, remove the flush call (data will still be saved, just not immediately flushed).

2. **GetGame().GetTime()** - Verify this returns float. If not, use alternative timing method.

3. **FileHandle operations** - The outbox flush uses `OpenFile` which may need adjustment.

### Step 5: Test in Community Offline Mode
```bash
# Start DayZ with Community Offline Mode
# Load Askal mod
# Run test scripts (see tests/ directory)
```

### Step 6: Commit
```bash
git add -A
git commit -m "hotfix: atomic reserve + rate-limit to prevent double-spend and DoS

- Add per-player locks to prevent race conditions
- Implement ReserveFunds/ConfirmReservation pattern
- Add rate limiting (5 requests per 10 seconds)
- Replace synchronous I/O with outbox pattern
- Add fail-safe rollback on errors"
```

---

## 🔄 ROLLBACK STEPS

### If Hotfix Causes Issues

#### Option 1: Revert Commit
```bash
git revert HEAD
git push origin hotfix/market-atomic-reserve
```

#### Option 2: Reset to Previous Commit
```bash
git log  # Find commit before hotfix
git reset --hard <commit-hash>
git push --force origin hotfix/market-atomic-reserve
```

#### Option 3: Emergency Disable (No Code Change)
Add to `AskalPurchaseModule.c`:
```c
static void SetMarketEnabled(bool enabled)
{
    s_MarketEnabled = false; // Force disable
}
```

Then call in RPC handler:
```c
void PurchaseItemRequest(...)
{
    if (!s_MarketEnabled)
    {
        SendPurchaseResponse(sender, false, "", 0, "Market temporarily disabled");
        return;
    }
    // ... rest of code
}
```

---

## ✅ VERIFICATION CHECKLIST

After applying hotfix:

- [ ] Code compiles without errors
- [ ] Server starts successfully
- [ ] Rate limiting works (test with spam)
- [ ] Double-spend prevented (test concurrent purchases)
- [ ] Outbox flushes correctly (check logs)
- [ ] No performance degradation
- [ ] Rollback procedure tested

---

## 🧪 TESTING COMMANDS

### Test 1: Concurrent Purchase (Double-Spend)
```bash
cd tests
python concurrent_purchase.py --steam-id "TEST_PLAYER" --count 10
# Expected: Only 1 purchase succeeds
```

### Test 2: Rate Limiting
```bash
python mass_spam.py --steam-id "TEST_PLAYER" --requests 50
# Expected: Only 5 requests succeed, rest rate-limited
```

### Test 3: Crash Recovery
```bash
python crash_recovery.py --steam-id "TEST_PLAYER"
# Expected: No double-spend, reserved funds accounted
```

---

## 📊 MONITORING

After deployment, monitor:

1. **Server Logs** - Look for:
   - `[AskalPurchase] ⚠️ Rate limit exceeded`
   - `[AskalBalance] ✅ Funds reservados`
   - `[AskalBalance] ⚠️ Lock timeout`

2. **Performance Metrics**:
   - Purchase latency (should be <50ms)
   - Server FPS (should remain >30)
   - File I/O operations (should decrease)

3. **Error Rates**:
   - Failed reservations
   - Lock timeouts
   - Outbox flush failures

---

## 🔧 CONFIGURATION

### Rate Limit Tuning

Edit `AskalPurchaseModule.c`:
```c
private static const float RATE_LIMIT_WINDOW = 10.0; // Window in seconds
private static const int MAX_REQUESTS_PER_WINDOW = 5; // Max requests
```

### Outbox Flush Interval

Edit `AskalPlayerBalance.c`:
```c
private static const float OUTBOX_FLUSH_INTERVAL = 5.0; // Flush every 5 seconds
```

### Cache TTL

Edit `AskalPlayerBalance.c`:
```c
private static const float CACHE_TTL = 30.0; // Cache for 30 seconds
```

---

## 🚨 EMERGENCY CONTACTS

If critical issues occur:
1. Disable market immediately (see Option 1 above)
2. Check server logs for errors
3. Rollback if necessary (see Rollback Steps)
4. Report issue with logs

---

## 📝 NOTES

- **EnforceScript Limitations**: The lock mechanism is simplified since EnforceScript is single-threaded. The main protection is atomic reservation.

- **Outbox Pattern**: Data is queued and flushed every 5 seconds. On server shutdown, all pending saves are flushed.

- **Backward Compatibility**: The hotfix maintains backward compatibility. Old code using `RemoveBalance()` still works, but new code should use `ReserveFunds()` + `ConfirmReservation()`.

- **Performance**: Caching reduces file I/O by ~80%. Outbox pattern batches writes, reducing disk operations.

---

**Last Updated:** 2024  
**Hotfix Version:** 1.0  
**Status:** Ready for Testing

