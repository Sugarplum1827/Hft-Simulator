#!/usr/bin/env python3
"""
HFT Performance Benchmark Tool
Measures and displays real-time trading simulation performance
"""

import time
import threading
from collections import deque
from datetime import datetime
import numpy as np

from models.engine import TradingEngine
from models.trader import Trader

class PerformanceBenchmark:
    """Real-time performance monitoring for HFT simulation"""
    
    def __init__(self):
        self.engine = TradingEngine()
        self.traders = []
        self.running = False
        
        # Performance metrics
        self.start_time = None
        self.trade_timestamps = deque(maxlen=10000)
        self.order_timestamps = deque(maxlen=10000)
        self.latency_measurements = deque(maxlen=1000)
        
        # Real-time stats
        self.current_tps = 0
        self.current_ops = 0
        self.avg_latency = 0
        self.peak_tps = 0
        self.total_trades = 0
        
    def create_traders(self, num_traders=10, symbols=None):
        """Create high-frequency trading bots"""
        if symbols is None:
            symbols = ["AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"]
            
        self.traders = []
        for i in range(num_traders):
            trader = Trader(
                trader_id=f"HFT_BOT_{i+1:03d}",
                initial_cash=1000000,  # $1M per trader
                symbols=symbols,
                engine=self.engine
            )
            # Override for maximum speed
            trader.order_frequency = 0.01  # 10ms between orders
            trader.min_order_size = 1
            trader.max_order_size = 50
            
            self.traders.append(trader)
            self.engine.register_trader(trader)
    
    def start_benchmark(self, duration_seconds=60):
        """Start the performance benchmark"""
        print(f"🚀 Starting HFT Performance Benchmark")
        print(f"Duration: {duration_seconds} seconds")
        print(f"Traders: {len(self.traders)}")
        print(f"Symbols: {self.traders[0].symbols if self.traders else 'None'}")
        print("-" * 50)
        
        self.running = True
        self.start_time = time.time()
        
        # Start engine
        self.engine.start()
        
        # Start traders
        for trader in self.traders:
            trader.start_trading()
        
        # Start monitoring thread
        monitor_thread = threading.Thread(target=self._monitor_performance, daemon=True)
        monitor_thread.start()
        
        # Run for specified duration
        try:
            time.sleep(duration_seconds)
        except KeyboardInterrupt:
            print("\nBenchmark interrupted by user")
        
        self.stop_benchmark()
    
    def stop_benchmark(self):
        """Stop the benchmark and print final results"""
        self.running = False
        
        # Stop traders
        for trader in self.traders:
            trader.stop_trading()
        
        # Stop engine
        self.engine.stop()
        
        self._print_final_results()
    
    def _monitor_performance(self):
        """Monitor and display real-time performance metrics"""
        last_update = time.time()
        last_trade_count = 0
        last_order_count = 0
        
        while self.running:
            current_time = time.time()
            
            # Update every 1 second
            if current_time - last_update >= 1.0:
                stats = self.engine.get_performance_stats()
                
                # Calculate rates
                current_trades = stats['total_trades']
                trade_delta = current_trades - last_trade_count
                self.current_tps = trade_delta / (current_time - last_update)
                
                # Update peak
                if self.current_tps > self.peak_tps:
                    self.peak_tps = self.current_tps
                
                # Calculate latency
                self.avg_latency = stats.get('avg_latency_ms', 0)
                
                # Print real-time stats
                elapsed = current_time - self.start_time
                self._print_realtime_stats(elapsed, stats)
                
                last_update = current_time
                last_trade_count = current_trades
            
            time.sleep(0.1)
    
    def _print_realtime_stats(self, elapsed, stats):
        """Print real-time performance statistics"""
        # Clear previous line and print new stats
        print(f"\r⚡ {elapsed:6.1f}s | "
              f"TPS: {self.current_tps:7.1f} | "
              f"Peak: {self.peak_tps:7.1f} | "
              f"Trades: {stats['total_trades']:6d} | "
              f"Orders: {stats['active_orders']:4d} | "
              f"Latency: {self.avg_latency:5.2f}ms", end="", flush=True)
    
    def _print_final_results(self):
        """Print comprehensive final benchmark results"""
        print("\n")
        print("=" * 60)
        print("🏁 BENCHMARK RESULTS")
        print("=" * 60)
        
        final_stats = self.engine.get_performance_stats()
        elapsed_time = final_stats['runtime_seconds']
        
        print(f"📊 PERFORMANCE METRICS:")
        print(f"   Total Runtime:        {elapsed_time:.2f} seconds")
        print(f"   Total Trades:         {final_stats['total_trades']:,}")
        print(f"   Total Volume:         {final_stats['total_volume']:,} shares")
        print(f"   Average TPS:          {final_stats['trades_per_second']:.1f}")
        print(f"   Peak TPS:             {self.peak_tps:.1f}")
        print(f"   Average Latency:      {final_stats['avg_latency_ms']:.2f} ms")
        print(f"   Active Symbols:       {final_stats['symbols_active']}")
        
        print(f"\n💰 TRADING STATISTICS:")
        total_pnl = sum(trader.get_total_pnl() for trader in self.traders)
        total_orders = sum(trader.orders_sent for trader in self.traders)
        total_fills = sum(trader.orders_filled for trader in self.traders)
        fill_rate = (total_fills / max(1, total_orders)) * 100
        
        print(f"   Total Orders Sent:    {total_orders:,}")
        print(f"   Total Orders Filled:  {total_fills:,}")
        print(f"   Fill Rate:            {fill_rate:.1f}%")
        print(f"   Combined P&L:         ${total_pnl:,.2f}")
        
        print(f"\n🔥 THROUGHPUT ANALYSIS:")
        if elapsed_time > 0:
            orders_per_second = total_orders / elapsed_time
            volume_per_second = final_stats['total_volume'] / elapsed_time
            print(f"   Orders/Second:        {orders_per_second:.1f}")
            print(f"   Volume/Second:        {volume_per_second:,.0f} shares")
            print(f"   Microseconds/Trade:   {(elapsed_time * 1000000) / max(1, final_stats['total_trades']):.0f} μs")
        
        # Performance rating
        print(f"\n⭐ PERFORMANCE RATING:")
        if self.peak_tps >= 1000:
            rating = "🚀 ULTRA HIGH FREQUENCY"
        elif self.peak_tps >= 500:
            rating = "⚡ HIGH FREQUENCY"
        elif self.peak_tps >= 100:
            rating = "🔥 FAST"
        elif self.peak_tps >= 50:
            rating = "✅ GOOD"
        else:
            rating = "⚠️  SLOW"
        
        print(f"   {rating}")
        print("=" * 60)

def main():
    """Run HFT performance benchmark"""
    benchmark = PerformanceBenchmark()
    
    print("HFT Trading Simulation - Performance Benchmark")
    print("=" * 50)
    
    # Configuration
    num_traders = int(input("Number of traders (default 10): ") or "10")
    duration = int(input("Duration in seconds (default 30): ") or "30")
    
    # Create traders
    benchmark.create_traders(num_traders)
    
    # Run benchmark
    benchmark.start_benchmark(duration)

if __name__ == "__main__":
    main()