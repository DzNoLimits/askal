# Iteration 1 - JSON Config Migration Notes

## Summary
Migrated test runner from text-based config to JSON config, removed offline/dev mode restriction, and enhanced file writing with platform-specific paths.

## Changes Made

### 1. New JSON Config File (`config/askal_dev.json`)
- Created structured JSON config with all test parameters
- Includes log tags configuration
- Platform-specific filesystem paths (Windows vs Unix)
- Default `enable: false` for safety

### 2. Config Class Restructure (`AskalDevTestConfig`)
- Replaced text-based config parsing with JSON loading via `AskalJsonLoader`
- Added `AskalDevTestLogTags` nested class for log tag configuration
- Updated field names to match JSON structure:
  - `market_dev_autotest` → `enable`
  - `dev_autotest_concurrent_count` → `concurrent_count`
  - `dev_autotest_spam_requests` → `spam_requests`
  - `dev_autotest_spam_window` → `spam_window_seconds`
  - `dev_autotest_write_files_to_profile` → `write_to_profile`
  - `dev_autotest_write_files_to_fs` → `write_to_fs`
  - Added `fs_output_path_windows` and `fs_output_path_unix`
  - Added `test_player_id_prefix` for customizable test player IDs

### 3. Config Loading (`LoadConfig()`)
- Replaced text file parsing with JSON loading
- Searches for `Config/askal_dev.json` in server root
- Falls back to profile paths if not found
- Verbose logging of config load status and values
- Safe error handling with defaults

### 4. Test Execution (`ShouldRunTests()`)
- **Removed offline/dev mode restriction** - tests run if `enable: true` regardless of mode
- Only requires server (not client)
- Removed multiplayer check

### 5. File Writing (`WriteResultFile()`, `GetTestResultsDirFS()`)
- Platform detection: chooses Windows vs Unix path based on path format
- Windows: Uses `fs_output_path_windows` (default: `C:\AskalTestResults`)
- Unix: Uses `fs_output_path_unix` (default: `Config/Askal/test_results`)
- Automatic directory creation with proper separators (`/` vs `\`)
- Verbose logging of directory creation and file writes
- Uses configurable log tags from `log_tags.file_written`

### 6. Test Functions
- Updated to use new config field names
- Uses `test_player_id_prefix` for test player IDs
- Uses configurable log tags from `log_tags` object
- All test logic preserved (no changes to business logic)

### 7. Documentation (`askal_dev_readme.md`)
- Complete user guide
- JSON config structure and options
- Output location details (both profile and filesystem)
- Troubleshooting guide

## File Output Locations

**Profile Path:** `$profile:Askal/test_results/`
- Windows: `%LOCALAPPDATA%\DayZ\Askal\test_results\`
- Linux: `~/.local/share/DayZ/Askal/test_results/`

**Filesystem Path:** Platform-specific
- **Windows:** `C:\AskalTestResults\` (configurable)
- **Unix/Linux:** `Config/Askal/test_results/` (configurable)

## Business Logic Preserved

- No changes to hotfix logic (reserve/confirm pattern, rate limiting, outbox)
- Tests call existing public APIs only
- No modifications to production code paths
- Fully gated behind config flag (`enable: false` by default)

## Next Steps

1. Apply `iter_1_config_json.diff`
2. Copy `config/askal_dev.json` to server `Config/` directory
3. Set `"enable": true` to enable tests
4. Start server
5. Check both output locations for JSON results:
   - Profile: `$profile:Askal/test_results/`
   - Filesystem: `C:\AskalTestResults\` (Windows) or `Config/Askal/test_results/` (Unix)

