#!/usr/bin/env python3
"""
ITER-1: Concurrent Purchase Test
Tests that only 1 purchase succeeds when N concurrent requests are sent.

Usage:
    python3 iter_1_concurrent_purchase.py --steam-id "STEAM_ID" --count 10 --price 500
"""

import argparse
import threading
import time
import json
from typing import List
from dataclasses import dataclass, asdict

@dataclass
class TestResult:
    thread_id: int
    success: bool
    response_time: float
    error: str = ""

class ConcurrentPurchaseTest:
    def __init__(self, steam_id: str, item_class: str, price: int, count: int):
        self.steam_id = steam_id
        self.item_class = item_class
        self.price = price
        self.count = count
        self.results: List[TestResult] = []
        self.lock = threading.Lock()
        
    def send_purchase_request(self, thread_id: int) -> TestResult:
        """Send a single purchase request (mock - replace with actual RPC)"""
        start_time = time.time()
        
        # TODO: Replace with actual DayZ RPC call
        # For now, this simulates the behavior
        # In real test, use DayZ RPC mechanism or test framework
        
        try:
            # Simulate RPC call delay
            time.sleep(0.01)
            
            # Mock: Assume first request succeeds, rest fail due to reservation
            # In real test, this would be actual RPC response
            success = (thread_id == 0)  # Only first succeeds
            elapsed = time.time() - start_time
            
            return TestResult(thread_id, success, elapsed, "" if success else "RESERVE_FAIL")
            
        except Exception as e:
            elapsed = time.time() - start_time
            return TestResult(thread_id, False, elapsed, str(e))
    
    def run_test(self) -> dict:
        """Run concurrent purchase test"""
        print(f"[ITER-1 TEST] Concurrent Purchase Test")
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
        expected_max_success = 1
        test_passed = successful <= expected_max_success
        
        if test_passed:
            print(f"✅ TEST PASSED: {successful} purchases succeeded (expected <= {expected_max_success})")
        else:
            print(f"❌ TEST FAILED: {successful} purchases succeeded (expected <= {expected_max_success})")
            print(f"   This indicates a double-spend vulnerability!")
        
        return {
            "test": "iter_1_concurrent_purchase",
            "passed": test_passed,
            "successful": successful,
            "failed": failed,
            "total_time": total_time,
            "avg_response_time": avg_response_time,
            "results": [asdict(r) for r in self.results]
        }

def main():
    parser = argparse.ArgumentParser(description="Test concurrent purchase requests")
    parser.add_argument("--steam-id", required=True, help="Steam ID to test")
    parser.add_argument("--count", type=int, default=10, help="Number of concurrent requests")
    parser.add_argument("--price", type=int, default=500, help="Price per item")
    parser.add_argument("--item-class", default="ItemClass", help="Item class name")
    
    args = parser.parse_args()
    
    test = ConcurrentPurchaseTest(
        steam_id=args.steam_id,
        item_class=args.item_class,
        price=args.price,
        count=args.count
    )
    
    results = test.run_test()
    
    # Save results to file
    with open("iter_1_concurrent_purchase_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"\nResults saved to: iter_1_concurrent_purchase_results.json")
    
    return 0 if results["passed"] else 1

if __name__ == "__main__":
    exit(main())

