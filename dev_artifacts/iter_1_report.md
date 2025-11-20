# Iteration 1 Hotfix - Report

## Summary
**Iteration:** 1  
**Branch:** `iter/1-hotfix-reserve-rate`  
**Goal:** Prevent double-spend exploit and DoS via atomic reservations and rate limiting

**Changes:**
- Atomic reserve/confirm pattern for balance operations
- Per-player rate limiting (5 requests per 10 seconds)
- Async persistence via outbox pattern (batched every 5 seconds)
- Per-player locks with timeout protection
- Clear logging tags for monitoring

---

## Files Changed

1. **Core/Scripts/3_Game/AskalPlayerBalance.c**
   - Added: `ReserveFunds()`, `ConfirmReservation()`, `ReleaseReservation()`
   - Added: Per-player locks, cache, outbox queue
   - Added: `FlushOutbox()`, `FlushAllPendingSaves()`

2. **Market/Scripts/4_World/AskalPurchaseService.c**
   - Modified: Use `ReserveFunds()` before item creation
   - Modified: Use `ConfirmReservation()` after item creation
   - Added: Rollback on failure

3. **Market/Scripts/4_World/AskalPurchaseModule.c**
   - Added: `CheckRateLimit()` function
   - Added: Rate limiting check in `ProcessPurchaseRequest()`

4. **Core/Scripts/5_Mission/MissionServer.c**
   - Added: `OnMissionFinish()` to flush pending saves

---

## Logging Tags Added

- `RESERVE_OK` - Reservation succeeded
- `RESERVE_FAIL` - Reservation failed (with reason)
- `RATE_LIMIT` - Rate limit exceeded
- `ENQUEUE_PERSIST` - Data queued for async persistence
- `ORDER_PLACED` - Purchase completed successfully

---

## Expected Behavior

### Before Hotfix
- ❌ Concurrent purchases: Both succeed (double-spend)
- ❌ Mass spam: All requests processed (DoS)
- ❌ Balance check/deduct: Separated (TOCTOU vulnerability)

### After Hotfix
- ✅ Concurrent purchases: Only 1 succeeds (atomic reservation)
- ✅ Mass spam: Max 5 requests succeed, rest rate-limited
- ✅ Balance reserve: Before item creation (prevents TOCTOU)
- ✅ Persistence: Batched every 5 seconds (reduces I/O)

---

## Test Scripts

1. **iter_1_concurrent_purchase.py** - Tests double-spend prevention
2. **iter_1_mass_spam.py** - Tests rate limiting
3. **iter_1_crash_recovery.py** - Tests crash recovery

---

## What to Collect from Testing

### 1. Server Logs
Collect these log snippets:

**Startup:**
```bash
# First 50 lines of server log
head -n 50 server.log
```

**Purchase Attempts:**
```bash
# Grep for reservation logs
grep -E "RESERVE_OK|RESERVE_FAIL|ORDER_PLACED" server.log | tail -n 50
```

**Rate Limiting:**
```bash
# Grep for rate limit logs
grep "RATE_LIMIT" server.log | tail -n 20
```

**Outbox Persistence:**
```bash
# Grep for persistence logs
grep -E "ENQUEUE_PERSIST|Flushed" server.log | tail -n 20
```

**Shutdown:**
```bash
# Last 50 lines of server log
tail -n 50 server.log
```

### 2. Test Results
- `iter_1_concurrent_purchase_results.json`
- `iter_1_mass_spam_results.json`
- `iter_1_crash_recovery_results.json`

### 3. Manual Test Observations
- Number of successful purchases in concurrent test
- Number of rate-limited requests in spam test
- Balance consistency after crash recovery
- Server performance (FPS, latency)

---

## Success Criteria

✅ **Test 1 (Concurrent Purchase):** Only 1 purchase succeeds  
✅ **Test 2 (Mass Spam):** Max 5 requests succeed, rest rate-limited  
✅ **Test 3 (Crash Recovery):** Balance consistent after restart  
✅ **Logs:** All tags present (`RESERVE_OK`, `RATE_LIMIT`, `ENQUEUE_PERSIST`, `ORDER_PLACED`)

---

## Next Steps

After testing, return:
1. All log snippets (startup, purchase, rate limit, persistence, shutdown)
2. Test result JSON files
3. Any compilation errors or runtime issues
4. Manual test observations

Then proceed to iteration 2 (if tests pass) or fix issues (if tests fail).

---

**Status:** Ready for Testing  
**Wait for:** Test results and logs from developer

