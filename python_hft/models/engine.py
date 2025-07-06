import threading
import time
from datetime import datetime, timedelta
from collections import defaultdict, deque
import queue

from models.order import Order, OrderSide, OrderStatus
from models.orderbook import OrderBook

class TradingEngine:
    """
    Main trading engine that handles order matching and execution
    """
    
    def __init__(self):
        """Initialize the trading engine"""
        self.orderbooks = {}  # symbol -> OrderBook
        self.active_orders = {}  # order_id -> order
        self.traders = {}  # trader_id -> trader reference
        
        # Trade execution tracking
        self.trade_history = deque(maxlen=10000)
        self.executions_today = 0
        
        # Performance metrics
        self.start_time = datetime.now()
        self.total_trades = 0
        self.total_volume = 0
        self.latency_measurements = deque(maxlen=1000)
        
        # Threading
        self.is_running = False
        self.order_queue = queue.Queue()
        self.execution_thread = None
        self.stats_lock = threading.Lock()
        self.orders_lock = threading.Lock()
        
        # Order processing statistics (optimized for HFT)
        self.orders_per_second = 0
        self.last_stats_update = time.time()
        self.orders_processed_since_last_update = 0
        self.batch_size = 100  # Process orders in batches for better performance
    
    def start(self):
        """Start the trading engine"""
        if not self.is_running:
            self.is_running = True
            self.execution_thread = threading.Thread(target=self._execution_loop, daemon=True)
            self.execution_thread.start()
    
    def stop(self):
        """Stop the trading engine"""
        self.is_running = False
        if self.execution_thread and self.execution_thread.is_alive():
            self.execution_thread.join(timeout=2.0)
    
    def register_trader(self, trader):
        """Register a trader with the engine"""
        self.traders[trader.trader_id] = trader
    
    def get_orderbook(self, symbol):
        """Get or create order book for a symbol"""
        if symbol not in self.orderbooks:
            self.orderbooks[symbol] = OrderBook(symbol)
        return self.orderbooks[symbol]
    
    def submit_order(self, order):
        """Submit an order for processing"""
        order_submit_time = time.time()
        order.submit_time = order_submit_time
        
        # Add to queue for processing
        self.order_queue.put(order)
    
    def _execution_loop(self):
        """Main execution loop that processes orders (optimized for HFT)"""
        while self.is_running:
            try:
                # Process orders in batches for better performance
                orders_to_process = []
                
                # Collect batch of orders with minimal blocking
                start_time = time.time()
                while len(orders_to_process) < self.batch_size and (time.time() - start_time) < 0.001:
                    try:
                        order = self.order_queue.get_nowait()
                        orders_to_process.append(order)
                    except queue.Empty:
                        break
                
                # Process the batch
                for order in orders_to_process:
                    self._process_order(order)
                    self.order_queue.task_done()
                
                # If no orders, small sleep to prevent busy waiting
                if not orders_to_process:
                    time.sleep(0.0001)  # 0.1ms sleep for HFT performance
                    
            except Exception as e:
                print(f"Error in execution loop: {e}")
                time.sleep(0.001)  # Brief pause on error
    
    def _process_order(self, order):
        """Process a single order"""
        process_start = time.time()
        
        with self.orders_lock:
            # Add to active orders
            self.active_orders[order.order_id] = order
            
            # Get order book
            orderbook = self.get_orderbook(order.symbol)
            
            # Try to match the order
            self._match_order(order, orderbook)
            
            # If order still has quantity, add to book
            if order.is_active() and order.quantity > 0:
                orderbook.add_order(order)
            else:
                # Remove from active orders if completely filled or cancelled
                if order.order_id in self.active_orders:
                    del self.active_orders[order.order_id]
        
        # Record processing latency
        process_end = time.time()
        latency_ms = (process_end - process_start) * 1000
        
        if hasattr(order, 'submit_time'):
            total_latency_ms = (process_end - order.submit_time) * 1000
            self.latency_measurements.append(total_latency_ms)
        
        # Update statistics
        self._update_processing_stats()
    
    def _match_order(self, incoming_order, orderbook):
        """Match an incoming order against the order book"""
        if incoming_order.side == OrderSide.BUY:
            # Match buy order against asks (sell orders)
            self._match_buy_order(incoming_order, orderbook)
        else:
            # Match sell order against bids (buy orders)
            self._match_sell_order(incoming_order, orderbook)
    
    def _match_buy_order(self, buy_order, orderbook):
        """Match a buy order against asks"""
        while buy_order.quantity > 0 and buy_order.is_active():
            best_ask = orderbook.get_best_ask()
            
            if not best_ask or best_ask.price > buy_order.price:
                break  # No more matching orders
            
            # Execute trade
            trade_quantity = min(buy_order.quantity, best_ask.quantity)
            trade_price = best_ask.price  # Price-time priority: use ask price
            
            self._execute_trade(buy_order, best_ask, trade_quantity, trade_price, orderbook)
            
            # Remove ask order if completely filled
            if best_ask.quantity == 0:
                orderbook.remove_order(best_ask.order_id, OrderSide.SELL)
                if best_ask.order_id in self.active_orders:
                    del self.active_orders[best_ask.order_id]
    
    def _match_sell_order(self, sell_order, orderbook):
        """Match a sell order against bids"""
        while sell_order.quantity > 0 and sell_order.is_active():
            best_bid = orderbook.get_best_bid()
            
            if not best_bid or best_bid.price < sell_order.price:
                break  # No more matching orders
            
            # Execute trade
            trade_quantity = min(sell_order.quantity, best_bid.quantity)
            trade_price = best_bid.price  # Price-time priority: use bid price
            
            self._execute_trade(best_bid, sell_order, trade_quantity, trade_price, orderbook)
            
            # Remove bid order if completely filled
            if best_bid.quantity == 0:
                orderbook.remove_order(best_bid.order_id, OrderSide.BUY)
                if best_bid.order_id in self.active_orders:
                    del self.active_orders[best_bid.order_id]
    
    def _execute_trade(self, buy_order, sell_order, quantity, price, orderbook):
        """Execute a trade between two orders"""
        trade_time = datetime.now()
        
        # Fill both orders
        buy_order.fill(quantity, price)
        sell_order.fill(quantity, price)
        
        # Create trade record
        trade = {
            'trade_id': f"{self.total_trades + 1:06d}",
            'timestamp': trade_time,
            'symbol': buy_order.symbol,
            'quantity': quantity,
            'price': price,
            'buyer_id': buy_order.trader_id,
            'seller_id': sell_order.trader_id,
            'buy_order_id': buy_order.order_id,
            'sell_order_id': sell_order.order_id,
            'side': 'BUY'  # From the perspective of the aggressive order
        }
        
        # Add to trade history
        with self.stats_lock:
            self.trade_history.append(trade)
            self.total_trades += 1
            self.total_volume += quantity
        
        # Add to order book trade history
        orderbook.add_trade(trade)
        
        # Notify traders of fills
        self._notify_trader_fill(buy_order, quantity, price)
        self._notify_trader_fill(sell_order, quantity, price)
    
    def _notify_trader_fill(self, order, quantity, price):
        """Notify a trader that their order was filled"""
        trader_id = order.trader_id
        if trader_id in self.traders:
            try:
                trader = self.traders[trader_id]
                trader.on_order_filled(order, quantity, price)
            except Exception as e:
                print(f"Error notifying trader {trader_id}: {e}")
    
    def _update_processing_stats(self):
        """Update processing statistics"""
        current_time = time.time()
        self.orders_processed_since_last_update += 1
        
        # Update stats every second
        if current_time - self.last_stats_update >= 1.0:
            self.orders_per_second = self.orders_processed_since_last_update
            self.orders_processed_since_last_update = 0
            self.last_stats_update = current_time
    
    def cancel_order(self, order_id):
        """Cancel an order"""
        with self.orders_lock:
            if order_id in self.active_orders:
                order = self.active_orders[order_id]
                order.cancel()
                
                # Remove from order book
                orderbook = self.get_orderbook(order.symbol)
                orderbook.remove_order(order_id, order.side)
                
                # Remove from active orders
                del self.active_orders[order_id]
                return True
        return False
    
    def get_recent_trades(self, count=20):
        """Get recent trades across all symbols"""
        with self.stats_lock:
            return list(self.trade_history)[-count:] if count > 0 else list(self.trade_history)
    
    def get_recent_trades_for_symbol(self, symbol, count=10):
        """Get recent trades for a specific symbol"""
        with self.stats_lock:
            symbol_trades = [trade for trade in self.trade_history if trade['symbol'] == symbol]
            return symbol_trades[-count:] if count > 0 else symbol_trades
    
    def get_all_trades(self):
        """Get all trades for export"""
        with self.stats_lock:
            return list(self.trade_history)
    
    def get_performance_stats(self):
        """Get engine performance statistics"""
        with self.stats_lock:
            current_time = datetime.now()
            runtime_seconds = (current_time - self.start_time).total_seconds()
            
            # Calculate trades per second
            trades_per_second = self.total_trades / max(1, runtime_seconds)
            
            # Calculate average latency
            avg_latency_ms = 0
            if self.latency_measurements:
                avg_latency_ms = sum(self.latency_measurements) / len(self.latency_measurements)
            
            # Count active orders
            active_orders_count = len(self.active_orders)
            
            return {
                'total_trades': self.total_trades,
                'total_volume': self.total_volume,
                'trades_per_second': trades_per_second,
                'orders_per_second': self.orders_per_second,
                'avg_latency_ms': avg_latency_ms,
                'active_orders': active_orders_count,
                'runtime_seconds': runtime_seconds,
                'symbols_active': len(self.orderbooks)
            }
    
    def get_market_summary(self):
        """Get summary of all markets"""
        summary = {}
        
        for symbol, orderbook in self.orderbooks.items():
            stats = orderbook.get_statistics()
            recent_trades = orderbook.get_recent_trades(5)
            
            # Calculate volume-weighted average price (VWAP) for recent trades
            vwap = 0
            if recent_trades:
                total_value = sum(trade['price'] * trade['quantity'] for trade in recent_trades)
                total_volume = sum(trade['quantity'] for trade in recent_trades)
                if total_volume > 0:
                    vwap = total_value / total_volume
            
            summary[symbol] = {
                'best_bid': stats['mid_price'],
                'best_ask': stats['mid_price'],
                'spread': stats['spread'],
                'mid_price': stats['mid_price'],
                'vwap': vwap,
                'volume': stats['total_bid_volume'] + stats['total_ask_volume'],
                'trade_count': len(recent_trades)
            }
        
        return summary
    
    def get_trader_orders(self, trader_id):
        """Get all active orders for a trader"""
        with self.orders_lock:
            return [order for order in self.active_orders.values() 
                    if order.trader_id == trader_id]
    
    def get_symbol_statistics(self, symbol):
        """Get detailed statistics for a symbol"""
        if symbol not in self.orderbooks:
            return None
        
        orderbook = self.orderbooks[symbol]
        recent_trades = self.get_recent_trades_for_symbol(symbol, 100)
        
        # Calculate price statistics
        if recent_trades:
            prices = [trade['price'] for trade in recent_trades]
            high_price = max(prices)
            low_price = min(prices)
            last_price = prices[-1] if prices else 0
            
            # Calculate VWAP
            total_value = sum(trade['price'] * trade['quantity'] for trade in recent_trades)
            total_volume = sum(trade['quantity'] for trade in recent_trades)
            vwap = total_value / total_volume if total_volume > 0 else 0
        else:
            high_price = low_price = last_price = vwap = 0
        
        return {
            'symbol': symbol,
            'last_price': last_price,
            'high_price': high_price,
            'low_price': low_price,
            'vwap': vwap,
            'total_volume': sum(trade['quantity'] for trade in recent_trades),
            'trade_count': len(recent_trades),
            'orderbook_stats': orderbook.get_statistics()
        }
