import heapq
from collections import defaultdict, deque
from datetime import datetime
import threading

from models.order import Order, OrderSide, OrderStatus

class OrderBookSide:
    """
    Represents one side of an order book (bids or asks)
    """
    
    def __init__(self, is_bid_side=True):
        """
        Initialize order book side
        
        Args:
            is_bid_side (bool): True for bid side (buy orders), False for ask side (sell orders)
        """
        self.is_bid_side = is_bid_side
        self.price_levels = {}  # price -> list of orders
        self.price_heap = []  # Priority queue for prices
        self.orders = {}  # order_id -> order mapping
        self.lock = threading.Lock()
    
    def add_order(self, order):
        """Add an order to this side of the book"""
        with self.lock:
            price = order.price
            
            # Add to price level
            if price not in self.price_levels:
                self.price_levels[price] = deque()
                # For bids: higher prices have higher priority (max heap)
                # For asks: lower prices have higher priority (min heap)
                if self.is_bid_side:
                    heapq.heappush(self.price_heap, -price)  # Negative for max heap
                else:
                    heapq.heappush(self.price_heap, price)   # Positive for min heap
            
            self.price_levels[price].append(order)
            self.orders[order.order_id] = order
    
    def remove_order(self, order_id):
        """Remove an order from this side of the book"""
        with self.lock:
            if order_id not in self.orders:
                return False
            
            order = self.orders[order_id]
            price = order.price
            
            # Remove from price level
            if price in self.price_levels:
                try:
                    self.price_levels[price].remove(order)
                    if not self.price_levels[price]:
                        # Remove empty price level
                        del self.price_levels[price]
                        # Note: We don't remove from heap for efficiency
                        # The heap cleanup happens in get_best_price()
                except ValueError:
                    pass  # Order not in deque
            
            # Remove from orders mapping
            del self.orders[order_id]
            return True
    
    def get_best_price(self):
        """Get the best price on this side"""
        with self.lock:
            # Clean up heap - remove prices that no longer have orders
            while self.price_heap:
                if self.is_bid_side:
                    price = -self.price_heap[0]  # Convert back from negative
                else:
                    price = self.price_heap[0]
                
                if price in self.price_levels and self.price_levels[price]:
                    return price
                else:
                    heapq.heappop(self.price_heap)
            
            return None
    
    def get_best_order(self):
        """Get the best order (first order at best price)"""
        with self.lock:
            best_price = self.get_best_price()
            if best_price and best_price in self.price_levels:
                if self.price_levels[best_price]:
                    return self.price_levels[best_price][0]
            return None
    
    def get_orders_at_price(self, price):
        """Get all orders at a specific price level"""
        with self.lock:
            return list(self.price_levels.get(price, []))
    
    def get_top_levels(self, num_levels):
        """Get top N price levels with their orders"""
        with self.lock:
            levels = []
            prices_seen = set()
            
            # Create a copy of heap for iteration
            heap_copy = self.price_heap[:]
            
            while heap_copy and len(levels) < num_levels:
                if self.is_bid_side:
                    price = -heapq.heappop(heap_copy)
                else:
                    price = heapq.heappop(heap_copy)
                
                if price in prices_seen:
                    continue
                    
                prices_seen.add(price)
                
                if price in self.price_levels and self.price_levels[price]:
                    # Aggregate orders at this price level
                    orders = list(self.price_levels[price])
                    total_quantity = sum(order.quantity for order in orders)
                    
                    levels.append({
                        'price': price,
                        'quantity': total_quantity,
                        'order_count': len(orders),
                        'orders': orders
                    })
            
            return levels
    
    def get_total_volume(self):
        """Get total volume on this side"""
        with self.lock:
            return sum(order.quantity for order in self.orders.values())

class OrderBook:
    """
    Complete order book for a trading symbol
    """
    
    def __init__(self, symbol):
        """
        Initialize order book for a symbol
        
        Args:
            symbol (str): Trading symbol (e.g., 'AAPL')
        """
        self.symbol = symbol
        self.bids = OrderBookSide(is_bid_side=True)   # Buy orders
        self.asks = OrderBookSide(is_bid_side=False)  # Sell orders
        self.trade_history = deque(maxlen=1000)  # Recent trades
        self.lock = threading.Lock()
    
    def add_order(self, order):
        """Add an order to the appropriate side of the book"""
        if order.symbol != self.symbol:
            raise ValueError(f"Order symbol {order.symbol} doesn't match book symbol {self.symbol}")
        
        if order.side == OrderSide.BUY:
            self.bids.add_order(order)
        else:
            self.asks.add_order(order)
    
    def remove_order(self, order_id, side):
        """Remove an order from the book"""
        if side == OrderSide.BUY:
            return self.bids.remove_order(order_id)
        else:
            return self.asks.remove_order(order_id)
    
    def get_best_bid(self):
        """Get the best bid order"""
        return self.bids.get_best_order()
    
    def get_best_ask(self):
        """Get the best ask order"""
        return self.asks.get_best_order()
    
    def get_best_bid_price(self):
        """Get the best bid price"""
        return self.bids.get_best_price()
    
    def get_best_ask_price(self):
        """Get the best ask price"""
        return self.asks.get_best_price()
    
    def get_spread(self):
        """Get the bid-ask spread"""
        best_bid = self.get_best_bid_price()
        best_ask = self.get_best_ask_price()
        
        if best_bid is not None and best_ask is not None:
            return best_ask - best_bid
        return None
    
    def get_mid_price(self):
        """Get the mid price (average of best bid and ask)"""
        best_bid = self.get_best_bid_price()
        best_ask = self.get_best_ask_price()
        
        if best_bid is not None and best_ask is not None:
            return (best_bid + best_ask) / 2
        return None
    
    def get_top_levels(self, num_levels=5):
        """
        Get top N levels from both sides of the book
        
        Returns:
            tuple: (bids, asks) where each is a list of price level dictionaries
        """
        bids = self.bids.get_top_levels(num_levels)
        asks = self.asks.get_top_levels(num_levels)
        return bids, asks
    
    def add_trade(self, trade):
        """Add a trade to the history"""
        with self.lock:
            self.trade_history.append(trade)
    
    def get_recent_trades(self, count=10):
        """Get recent trades"""
        with self.lock:
            return list(self.trade_history)[-count:]
    
    def get_volume_at_price(self, price, side):
        """Get total volume at a specific price"""
        if side == OrderSide.BUY:
            orders = self.bids.get_orders_at_price(price)
        else:
            orders = self.asks.get_orders_at_price(price)
        
        return sum(order.quantity for order in orders)
    
    def get_market_depth(self, max_levels=10):
        """Get market depth (cumulative volume at each price level)"""
        bids, asks = self.get_top_levels(max_levels)
        
        # Calculate cumulative volumes
        bid_depth = []
        ask_depth = []
        
        cumulative_bid_volume = 0
        for level in bids:
            cumulative_bid_volume += level['quantity']
            bid_depth.append({
                'price': level['price'],
                'quantity': level['quantity'],
                'cumulative_volume': cumulative_bid_volume
            })
        
        cumulative_ask_volume = 0
        for level in asks:
            cumulative_ask_volume += level['quantity']
            ask_depth.append({
                'price': level['price'],
                'quantity': level['quantity'],
                'cumulative_volume': cumulative_ask_volume
            })
        
        return bid_depth, ask_depth
    
    def get_snapshot(self):
        """Get a complete snapshot of the order book"""
        bids, asks = self.get_top_levels(10)
        
        return {
            'symbol': self.symbol,
            'timestamp': datetime.now(),
            'bids': bids,
            'asks': asks,
            'best_bid': self.get_best_bid_price(),
            'best_ask': self.get_best_ask_price(),
            'spread': self.get_spread(),
            'mid_price': self.get_mid_price()
        }
    
    def is_crossed(self):
        """Check if the book is crossed (bid >= ask)"""
        best_bid = self.get_best_bid_price()
        best_ask = self.get_best_ask_price()
        
        if best_bid is not None and best_ask is not None:
            return best_bid >= best_ask
        return False
    
    def get_statistics(self):
        """Get order book statistics"""
        bids, asks = self.get_top_levels(5)
        
        return {
            'symbol': self.symbol,
            'total_bid_volume': self.bids.get_total_volume(),
            'total_ask_volume': self.asks.get_total_volume(),
            'bid_levels': len(self.bids.price_levels),
            'ask_levels': len(self.asks.price_levels),
            'total_orders': len(self.bids.orders) + len(self.asks.orders),
            'spread': self.get_spread(),
            'mid_price': self.get_mid_price(),
            'is_crossed': self.is_crossed()
        }
