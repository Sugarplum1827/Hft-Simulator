from datetime import datetime
from enum import Enum
import uuid

class OrderSide(Enum):
    """Enumeration for order sides"""
    BUY = "BUY"
    SELL = "SELL"

class OrderStatus(Enum):
    """Enumeration for order status"""
    PENDING = "PENDING"
    PARTIALLY_FILLED = "PARTIALLY_FILLED"
    FILLED = "FILLED"
    CANCELLED = "CANCELLED"

class Order:
    """
    Represents a trading order in the system
    """
    
    def __init__(self, trader_id, symbol, side, quantity, price):
        """
        Initialize a new order
        
        Args:
            trader_id (str): ID of the trader placing the order
            symbol (str): Trading symbol (e.g., 'AAPL')
            side (OrderSide): BUY or SELL
            quantity (int): Number of shares
            price (float): Price per share
        """
        self.order_id = str(uuid.uuid4())
        self.trader_id = trader_id
        self.symbol = symbol
        self.side = side
        self.quantity = quantity
        self.original_quantity = quantity
        self.price = price
        self.status = OrderStatus.PENDING
        self.timestamp = datetime.now()
        self.fills = []  # List of partial fills
    
    def fill(self, quantity, price):
        """
        Fill part or all of the order
        
        Args:
            quantity (int): Quantity being filled
            price (float): Fill price
        """
        if quantity > self.quantity:
            raise ValueError("Fill quantity exceeds remaining order quantity")
        
        # Record the fill
        fill = {
            'quantity': quantity,
            'price': price,
            'timestamp': datetime.now()
        }
        self.fills.append(fill)
        
        # Update remaining quantity
        self.quantity -= quantity
        
        # Update status
        if self.quantity == 0:
            self.status = OrderStatus.FILLED
        else:
            self.status = OrderStatus.PARTIALLY_FILLED
    
    def cancel(self):
        """Cancel the order"""
        if self.status in [OrderStatus.PENDING, OrderStatus.PARTIALLY_FILLED]:
            self.status = OrderStatus.CANCELLED
    
    def get_filled_quantity(self):
        """Get total filled quantity"""
        return sum(fill['quantity'] for fill in self.fills)
    
    def get_average_fill_price(self):
        """Get average fill price"""
        if not self.fills:
            return 0.0
        
        total_value = sum(fill['quantity'] * fill['price'] for fill in self.fills)
        total_quantity = sum(fill['quantity'] for fill in self.fills)
        
        return total_value / total_quantity if total_quantity > 0 else 0.0
    
    def is_complete(self):
        """Check if order is completely filled"""
        return self.status == OrderStatus.FILLED
    
    def is_active(self):
        """Check if order is still active (can be filled)"""
        return self.status in [OrderStatus.PENDING, OrderStatus.PARTIALLY_FILLED]
    
    def __str__(self):
        return (f"Order({self.order_id[:8]}, {self.trader_id}, "
                f"{self.symbol}, {self.side.value}, "
                f"{self.quantity}@{self.price:.2f})")
    
    def __repr__(self):
        return self.__str__()
    
    def to_dict(self):
        """Convert order to dictionary for serialization"""
        return {
            'order_id': self.order_id,
            'trader_id': self.trader_id,
            'symbol': self.symbol,
            'side': self.side.value,
            'quantity': self.quantity,
            'original_quantity': self.original_quantity,
            'price': self.price,
            'status': self.status.value,
            'timestamp': self.timestamp,
            'fills': self.fills
        }
