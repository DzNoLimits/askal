# Iteration 1 - Compilation Fix #6 Notes

## Summary
Fixed duplicate variable declaration error by declaring `logTag` once at function scope instead of twice in separate if blocks.

## Error Fixed

**File:** `Core/Scripts/5_Mission/AskalDevTestRunner.c:288`  
**Error:** `Multiple declaration of variable 'logTag'`  
**Root Cause:** Two `string logTag` declarations in the same function scope (lines 262 and 288) within `WriteResultFile()`.

## Change Made

Moved `logTag` declaration to function scope (before the if blocks) and removed duplicate declarations:
- **Before:** `logTag` declared inside both `if (s_Config.write_to_profile)` and `if (s_Config.write_to_fs)` blocks
- **After:** `logTag` declared once at function level (line ~250), reused in both blocks

## Rationale

In EnforceScript, variables declared in if blocks are scoped to the function, not just the block. Both declarations were in the same function scope, causing a conflict. Declaring once at function level resolves the conflict while preserving identical behavior.

