# Iteration 1 Hotfix - Apply Instructions

## Summary
**Branch:** `iter/1-hotfix-reserve-rate`  
**Changes:** Atomic reserve pattern, rate limiting (5 ops/10s), async persistence via outbox  
**Files Changed:** 4 files (AskalPlayerBalance.c, AskalPurchaseService.c, AskalPurchaseModule.c, MissionServer.c)

---

## Step 1: Create Branch
```bash
cd P:\askal
git checkout -b iter/1-hotfix-reserve-rate
```

## Step 2: Apply Patch
```bash
git apply iter_1_hotfix.diff
```

## Step 3: Verify Changes
```bash
git status
git diff HEAD
```

Expected files changed:
- `Core/Scripts/3_Game/AskalPlayerBalance.c`
- `Market/Scripts/4_World/AskalPurchaseService.c`
- `Market/Scripts/4_World/AskalPurchaseModule.c`
- `Core/Scripts/5_Mission/MissionServer.c`

## Step 4: Compile
Compile your DayZ mod using your normal build process. If compilation fails:

**Common Issues:**
- `GetGame().GetTime()` - Verify this returns float. If not, use alternative timing.
- Syntax errors - Check for missing semicolons or brackets.

## Step 5: Test in Community Offline Mode
1. Start DayZ with Community Offline Mode
2. Load Askal mod
3. Connect to server
4. Run test scripts (see below)

---

## Rollback (If Needed)

### Option 1: Revert Patch
```bash
git checkout -- Core/Scripts/3_Game/AskalPlayerBalance.c
git checkout -- Market/Scripts/4_World/AskalPurchaseService.c
git checkout -- Market/Scripts/4_World/AskalPurchaseModule.c
git checkout -- Core/Scripts/5_Mission/MissionServer.c
```

### Option 2: Reset Branch
```bash
git reset --hard HEAD~1
```

### Option 3: Emergency Disable Market (No Code Change)
Add to `AskalPurchaseModule.c` constructor:
```c
void AskalPurchaseModule()
{
    s_MarketEnabled = false; // EMERGENCY DISABLE
    // ... rest of code
}
```

---

## What Changed

### 1. Atomic Reserve Pattern
- `ReserveFunds()` - Reserves balance before item creation
- `ConfirmReservation()` - Confirms reservation after item created
- `ReleaseReservation()` - Rolls back if item creation fails

### 2. Rate Limiting
- Max 5 requests per 10 seconds per player
- Logs: `RATE_LIMIT steamId=...`

### 3. Async Persistence
- Outbox queue batches writes
- Flushes every 5 seconds or on shutdown
- Logs: `ENQUEUE_PERSIST steamId=...`

### 4. Logging Tags
- `RESERVE_OK` - Reservation succeeded
- `RESERVE_FAIL` - Reservation failed
- `RATE_LIMIT` - Rate limit hit
- `ENQUEUE_PERSIST` - Data queued for persistence
- `ORDER_PLACED` - Purchase completed

---

## Testing Checklist

After applying, run these tests and collect logs:

1. **Concurrent Purchase Test**
   ```bash
   python3 tests/iter_1_concurrent_purchase.py --steam-id "YOUR_STEAM_ID" --count 10
   ```

2. **Mass Spam Test**
   ```bash
   python3 tests/iter_1_mass_spam.py --steam-id "YOUR_STEAM_ID" --requests 50
   ```

3. **Crash Recovery Test**
   ```bash
   python3 tests/iter_1_crash_recovery.py --steam-id "YOUR_STEAM_ID"
   ```

4. **Manual Server Test**
   - Start server
   - Make a purchase
   - Check server logs for tags: `RESERVE_OK`, `ORDER_PLACED`, `ENQUEUE_PERSIST`
   - Wait 5+ seconds, check for outbox flush message

---

## Log Collection

Collect these log snippets:

1. **Server startup logs** (first 50 lines)
2. **Purchase attempt logs** (grep for `RESERVE_OK`, `RESERVE_FAIL`, `ORDER_PLACED`)
3. **Rate limit logs** (grep for `RATE_LIMIT`)
4. **Outbox flush logs** (grep for `ENQUEUE_PERSIST`, `Flushed`)
5. **Server shutdown logs** (last 50 lines, should show flush)

---

## Expected Behavior

### Before Hotfix
- Concurrent purchases: Both succeed (double-spend)
- Mass spam: All requests processed (DoS)
- Balance check/deduct: Separated (TOCTOU)

### After Hotfix
- Concurrent purchases: Only 1 succeeds
- Mass spam: Max 5 requests succeed, rest rate-limited
- Balance reserve: Before item creation (atomic)
- Persistence: Batched every 5 seconds

---

**Next:** Run tests and return logs. See `iter_1_report.md` for what to collect.

