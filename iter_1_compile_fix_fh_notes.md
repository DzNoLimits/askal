# Iteration 1 - Compilation Fix #5 Notes

## Summary
Fixed duplicate variable declaration error by renaming filesystem file handle variable from `fh` to `fsFh`.

## Error Fixed

**File:** `Core/Scripts/5_Mission/AskalDevTestRunner.c:253`  
**Error:** `Multiple declaration of variable 'fh'`  
**Root Cause:** Two `FileHandle fh` declarations in the same function scope (lines 233 and 253).

## Change Made

Renamed the second file handle variable in `WriteResultFile()` function:
- **Line 253:** Changed `FileHandle fh = OpenFile(fsPath, FileMode.WRITE);` to `FileHandle fsFh = OpenFile(fsPath, FileMode.WRITE);`
- Updated all references to use `fsFh` instead of `fh` in the filesystem write block

## Rationale

In EnforceScript, variables declared in if blocks are scoped to the function, not just the block. Both `fh` declarations were in the same function scope, causing a conflict. Renaming the second one to `fsFh` resolves the conflict while preserving identical behavior.

