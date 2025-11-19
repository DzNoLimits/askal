#!/usr/bin/env python3
"""
Crash Recovery Test - PoC for Data Persistence

Tests that reserved funds are properly handled on server crash/restart.
Simulates: reserve funds -> crash -> restart -> verify consistency.

Usage:
    python crash_recovery.py --steam-id "TEST_PLAYER"
"""

import argparse
import time
import requests
import json
import subprocess
import os
from typing import Dict, Optional

class CrashRecoveryTest:
    def __init__(self, server_url: str, steam_id: str, server_pid: Optional[int] = None):
        self.server_url = server_url
        self.steam_id = steam_id
        self.server_pid = server_pid
        self.initial_balance: Optional[int] = None
        self.final_balance: Optional[int] = None
        
    def get_balance(self) -> Optional[int]:
        """Get current balance for player"""
        try:
            response = requests.get(
                f"{self.server_url}/api/balance/{self.steam_id}",
                timeout=5.0
            )
            if response.status_code == 200:
                data = response.json()
                return data.get("balance", {}).get("Askal_Coin", 0)
        except Exception as e:
            print(f"[ERROR] Failed to get balance: {e}")
        return None
    
    def reserve_funds(self, amount: int) -> bool:
        """Reserve funds (simulate purchase start)"""
        payload = {
            "steamId": self.steam_id,
            "amount": amount,
            "currencyId": "Askal_Coin"
        }
        
        try:
            response = requests.post(
                f"{self.server_url}/rpc/ReserveFunds",
                json=payload,
                timeout=5.0
            )
            return response.status_code == 200 and response.json().get("success", False)
        except Exception as e:
            print(f"[ERROR] Failed to reserve funds: {e}")
            return False
    
    def kill_server(self):
        """Kill server process (simulate crash)"""
        if self.server_pid:
            try:
                if os.name == 'nt':  # Windows
                    subprocess.run(["taskkill", "/F", "/PID", str(self.server_pid)], check=False)
                else:  # Linux/Mac
                    subprocess.run(["kill", "-9", str(self.server_pid)], check=False)
                print(f"[TEST] Server process {self.server_pid} killed")
                time.sleep(2)  # Wait for process to die
            except Exception as e:
                print(f"[ERROR] Failed to kill server: {e}")
        else:
            print("[WARNING] Server PID not provided - manual kill required")
            print("  Please kill the server process manually and press Enter...")
            input()
    
    def wait_for_server_restart(self, max_wait: int = 30):
        """Wait for server to restart"""
        print(f"[TEST] Waiting for server to restart (max {max_wait}s)...")
        
        for i in range(max_wait):
            try:
                response = requests.get(f"{self.server_url}/health", timeout=2.0)
                if response.status_code == 200:
                    print(f"[TEST] Server is back online")
                    return True
            except:
                pass
            
            time.sleep(1)
            if i % 5 == 0:
                print(f"  ... still waiting ({i}s)")
        
        print(f"[WARNING] Server did not restart within {max_wait}s")
        return False
    
    def run_test(self) -> Dict:
        """Run crash recovery test"""
        print(f"[TEST] Starting crash recovery test")
        print(f"  Steam ID: {self.steam_id}")
        print()
        
        # Step 1: Get initial balance
        print("[STEP 1] Getting initial balance...")
        self.initial_balance = self.get_balance()
        if self.initial_balance is None:
            print("[ERROR] Failed to get initial balance")
            return {"test": "crash_recovery", "passed": False, "error": "Failed to get initial balance"}
        
        print(f"  Initial balance: {self.initial_balance}")
        
        # Step 2: Reserve funds
        reserve_amount = 500
        print(f"[STEP 2] Reserving {reserve_amount} funds...")
        if not self.reserve_funds(reserve_amount):
            print("[ERROR] Failed to reserve funds")
            return {"test": "crash_recovery", "passed": False, "error": "Failed to reserve funds"}
        
        print(f"  Funds reserved: {reserve_amount}")
        time.sleep(1)  # Small delay to ensure reservation is processed
        
        # Step 3: Verify balance decreased (reserved)
        balance_after_reserve = self.get_balance()
        print(f"[STEP 3] Balance after reserve: {balance_after_reserve}")
        
        # Step 4: Kill server (simulate crash)
        print(f"[STEP 4] Simulating server crash...")
        self.kill_server()
        
        # Step 5: Wait for restart
        print(f"[STEP 5] Waiting for server restart...")
        if not self.wait_for_server_restart():
            return {"test": "crash_recovery", "passed": False, "error": "Server did not restart"}
        
        # Step 6: Check final balance
        print(f"[STEP 6] Checking final balance...")
        time.sleep(2)  # Wait for server to fully initialize
        self.final_balance = self.get_balance()
        
        if self.final_balance is None:
            print("[ERROR] Failed to get final balance")
            return {"test": "crash_recovery", "passed": False, "error": "Failed to get final balance"}
        
        print(f"  Final balance: {self.final_balance}")
        print()
        
        # Analyze results
        # Expected: If reservation was confirmed before crash, balance should be reduced
        # If reservation was not confirmed, balance should be unchanged
        # Ideal: Reservation should be rolled back on crash (balance unchanged)
        
        balance_change = self.initial_balance - self.final_balance
        
        print(f"[RESULTS]")
        print(f"  Initial balance: {self.initial_balance}")
        print(f"  Final balance: {self.final_balance}")
        print(f"  Balance change: {balance_change}")
        print()
        
        # Test passes if:
        # 1. No double-spend (balance didn't decrease twice)
        # 2. Reservation was handled correctly (either confirmed or rolled back)
        
        # For this test, we expect reservation to be rolled back on crash
        # (since we didn't confirm it)
        expected_final = self.initial_balance  # Reservation should be rolled back
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
            "test": "crash_recovery",
            "passed": test_passed,
            "initial_balance": self.initial_balance,
            "final_balance": self.final_balance,
            "balance_change": balance_change,
            "expected_final": expected_final
        }

def main():
    parser = argparse.ArgumentParser(description="Test crash recovery")
    parser.add_argument("--steam-id", required=True, help="Steam ID to test")
    parser.add_argument("--server-url", default="http://localhost:2302", help="Server URL")
    parser.add_argument("--server-pid", type=int, help="Server process ID (for auto-kill)")
    
    args = parser.parse_args()
    
    test = CrashRecoveryTest(
        server_url=args.server_url,
        steam_id=args.steam_id,
        server_pid=args.server_pid
    )
    
    results = test.run_test()
    
    # Save results to file
    with open("crash_recovery_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"\nResults saved to: crash_recovery_results.json")
    
    return 0 if results.get("passed", False) else 1

if __name__ == "__main__":
    exit(main())

