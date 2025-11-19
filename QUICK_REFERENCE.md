# Askal Audit - Quick Reference Card

## 🚨 CRITICAL FIXES (Apply First)

### 1. Race Condition Fix
**File:** `Core/Scripts/3_Game/AskalPlayerBalance.c`
**Add:** Per-player locks around balance operations
**Time:** 4 hours

### 2. Rate Limiting
**File:** `Market/Scripts/4_World/AskalPurchaseModule.c`
**Add:** CheckRateLimit() before processing requests
**Time:** 2 hours

### 3. Caching
**File:** `Core/Scripts/3_Game/AskalPlayerBalance.c`
**Add:** In-memory cache with 30s TTL
**Time:** 4 hours

### 4. Reservation Pattern
**File:** `Core/Scripts/3_Game/AskalPlayerBalance.c`
**Add:** ReserveBalance(), ConfirmReservation(), ReleaseReservation()
**Time:** 4 hours

---

## 📁 FILES TO MODIFY

1. `Core/Scripts/3_Game/AskalPlayerBalance.c` - Locks, cache, reservations
2. `Market/Scripts/4_World/AskalPurchaseService.c` - Use reservations
3. `Market/Scripts/4_World/AskalPurchaseModule.c` - Rate limiting, batch atomicity
4. `Core/Scripts/3_Game/AskalJsonLoader.c` - Add FlushFile()

---

## 🧪 QUICK TEST COMMANDS

### Test Race Condition
```c
// Send 2 concurrent purchases for same player
GetRPCManager().SendRPC("AskalMarketModule", "PurchaseItemRequest", ...);
// Repeat immediately
```

### Test Rate Limiting
```c
// Send 20 requests in < 1 second
for (int i = 0; i < 20; i++) {
    GetRPCManager().SendRPC("AskalMarketModule", "PurchaseItemRequest", ...);
}
```

### Test Batch Atomicity
```c
// Send batch with insufficient balance
// Expected: All fail
// Actual: Partial success (BUG)
```

---

## 🔍 KEY CODE PATTERNS

### Lock Pattern
```c
Lock lock = GetOrCreateLock(steamId);
lock.Lock();
// ... atomic operation ...
lock.Unlock();
```

### Reservation Pattern
```c
if (!ReserveBalance(steamId, price)) return false;
EntityAI item = CreateItem(...);
if (!item) {
    ReleaseReservation(steamId, price);
    return false;
}
ConfirmReservation(steamId, price);
```

### Cache Pattern
```c
AskalPlayerData data = GetCachedOrLoad(steamId);
// Use cached data
// Update cache on write
```

---

## ⚡ PERFORMANCE TARGETS

- **File I/O Reduction:** >80% (via caching)
- **Server FPS:** >30 FPS under load
- **Request Latency:** <50ms per purchase
- **Cache Hit Rate:** >90%

---

## 🛡️ SECURITY TARGETS

- **Zero double-spends** (race condition fixed)
- **Rate limit:** Max 5 requests/second per player
- **Atomic transactions** (all-or-nothing)
- **No data loss** (flush on shutdown)

---

## 📊 MONITORING METRICS

Track these in logs:
- `[AskalBalance] Cache hit/miss`
- `[AskalPurchase] Rate limit exceeded`
- `[AskalBalance] Lock timeout`
- `[AskalPurchase] Batch rollback`

---

## 🚀 DEPLOYMENT CHECKLIST

- [ ] Apply critical fixes
- [ ] Run test scenarios 1-5
- [ ] Load test with 10+ players
- [ ] Verify no double-spend
- [ ] Check performance metrics
- [ ] Deploy to production
- [ ] Monitor for 24 hours

---

## 📞 EMERGENCY ROLLBACK

If issues occur:
1. Disable new code paths (feature flags)
2. Revert to synchronous I/O temporarily
3. Keep locks active (prevents exploits)
4. Investigate and fix
5. Re-deploy

---

**See full details in:**
- `askal_audit_critical_findings.md` - Complete audit
- `askal_implementation_guide.md` - Code examples
- `askal_testplan.yml` - Test procedures

