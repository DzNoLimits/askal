#!/usr/bin/env python3
"""
ITER-1: Mass Spam Test
Tests rate limiting (max 5 requests per 10 seconds).

Usage:
    python3 iter_1_mass_spam.py --steam-id "STEAM_ID" --requests 50
"""

import argparse
import time
import json
from typing import List
from dataclasses import dataclass, asdict

@dataclass
class RequestResult:
    request_id: int
    timestamp: float
    success: bool
    rate_limited: bool
    response_time: float
    error: str = ""

class MassSpamTest:
    def __init__(self, steam_id: str, item_class: str, price: int, request_count: int):
        self.steam_id = steam_id
        self.item_class = item_class
        self.price = price
        self.request_count = request_count
        self.results: List[RequestResult] = []
        
    def send_request(self, request_id: int) -> RequestResult:
        """Send a single purchase request (mock - replace with actual RPC)"""
        timestamp = time.time()
        start_time = time.time()
        
        # TODO: Replace with actual DayZ RPC call
        # Mock: First 5 succeed, rest rate-limited
        success = (request_id < 5)
        rate_limited = (request_id >= 5)
        elapsed = time.time() - start_time
        
        return RequestResult(request_id, timestamp, success, rate_limited, elapsed, "RATE_LIMIT" if rate_limited else "")
    
    def run_test(self) -> dict:
        """Run mass spam test"""
        print(f"[ITER-1 TEST] Mass Spam Test")
        print(f"  Steam ID: {self.steam_id}")
        print(f"  Requests: {self.request_count}")
        print(f"  Expected rate limit: 5 requests per 10 seconds")
        print()
        
        start_time = time.time()
        
        # Send all requests as fast as possible
        for i in range(self.request_count):
            result = self.send_request(i)
            self.results.append(result)
            time.sleep(0.01)  # Small delay
        
        total_time = time.time() - start_time
        
        # Analyze results
        successful = sum(1 for r in self.results if r.success)
        rate_limited = sum(1 for r in self.results if r.rate_limited)
        failed_other = len(self.results) - successful - rate_limited
        
        # Count requests in first 10 seconds
        window_start = self.results[0].timestamp if self.results else 0
        window_end = window_start + 10.0
        
        requests_in_window = sum(
            1 for r in self.results
            if window_start <= r.timestamp <= window_end
        )
        
        successful_in_window = sum(
            1 for r in self.results
            if window_start <= r.timestamp <= window_end and r.success
        )
        
        avg_response_time = sum(r.response_time for r in self.results) / len(self.results) if self.results else 0
        
        print(f"[RESULTS]")
        print(f"  Total requests: {len(self.results)}")
        print(f"  Successful: {successful}")
        print(f"  Rate limited: {rate_limited}")
        print(f"  Failed (other): {failed_other}")
        print(f"  Requests in first 10s: {requests_in_window}")
        print(f"  Successful in first 10s: {successful_in_window}")
        print(f"  Total time: {total_time:.2f}s")
        print(f"  Avg response time: {avg_response_time*1000:.2f}ms")
        print()
        
        # Expected: Max 5 successful requests in 10-second window
        expected_max_in_window = 5
        test_passed = successful_in_window <= expected_max_in_window and rate_limited > 0
        
        if test_passed:
            print(f"✅ TEST PASSED:")
            print(f"   - {successful_in_window} successful in first 10s (expected <= {expected_max_in_window})")
            print(f"   - {rate_limited} requests rate-limited")
        else:
            print(f"❌ TEST FAILED:")
            print(f"   - {successful_in_window} successful in first 10s (expected <= {expected_max_in_window})")
            print(f"   - Rate limiting may not be working!")
        
        return {
            "test": "iter_1_mass_spam",
            "passed": test_passed,
            "total_requests": len(self.results),
            "successful": successful,
            "rate_limited": rate_limited,
            "failed_other": failed_other,
            "successful_in_window": successful_in_window,
            "total_time": total_time,
            "avg_response_time": avg_response_time,
            "results": [asdict(r) for r in self.results]
        }

def main():
    parser = argparse.ArgumentParser(description="Test rate limiting with mass spam")
    parser.add_argument("--steam-id", required=True, help="Steam ID to test")
    parser.add_argument("--requests", type=int, default=50, help="Number of requests to send")
    parser.add_argument("--price", type=int, default=100, help="Price per item")
    parser.add_argument("--item-class", default="ItemClass", help="Item class name")
    
    args = parser.parse_args()
    
    test = MassSpamTest(
        steam_id=args.steam_id,
        item_class=args.item_class,
        price=args.price,
        request_count=args.requests
    )
    
    results = test.run_test()
    
    # Save results to file
    with open("iter_1_mass_spam_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"\nResults saved to: iter_1_mass_spam_results.json")
    
    return 0 if results["passed"] else 1

if __name__ == "__main__":
    exit(main())

