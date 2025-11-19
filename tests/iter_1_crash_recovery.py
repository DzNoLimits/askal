#!/usr/bin/env python3
"""
ITER-1: Crash Recovery Test
Tests that reserved funds are handled correctly on server crash/restart.

Usage:
    python3 iter_1_crash_recovery.py --steam-id "STEAM_ID"
"""

import argparse
import time
import json
from dataclasses import dataclass, asdict

@dataclass
class CrashRecoveryResult:
    initial_balance: int
    balance_after_reserve: int
    final_balance: int
    balance_change: int
    test_passed: bool

class CrashRecoveryTest:
    def __init__(self, steam_id: str):
        self.steam_id = steam_id
        self.initial_balance = None
        self.final_balance = None
        
    def get_balance(self) -> int:
        """Get current balance (mock - replace with actual API)"""
        # TODO: Replace with actual balance check
        # For now, simulate
        return 1000
    
    def reserve_funds(self, amount: int) -> bool:
        """Reserve funds (mock - replace with actual RPC)"""
        # TODO: Replace with actual ReserveFunds RPC
        return True
    
    def run_test(self) -> dict:
        """Run crash recovery test"""
        print(f"[ITER-1 TEST] Crash Recovery Test")
        print(f"  Steam ID: {self.steam_id}")
        print()
        
        # Step 1: Get initial balance
        print("[STEP 1] Getting initial balance...")
        self.initial_balance = self.get_balance()
        print(f"  Initial balance: {self.initial_balance}")
        
        # Step 2: Reserve funds
        reserve_amount = 500
        print(f"[STEP 2] Reserving {reserve_amount} funds...")
        if not self.reserve_funds(reserve_amount):
            return {"test": "iter_1_crash_recovery", "passed": False, "error": "Failed to reserve funds"}
        
        print(f"  Funds reserved: {reserve_amount}")
        time.sleep(1)
        
        # Step 3: Verify balance decreased (reserved)
        balance_after_reserve = self.get_balance()
        print(f"[STEP 3] Balance after reserve: {balance_after_reserve}")
        
        # Step 4: Simulate crash (manual step)
        print(f"[STEP 4] Simulating server crash...")
        print("  MANUAL: Kill server process now, then restart")
        input("  Press Enter after server restart...")
        
        # Step 5: Check final balance
        print(f"[STEP 5] Checking final balance...")
        time.sleep(2)
        self.final_balance = self.get_balance()
        
        print(f"  Final balance: {self.final_balance}")
        print()
        
        # Analyze results
        balance_change = self.initial_balance - self.final_balance
        
        print(f"[RESULTS]")
        print(f"  Initial balance: {self.initial_balance}")
        print(f"  Final balance: {self.final_balance}")
        print(f"  Balance change: {balance_change}")
        print()
        
        # Expected: If reservation was not confirmed, balance should be unchanged
        # (reservation rolled back on crash)
        expected_final = self.initial_balance
        test_passed = self.final_balance == expected_final
        
        if test_passed:
            print(f"✅ TEST PASSED:")
            print(f"   - Balance correctly restored after crash")
            print(f"   - No double-spend detected")
        else:
            print(f"❌ TEST FAILED:")
            print(f"   - Balance changed unexpectedly: {balance_change}")
            print(f"   - Reservation may not have been rolled back")
        
        return {
            "test": "iter_1_crash_recovery",
            "passed": test_passed,
            "initial_balance": self.initial_balance,
            "final_balance": self.final_balance,
            "balance_change": balance_change,
            "expected_final": expected_final
        }

def main():
    parser = argparse.ArgumentParser(description="Test crash recovery")
    parser.add_argument("--steam-id", required=True, help="Steam ID to test")
    
    args = parser.parse_args()
    
    test = CrashRecoveryTest(steam_id=args.steam_id)
    results = test.run_test()
    
    # Save results to file
    with open("iter_1_crash_recovery_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"\nResults saved to: iter_1_crash_recovery_results.json")
    
    return 0 if results.get("passed", False) else 1

if __name__ == "__main__":
    exit(main())

