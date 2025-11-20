# Iteration 1 - Test Runner Write Fix Notes

## Summary
Fixed test runner to reliably write JSON result files to both profile and filesystem paths, with improved config handling and logging.

## Changes Made

### 1. Config File (`config/askal_dev.cfg`)
- Added new config options:
  - `dev_autotest_write_files_to_profile` (default: true)
  - `dev_autotest_write_files_to_fs` (default: true)
  - `dev_autotest_fs_output_path` (default: "Config/Askal/test_results")
- Changed default `market_dev_autotest` to `false` for safety
- Added comments explaining each option

### 2. Config Class (`AskalDevTestConfig`)
- Added three new fields for file writing control
- Updated defaults to match config file

### 3. Config Loading (`LoadConfig()`)
- Added parsing for new config options
- Handles string values for filesystem path

### 4. File Writing (`WriteResultFile()`)
- **Dual-path writing:** Writes to both profile and filesystem paths if enabled
- **Profile path:** `$profile:Askal/test_results/` (existing behavior)
- **Filesystem path:** `Config/Askal/test_results/` (relative to server root)
- **Directory creation:** Automatically creates directory structure for FS path
- **Logging:** Logs `TEST_FILE_WRITTEN path=<full_path>` for each successful write
- **Error handling:** Continues if one path fails, reports both successes/failures

### 5. Directory Creation (`GetTestResultsDirFS()`)
- New function to create filesystem output directory
- Splits path by `/` or `\` and creates each level
- Handles both forward and backslash separators

### 6. Test Logging
- Added `TEST_CONCURRENT START/END` tags
- Added `TEST_SPAM START/END` tags
- Added `TEST_CRASH START/END` tags
- Enhanced completion messages with both output paths

### 7. Documentation (`askal_dev_readme.md`)
- Complete user guide
- Configuration instructions
- Output location details
- Troubleshooting guide

## File Output Locations

**Profile Path:** `$profile:Askal/test_results/`
- Windows: `%LOCALAPPDATA%\DayZ\Askal\test_results\`
- Linux: `~/.local/share/DayZ/Askal/test_results/`

**Filesystem Path:** `Config/Askal/test_results/` (relative to server root)
- Example: `C:\DayZServer\Config\Askal\test_results\`

## Result Files

Three JSON files are created in both locations (if enabled):
- `iter_1_concurrent_purchase_result.json`
- `iter_1_mass_spam_result.json`
- `iter_1_crash_recovery_result.json`

## Business Logic Preserved

- No changes to hotfix logic (reserve/confirm pattern, rate limiting, outbox)
- Tests call existing public APIs only
- No modifications to production code paths
- Fully gated behind dev flag

## Next Steps

1. Apply `iter_1_autotest_writefix.diff`
2. Copy `config/askal_dev.cfg` to server config directory
3. Set `market_dev_autotest = true` to enable
4. Start server in offline mode
5. Check both output locations for JSON results:
   - Profile: `$profile:Askal/test_results/`
   - Filesystem: `Config/Askal/test_results/` (relative to server root)

