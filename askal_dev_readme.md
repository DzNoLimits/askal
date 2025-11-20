# Askal Dev Test Runner - User Guide

## Overview

The Askal Dev Test Runner automatically executes ITER-1 hotfix tests when enabled via JSON config. Tests run on server startup and write JSON result files.

## Enable/Disable Tests

### Enable Tests

1. Edit `Config/askal_dev.json` (or create it in server root `Config/` directory)
2. Set `"enable": true`
3. Start server
4. Tests run automatically after 3 second initialization delay

### Disable Tests

1. Set `"enable": false` in `Config/askal_dev.json`
2. Or delete/rename config file (defaults to disabled)

## Configuration File

**Location:** `Config/askal_dev.json` (relative to server root)

**Structure:**
```json
{
	"enable": false,
	"concurrent_count": 10,
	"spam_requests": 50,
	"spam_window_seconds": 10,
	"allow_crash_simulation": false,
	"write_to_profile": true,
	"write_to_fs": true,
	"fs_output_path_windows": "C:\\AskalTestResults",
	"fs_output_path_unix": "Config/Askal/test_results",
	"test_player_id_prefix": "TEST_PLAYER_",
	"log_tags": {
		"concurrent": "TEST_CONCURRENT",
		"spam": "TEST_SPAM",
		"crash": "TEST_CRASH",
		"file_written": "TEST_FILE_WRITTEN"
	}
}
```

**Config Options:**
- `enable` - Enable/disable tests (default: false)
- `concurrent_count` - Number of concurrent reserve attempts (default: 10)
- `spam_requests` - Number of spam requests (default: 50)
- `spam_window_seconds` - Rate limit window in seconds (default: 10)
- `allow_crash_simulation` - Allow crash simulation (not implemented, default: false)
- `write_to_profile` - Write to profile path (default: true)
- `write_to_fs` - Write to filesystem path (default: true)
- `fs_output_path_windows` - Windows filesystem path (default: "C:\\AskalTestResults")
- `fs_output_path_unix` - Unix filesystem path (default: "Config/Askal/test_results")
- `test_player_id_prefix` - Prefix for test player IDs (default: "TEST_PLAYER_")
- `log_tags` - Custom log tags for each test type

## Output Locations

Results are written to **both** locations if enabled:

### 1. Profile Path
**Location:** `$profile:Askal/test_results/`

- Windows: `%LOCALAPPDATA%\DayZ\Askal\test_results\`
- Linux: `~/.local/share/DayZ/Askal/test_results/`

### 2. Filesystem Path
**Location:** Platform-specific:
- **Windows:** `C:\AskalTestResults\` (or `fs_output_path_windows` from config)
- **Unix/Linux:** `Config/Askal/test_results/` (or `fs_output_path_unix` from config)

## Result Files

Three JSON files are created in both locations (if enabled):

- `iter_1_concurrent_purchase_result.json` - Double-spend prevention test
- `iter_1_mass_spam_result.json` - Rate limiting test
- `iter_1_crash_recovery_result.json` - Crash recovery test

Each file contains:
- Test name and timestamps
- Attempt counts and success/failure counts
- Test-specific metrics
- Pass/fail status

## Log Tags

Search server logs for these tags (configurable in `log_tags`):

- `TEST_CONCURRENT START/END` - Concurrent purchase test
- `TEST_SPAM START/END` - Mass spam test
- `TEST_CRASH START/END` - Crash recovery test
- `TEST_FILE_WRITTEN path=...` - File write confirmation

Existing hotfix tags are also logged:
- `RESERVE_OK` - Reservation succeeded
- `RESERVE_FAIL` - Reservation failed
- `RATE_LIMIT` - Rate limit hit
- `ENQUEUE_PERSIST` - Data queued for persistence
- `ORDER_PLACED` - Purchase completed

## Disable File Writes

To disable file writes (logs only):

```json
{
	"write_to_profile": false,
	"write_to_fs": false
}
```

## Safety

- Tests run on server (not client)
- Uses isolated test player IDs (configurable prefix)
- Cleans up all test data after completion
- Does not modify production player data
- Fully reversible (disable via config)

## Troubleshooting

**No results written:**
- Check `"enable": true` in `Config/askal_dev.json`
- Verify server is running
- Check server logs for `TEST_FILE_WRITTEN` messages
- Ensure output directories are writable
- Check config loading messages: `[AskalDevTest] Loading config from: ...`

**Config not found:**
- Create `Config/askal_dev.json` in server root directory
- Check server logs for config loading messages
- Verify JSON syntax is valid

**Tests not running:**
- Verify `"enable": true` in config
- Check server logs for `[AskalDevTest]` messages
- Ensure server is running (not client)

**Directory creation fails:**
- Check filesystem permissions
- Verify path is valid for your platform
- Windows: Use `C:\\AskalTestResults` format (double backslash)
- Unix: Use `Config/Askal/test_results` format (forward slash)
