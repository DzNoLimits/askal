# Iteration 1 - Compilation Fix Notes

## Summary
Fixed compilation errors without modifying hotfix business logic.

## Errors Fixed

### 1. AskalPlayerBalance.c:249 - Incompatible parameter 'steamId'
**Issue:** `foreach (string steamId : s_OutboxQueue)` - EnforceScript doesn't support foreach iteration on maps.
**Fix:** Changed to index-based iteration using `GetKey(i)` and `GetElement(i)`:
```c
for (int i = 0; i < s_OutboxQueue.Count(); i++)
{
    string steamId = s_OutboxQueue.GetKey(i);
    AskalPlayerData data = s_OutboxQueue.GetElement(i);
    ...
}
```

### 2. AskalDatabaseSync.c:131 - Bad type 'array'
**Issue:** `array<ref AskalDatasetSyncData>` - Missing `ref` keyword.
**Fix:** Changed to `ref array<ref AskalDatasetSyncData>`.

### 3. AskalDatabaseSync.c:504 - Bad type 'AskalJsonLoader'
**Issue:** Template class `AskalJsonLoader<AskalCategoryBatchData>` not recognized.
**Fix:** Added forward declaration: `class AskalJsonLoader<Class T>;`

### 4. AskalMarketModule.c:51,137,163 - Bad type 'Param4', 'Param1', 'Param3'
**Issue:** Generic Param types not recognized.
**Fix:** Added forward declarations:
```c
class Param1<Class T1>;
class Param2<Class T1, Class T2>;
class Param3<Class T1, Class T2, Class T3>;
class Param4<Class T1, Class T2, Class T3, Class T4>;
```

### 5. AskalSetupResolver.c:266 - Bad type 'JsonFileLoader'
**Issue:** Template class `JsonFileLoader<AskalSetupConfig>` not recognized.
**Fix:** Added forward declaration: `class JsonFileLoader<Class T>;`

### 6. AskalTraderConfig.c:140 - Bad type 'AskalJsonLoader'
**Issue:** Template class `AskalJsonLoader<AskalTraderConfig>` not recognized.
**Fix:** Added forward declaration: `class AskalJsonLoader<Class T>;`

### 7. AskalMarketConfig.c:121 - Bad type 'JsonFileLoader'
**Issue:** Template class `JsonFileLoader<AskalMarketConfigFile>` not recognized.
**Fix:** Added forward declaration: `class JsonFileLoader<Class T>;`

### 8. AskalMarketLoader.c:95 - Bad type 'JsonFileLoader'
**Issue:** Template class `JsonFileLoader<AskalVirtualStoreConfig>` not recognized.
**Fix:** Added forward declaration: `class JsonFileLoader<Class T>;`

## Files Modified
1. `Core/Scripts/3_Game/AskalPlayerBalance.c` - Fixed map iteration
2. `Core/Scripts/3_Game/AskalDatabaseSync.c` - Fixed array type, added forward declaration
3. `Market/Scripts/3_Game/AskalMarketModule.c` - Added Param forward declarations
4. `Market/Scripts/3_Game/AskalSetupResolver.c` - Added JsonFileLoader forward declaration
5. `Market/Scripts/3_Game/AskalTraderConfig.c` - Added AskalJsonLoader forward declaration
6. `Market/Scripts/3_Game/AskalMarketConfig.c` - Added JsonFileLoader forward declaration
7. `Market/Scripts/3_Game/AskalMarketLoader.c` - Added JsonFileLoader forward declaration

## Business Logic Preserved
- All hotfix logic (reserve/confirm pattern, rate limiting, outbox) remains unchanged
- Only compilation fixes applied (type corrections, forward declarations)
- No functional changes to iteration 1 hotfix

## Next Steps
1. Apply `iter_1_compile_fix.diff`
2. Recompile mod
3. Report any remaining compilation errors

