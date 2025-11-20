# Iteration 1 - Automated Test Runner Notes

## Summary
Server-side test harness that automatically runs ITER-1 hotfix tests when enabled via config flag.

## Files Created/Modified

### New Files
1. **`Core/Scripts/5_Mission/AskalDevTestRunner.c`** - Test runner class
   - Loads config from `config/askal_dev.cfg`
   - Runs three tests: concurrent purchase, mass spam, crash recovery
   - Writes JSON results to `$profile:Askal/test_results/`

2. **`config/askal_dev.cfg`** - Dev test configuration
   - `market_dev_autotest` - Enable/disable tests (default: false)
   - `dev_autotest_concurrent_count` - Number of concurrent reserve attempts (default: 10)
   - `dev_autotest_spam_requests` - Number of spam requests (default: 50)
   - `dev_autotest_spam_window` - Rate limit window in seconds (default: 10)
   - `dev_autotest_allow_crash` - Allow crash simulation (default: false)

### Modified Files
1. **`Core/Scripts/5_Mission/MissionServer.c`** - Added test runner trigger
   - Calls `AskalDevTestRunner.RunAllTests()` after 3 second delay
   - Only runs if config flag is enabled

## Test Details

### Test 1: Concurrent Purchase
**Purpose:** Verify double-spend prevention (only 1 reserve succeeds)
- Creates test player with sufficient balance
- Makes N concurrent `ReserveFunds()` calls
- Expects: 1 success, N-1 failures
- Cleans up reservations and test balance
- **Log Tag:** `TEST_CONCURRENT`

### Test 2: Mass Spam
**Purpose:** Verify rate limiting behavior (indirectly via lock mechanism)
- Makes M rapid reserve requests
- Tracks accepted vs lock failures
- Note: Actual rate limiting (5 per 10s) is in `AskalPurchaseModule.CheckRateLimit()` which is private
- This test verifies the lock mechanism works (prevents concurrent access)
- Full rate limit testing requires RPC integration tests
- **Log Tag:** `TEST_SPAM`

### Test 3: Crash Recovery
**Purpose:** Verify reservation persistence across flush
- Reserves funds
- Flushes outbox (simulating crash)
- Verifies balance unchanged (reservation doesn't deduct until confirmed)
- Releases reservation and cleans up
- **Log Tag:** `TEST_CRASH`

## Result Files

Results are written to: `$profile:Askal/test_results/`

- `iter_1_concurrent_purchase_result.json`
- `iter_1_mass_spam_result.json`
- `iter_1_crash_recovery_result.json`

Each JSON file contains:
- Test name
- Start/end timestamps
- Duration
- Attempt counts
- Success/failure counts
- Pass/fail status
- Test-specific metrics

## Log Tags

The test runner uses these tags for easy grepping:
- `TEST_CONCURRENT` - Concurrent purchase test
- `TEST_SPAM` - Mass spam test
- `TEST_CRASH` - Crash recovery test

Existing hotfix tags are also logged:
- `RESERVE_OK` - Reservation succeeded
- `RESERVE_FAIL` - Reservation failed
- `RATE_LIMIT` - Rate limit hit (from actual RPC calls)
- `ENQUEUE_PERSIST` - Data queued for persistence
- `ORDER_PLACED` - Purchase completed

## Usage

### Enable Tests
1. Edit `config/askal_dev.cfg` (or create it in `$profile:config/askal_dev.cfg`)
2. Set `market_dev_autotest = true`
3. Start server in offline/dev mode
4. Tests run automatically after 3 second initialization delay

### Disable Tests
1. Set `market_dev_autotest = false` in config
2. Or delete/rename config file (defaults to disabled)

### View Results
1. Check server console for `TEST_*` log lines
2. Copy result JSON files from `$profile:Askal/test_results/`
3. Parse JSON to verify test outcomes

## Safety

- Tests only run in offline/dev server mode (not multiplayer)
- Uses isolated test player IDs (`TEST_PLAYER_*`)
- Cleans up all test data after completion
- Does not modify production player data
- Fully reversible (disable via config flag)

## Limitations

1. **Rate Limiting Test:** Cannot directly test `CheckRateLimit()` as it's private in `AskalPurchaseModule`. The test verifies lock mechanism instead. Full rate limit testing requires RPC integration tests.

2. **Crash Simulation:** Does not actually crash the server (would require `GetGame().RequestExit()` which is disabled by default). Instead simulates by flushing outbox and checking state.

3. **Concurrent Execution:** EnforceScript is single-threaded, so "concurrent" tests are sequential rapid calls. Still tests the lock mechanism effectively.

## Business Logic Preserved

- No changes to hotfix logic (reserve/confirm pattern, rate limiting, outbox)
- Tests call existing public APIs only
- No modifications to production code paths
- Fully gated behind dev flag

## Next Steps

1. Apply `iter_1_autotest_runner.diff`
2. Copy `config/askal_dev.cfg` to server config directory
3. Set `market_dev_autotest = true`
4. Start server in offline mode
5. Check `$profile:Askal/test_results/` for JSON results
6. Review server logs for `TEST_*` tags

