# ITERATION 1 - WHAT I (HUMAN) MUST RUN & RETURN

## 2-Line Summary
**Iteration 1 Hotfix:** Atomic reserve pattern + rate limiting (5 ops/10s) + async persistence to prevent double-spend and DoS.  
**Status:** Ready for testing - apply patch, compile, run tests, return logs.

---

## CHECKLIST: What to Run

### Step 1: Apply Patch
```bash
cd P:\askal
git checkout -b iter/1-hotfix-reserve-rate
git apply iter_1_hotfix.diff
git status  # Verify 4 files changed
```

### Step 2: Compile
```bash
# Use your normal DayZ mod compilation process
# Report any compilation errors
```

### Step 3: Start Server
```bash
# Start DayZ server with Community Offline Mode
# Load Askal mod
# Note: Server log file path for later collection
```

### Step 4: Run Test Scripts

**Test 1: Concurrent Purchase (Double-Spend)**
```bash
cd tests
python3 iter_1_concurrent_purchase.py --steam-id "YOUR_STEAM_ID" --count 10 --price 500
```
**Expected:** Only 1 purchase succeeds  
**Collect:** `iter_1_concurrent_purchase_results.json`

**Test 2: Mass Spam (Rate Limiting)**
```bash
python3 iter_1_mass_spam.py --steam-id "YOUR_STEAM_ID" --requests 50
```
**Expected:** Max 5 requests succeed, rest rate-limited  
**Collect:** `iter_1_mass_spam_results.json`

**Test 3: Crash Recovery**
```bash
python3 iter_1_crash_recovery.py --steam-id "YOUR_STEAM_ID"
# Follow prompts: reserve funds, kill server, restart, check balance
```
**Expected:** Balance consistent after restart  
**Collect:** `iter_1_crash_recovery_results.json`

### Step 5: Manual Test
1. Make a purchase in-game
2. Check server console for log tags
3. Wait 5+ seconds, check for outbox flush
4. Make 6+ rapid purchases, verify rate limiting

---

## CHECKLIST: What to Return

### 1. Log Snippets (Required)

**Server Startup (first 50 lines):**
```bash
head -n 50 <server_log_file>
```

**Purchase Attempts (grep for tags):**
```bash
grep -E "RESERVE_OK|RESERVE_FAIL|ORDER_PLACED" <server_log_file> | tail -n 50
```

**Rate Limiting (grep for RATE_LIMIT):**
```bash
grep "RATE_LIMIT" <server_log_file> | tail -n 20
```

**Outbox Persistence (grep for ENQUEUE_PERSIST):**
```bash
grep -E "ENQUEUE_PERSIST|Flushed" <server_log_file> | tail -n 20
```

**Server Shutdown (last 50 lines):**
```bash
tail -n 50 <server_log_file>
```

### 2. Test Result Files (Required)
- `iter_1_concurrent_purchase_results.json`
- `iter_1_mass_spam_results.json`
- `iter_1_crash_recovery_results.json`

### 3. Compilation/Runtime Issues (If Any)
- Any compilation errors (paste full error)
- Any runtime errors (paste full error)
- Any unexpected behavior

### 4. Manual Test Observations (Required)
- Number of successful purchases in concurrent test: ___
- Number of rate-limited requests in spam test: ___
- Balance after crash recovery: ___
- Server FPS during tests: ___
- Any other observations: ___

---

## Expected Log Tags

You should see these tags in server logs:

- `RESERVE_OK steamId=...` - Reservation succeeded
- `RESERVE_FAIL steamId=... reason=...` - Reservation failed
- `RATE_LIMIT steamId=...` - Rate limit hit
- `ENQUEUE_PERSIST steamId=...` - Data queued for persistence
- `ORDER_PLACED steamId=...` - Purchase completed
- `Flushed X pending saves to disk` - Outbox flush

---

## Success Criteria

✅ **Concurrent Purchase:** Only 1 succeeds (prevents double-spend)  
✅ **Mass Spam:** Max 5 succeed, rest rate-limited (prevents DoS)  
✅ **Crash Recovery:** Balance consistent (no data loss)  
✅ **Logs:** All tags present and correct

---

## If Tests Fail

If any test fails:
1. Paste the full error/log output
2. Note which test failed
3. Describe what happened vs. what was expected
4. I will create a fix in next iteration

---

## After Returning Results

Once you paste:
- All log snippets
- Test result JSON files
- Compilation/runtime issues (if any)
- Manual test observations

I will:
1. Analyze results
2. Confirm pass/fail per test
3. Propose iteration 2 (if tests pass) or fix (if tests fail)
4. Wait for your "go next" before proceeding

---

**READY TO TEST**  
**Return all items above before proceeding to iteration 2**

