#!/usr/bin/env python3
"""
Concurrent Purchase Test - PoC for Double-Spend Vulnerability

Tests that only one purchase succeeds when multiple concurrent requests
are sent for the same player with insufficient balance for all.

Usage:
    python concurrent_purchase.py --steam-id "TEST_PLAYER" --count 10 --price 500
"""

import argparse
import threading
import time
import requests
import json
from typing import List, Dict
from dataclasses import dataclass

@dataclass
class TestResult:
    thread_id: int
    success: bool
    response_time: float
    error: str = ""

class ConcurrentPurchaseTest:
    def __init__(self, server_url: str, steam_id: str, item_class: str, price: int, count: int):
        self.server_url = server_url
        self.steam_id = steam_id
        self.item_class = item_class
        self.price = price
        self.count = count
        self.results: List[TestResult] = []
        self.lock = threading.Lock()
        
    def send_purchase_request(self, thread_id: int) -> TestResult:
        """Send a single purchase request"""
        start_time = time.time()
        
        # Simulate RPC call (adjust endpoint based on your server setup)
        # This is a mock - replace with actual RPC call mechanism
        payload = {
            "steamId": self.steam_id,
            "itemClass": self.item_class,
            "price": self.price,
            "currencyId": "Askal_Coin",
            "quantity": 1.0,
            "quantityType": 0,
            "contentType": 0
        }
        
        try:
            # For DayZ, you'd use the actual RPC mechanism
            # This is a placeholder for testing framework
            response = requests.post(
                f"{self.server_url}/rpc/PurchaseItemRequest",
                json=payload,
                timeout=5.0
            )
            
            elapsed = time.time() - start_time
            success = response.status_code == 200 and response.json().get("success", False)
            error = "" if success else response.text
            
            return TestResult(thread_id, success, elapsed, error)
            
        except Exception as e:
            elapsed = time.time() - start_time
            return TestResult(thread_id, False, elapsed, str(e))
    
    def run_test(self) -> Dict:
        """Run concurrent purchase test"""
        print(f"[TEST] Starting concurrent purchase test")
        print(f"  Steam ID: {self.steam_id}")
        print(f"  Item: {self.item_class}")
        print(f"  Price: {self.price}")
        print(f"  Concurrent requests: {self.count}")
        print()
        
        threads = []
        
        # Launch all threads simultaneously
        for i in range(self.count):
            thread = threading.Thread(
                target=lambda tid=i: self.results.append(self.send_purchase_request(tid))
            )
            threads.append(thread)
        
        # Start all threads at once
        start_time = time.time()
        for thread in threads:
            thread.start()
        
        # Wait for all threads to complete
        for thread in threads:
            thread.join()
        
        total_time = time.time() - start_time
        
        # Analyze results
        successful = sum(1 for r in self.results if r.success)
        failed = len(self.results) - successful
        avg_response_time = sum(r.response_time for r in self.results) / len(self.results) if self.results else 0
        
        print(f"[RESULTS]")
        print(f"  Total requests: {len(self.results)}")
        print(f"  Successful: {successful}")
        print(f"  Failed: {failed}")
        print(f"  Total time: {total_time:.2f}s")
        print(f"  Avg response time: {avg_response_time*1000:.2f}ms")
        print()
        
        # Expected: Only 1 should succeed (if balance = price)
        # If balance > price * count, all should succeed
        # If balance < price, none should succeed
        
        expected_max_success = 1  # Assuming balance = price
        test_passed = successful <= expected_max_success
        
        if test_passed:
            print(f"✅ TEST PASSED: {successful} purchases succeeded (expected <= {expected_max_success})")
        else:
            print(f"❌ TEST FAILED: {successful} purchases succeeded (expected <= {expected_max_success})")
            print(f"   This indicates a double-spend vulnerability!")
        
        return {
            "test": "concurrent_purchase",
            "passed": test_passed,
            "successful": successful,
            "failed": failed,
            "total_time": total_time,
            "avg_response_time": avg_response_time,
            "results": [
                {
                    "thread_id": r.thread_id,
                    "success": r.success,
                    "response_time": r.response_time,
                    "error": r.error
                }
                for r in self.results
            ]
        }

def main():
    parser = argparse.ArgumentParser(description="Test concurrent purchase requests")
    parser.add_argument("--steam-id", required=True, help="Steam ID to test")
    parser.add_argument("--count", type=int, default=10, help="Number of concurrent requests")
    parser.add_argument("--price", type=int, default=500, help="Price per item")
    parser.add_argument("--item-class", default="ItemClass", help="Item class name")
    parser.add_argument("--server-url", default="http://localhost:2302", help="Server URL")
    
    args = parser.parse_args()
    
    test = ConcurrentPurchaseTest(
        server_url=args.server_url,
        steam_id=args.steam_id,
        item_class=args.item_class,
        price=args.price,
        count=args.count
    )
    
    results = test.run_test()
    
    # Save results to file
    with open("concurrent_purchase_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"\nResults saved to: concurrent_purchase_results.json")
    
    return 0 if results["passed"] else 1

if __name__ == "__main__":
    exit(main())

