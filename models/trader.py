import random
import time
import threading
from datetime import datetime
import numpy as np

from models.order import Order, OrderSide

class Trader:
    """
    Simulated trading bot that generates orders
    """
    
    def __init__(self, trader_id, initial_cash, symbols, engine):
        """
        Initialize a trading bot
        
        Args:
            trader_id (str): Unique identifier for the trader
            initial_cash (float): Starting cash amount
            symbols (list): List of symbols to trade
            engine: Reference to the trading engine
        """
        self.trader_id = trader_id
        self.initial_cash = initial_cash
        self.cash = initial_cash
        self.symbols = symbols
        self.engine = engine
        
        # Portfolio tracking
        self.positions = {symbol: 0 for symbol in symbols}  # Share positions
        self.average_costs = {symbol: 0.0 for symbol in symbols}  # Average cost basis
        
        # Trading statistics
        self.orders_sent = 0
        self.orders_filled = 0
        self.total_volume = 0
        
        # Trading parameters
        self.min_order_size = 10
        self.max_order_size = 100
        self.price_volatility = 0.02  # 2% price variation
        self.order_frequency = 0.5  # Seconds between orders
        self.market_price_cache = {symbol: 100.0 for symbol in symbols}  # Starting prices
        
        # Threading
        self.is_active = False
        self.trade_thread = None
    
    def start_trading(self):
        """Start the trading bot in a separate thread"""
        if not self.is_active:
            self.is_active = True
            self.trade_thread = threading.Thread(target=self._trading_loop, daemon=True)
            self.trade_thread.start()
    
    def stop_trading(self):
        """Stop the trading bot"""
        self.is_active = False
        if self.trade_thread and self.trade_thread.is_alive():
            self.trade_thread.join(timeout=1.0)
    
    def _trading_loop(self):
        """Main trading loop that runs in a separate thread"""
        while self.is_active:
            try:
                # Random delay between orders
                delay = np.random.exponential(self.order_frequency)
                time.sleep(delay)
                
                if not self.is_active:
                    break
                
                # Generate a random order
                self._generate_order()
                
            except Exception as e:
                print(f"Error in trading loop for {self.trader_id}: {e}")
                time.sleep(1)  # Brief pause on error
    
    def _generate_order(self):
        """Generate and submit a random order"""
        if not self.symbols:
            return
        
        # Choose random symbol
        symbol = random.choice(self.symbols)
        
        # Get current market price estimate
        market_price = self._estimate_market_price(symbol)
        
        # Decide order side (buy/sell) with some bias based on position
        side = self._decide_order_side(symbol)
        
        # Generate order parameters
        quantity = random.randint(self.min_order_size, self.max_order_size)
        
        # Generate price with some randomness around market price
        price_variation = random.gauss(0, self.price_volatility)
        
        if side == OrderSide.BUY:
            # Buyers typically bid below market price
            price = market_price * (1 - abs(price_variation))
        else:
            # Sellers typically ask above market price
            price = market_price * (1 + abs(price_variation))
        
        # Round price to 2 decimal places
        price = round(price, 2)
        
        # Check if we can afford the order (for buy orders)
        if side == OrderSide.BUY and quantity * price > self.cash:
            # Reduce quantity or skip order
            affordable_quantity = int(self.cash / price)
            if affordable_quantity < self.min_order_size:
                return  # Can't afford minimum order
            quantity = affordable_quantity
        
        # Check if we have enough shares to sell
        if side == OrderSide.SELL and quantity > self.positions[symbol]:
            if self.positions[symbol] < self.min_order_size:
                return  # Not enough shares to sell
            quantity = self.positions[symbol]
        
        # Create and submit order
        order = Order(self.trader_id, symbol, side, quantity, price)
        self.engine.submit_order(order)
        self.orders_sent += 1
    
    def _estimate_market_price(self, symbol):
        """Estimate current market price based on recent trades and order book"""
        # Get recent trades for price discovery
        recent_trades = self.engine.get_recent_trades_for_symbol(symbol, 5)
        
        if recent_trades:
            # Use volume-weighted average of recent trades
            total_value = sum(trade['price'] * trade['quantity'] for trade in recent_trades)
            total_volume = sum(trade['quantity'] for trade in recent_trades)
            if total_volume > 0:
                self.market_price_cache[symbol] = total_value / total_volume
        else:
            # If no recent trades, use order book mid-price or random walk
            orderbook = self.engine.get_orderbook(symbol)
            best_bid = orderbook.get_best_bid()
            best_ask = orderbook.get_best_ask()
            
            if best_bid and best_ask:
                self.market_price_cache[symbol] = (best_bid.price + best_ask.price) / 2
            else:
                # Random walk from current price
                change = random.gauss(0, 0.01)  # 1% daily volatility
                self.market_price_cache[symbol] *= (1 + change)
                self.market_price_cache[symbol] = max(1.0, self.market_price_cache[symbol])
        
        return self.market_price_cache[symbol]
    
    def _decide_order_side(self, symbol):
        """Decide whether to buy or sell based on current position and market conditions"""
        position = self.positions[symbol]
        
        # If we have a large position, bias toward selling
        if position > 500:
            return OrderSide.SELL if random.random() < 0.7 else OrderSide.BUY
        # If we have no position, bias toward buying
        elif position == 0:
            return OrderSide.BUY if random.random() < 0.7 else OrderSide.SELL
        # Otherwise, random
        else:
            return random.choice([OrderSide.BUY, OrderSide.SELL])
    
    def on_order_filled(self, order, fill_quantity, fill_price):
        """
        Callback when an order is filled (called by the engine)
        
        Args:
            order: The filled order
            fill_quantity: Quantity filled
            fill_price: Price of the fill
        """
        symbol = order.symbol
        
        if order.side == OrderSide.BUY:
            # Update cash and position
            cost = fill_quantity * fill_price
            self.cash -= cost
            
            # Update average cost basis
            old_position = self.positions[symbol]
            old_cost_basis = self.average_costs[symbol] * old_position
            new_cost_basis = old_cost_basis + cost
            new_position = old_position + fill_quantity
            
            self.positions[symbol] = new_position
            if new_position > 0:
                self.average_costs[symbol] = new_cost_basis / new_position
            
        else:  # SELL
            # Update cash and position
            proceeds = fill_quantity * fill_price
            self.cash += proceeds
            self.positions[symbol] -= fill_quantity
            
            # If position goes to zero, reset average cost
            if self.positions[symbol] == 0:
                self.average_costs[symbol] = 0.0
        
        self.orders_filled += 1
        self.total_volume += fill_quantity
    
    def get_portfolio_value(self):
        """Calculate current portfolio value based on market prices"""
        portfolio_value = self.cash
        
        for symbol, position in self.positions.items():
            if position > 0:
                market_price = self._estimate_market_price(symbol)
                portfolio_value += position * market_price
        
        return portfolio_value
    
    def get_total_pnl(self):
        """Calculate total profit/loss"""
        return self.get_portfolio_value() - self.initial_cash
    
    def get_position_pnl(self, symbol):
        """Calculate P&L for a specific position"""
        position = self.positions[symbol]
        if position == 0:
            return 0.0
        
        market_price = self._estimate_market_price(symbol)
        market_value = position * market_price
        cost_basis = position * self.average_costs[symbol]
        
        return market_value - cost_basis
    
    def get_trading_stats(self):
        """Get trading statistics"""
        return {
            'trader_id': self.trader_id,
            'cash': self.cash,
            'portfolio_value': self.get_portfolio_value(),
            'total_pnl': self.get_total_pnl(),
            'orders_sent': self.orders_sent,
            'orders_filled': self.orders_filled,
            'total_volume': self.total_volume,
            'positions': self.positions.copy(),
            'fill_rate': self.orders_filled / max(1, self.orders_sent)
        }
