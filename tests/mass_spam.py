#!/usr/bin/env python3
"""
Mass Spam Test - PoC for Rate Limiting

Tests that rate limiting prevents DoS attacks by limiting requests
to MAX_REQUESTS_PER_WINDOW (5) per RATE_LIMIT_WINDOW (10 seconds).

Usage:
    python mass_spam.py --steam-id "TEST_PLAYER" --requests 50
"""

import argparse
import time
import requests
import json
from typing import List, Dict
from dataclasses import dataclass

@dataclass
class RequestResult:
    request_id: int
    timestamp: float
    success: bool
    rate_limited: bool
    response_time: float
    error: str = ""

class MassSpamTest:
    def __init__(self, server_url: str, steam_id: str, item_class: str, price: int, request_count: int):
        self.server_url = server_url
        self.steam_id = steam_id
        self.item_class = item_class
        self.price = price
        self.request_count = request_count
        self.results: List[RequestResult] = []
        
    def send_request(self, request_id: int) -> RequestResult:
        """Send a single purchase request"""
        timestamp = time.time()
        start_time = time.time()
        
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
            response = requests.post(
                f"{self.server_url}/rpc/PurchaseItemRequest",
                json=payload,
                timeout=2.0
            )
            
            elapsed = time.time() - start_time
            success = response.status_code == 200 and response.json().get("success", False)
            
            # Check if rate limited (error message contains "rate limit")
            response_text = response.text.lower()
            rate_limited = "rate limit" in response_text or response.status_code == 429
            
            error = "" if success or rate_limited else response.text
            
            return RequestResult(request_id, timestamp, success, rate_limited, elapsed, error)
            
        except Exception as e:
            elapsed = time.time() - start_time
            return RequestResult(request_id, timestamp, False, False, elapsed, str(e))
    
    def run_test(self) -> Dict:
        """Run mass spam test"""
        print(f"[TEST] Starting mass spam test")
        print(f"  Steam ID: {self.steam_id}")
        print(f"  Requests: {self.request_count}")
        print(f"  Expected rate limit: 5 requests per 10 seconds")
        print()
        
        start_time = time.time()
        
        # Send all requests as fast as possible
        for i in range(self.request_count):
            result = self.send_request(i)
            self.results.append(result)
            
            # Small delay to avoid overwhelming (but still fast)
            time.sleep(0.01)
        
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
            "test": "mass_spam",
            "passed": test_passed,
            "total_requests": len(self.results),
            "successful": successful,
            "rate_limited": rate_limited,
            "failed_other": failed_other,
            "successful_in_window": successful_in_window,
            "total_time": total_time,
            "avg_response_time": avg_response_time,
            "results": [
                {
                    "request_id": r.request_id,
                    "timestamp": r.timestamp,
                    "success": r.success,
                    "rate_limited": r.rate_limited,
                    "response_time": r.response_time,
                    "error": r.error
                }
                for r in self.results
            ]
        }

def main():
    parser = argparse.ArgumentParser(description="Test rate limiting with mass spam")
    parser.add_argument("--steam-id", required=True, help="Steam ID to test")
    parser.add_argument("--requests", type=int, default=50, help="Number of requests to send")
    parser.add_argument("--price", type=int, default=100, help="Price per item")
    parser.add_argument("--item-class", default="ItemClass", help="Item class name")
    parser.add_argument("--server-url", default="http://localhost:2302", help="Server URL")
    
    args = parser.parse_args()
    
    test = MassSpamTest(
        server_url=args.server_url,
        steam_id=args.steam_id,
        item_class=args.item_class,
        price=args.price,
        request_count=args.requests
    )
    
    results = test.run_test()
    
    # Save results to file
    with open("mass_spam_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"\nResults saved to: mass_spam_results.json")
    
    return 0 if results["passed"] else 1

if __name__ == "__main__":
    exit(main())

