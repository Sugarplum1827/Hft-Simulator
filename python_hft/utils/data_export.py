import csv
import io
from datetime import datetime
import pandas as pd

class DataExporter:
    """
    Utility class for exporting trading data to various formats
    """
    
    def __init__(self):
        """Initialize the data exporter"""
        pass
    
    def export_trades_to_csv(self, trades):
        """
        Export trade data to CSV format
        
        Args:
            trades (list): List of trade dictionaries
            
        Returns:
            str: CSV formatted string
        """
        if not trades:
            return ""
        
        # Create CSV in memory
        output = io.StringIO()
        
        # Define CSV headers
        headers = [
            'Trade ID', 'Timestamp', 'Symbol', 'Side', 'Quantity', 
            'Price', 'Value', 'Buyer ID', 'Seller ID', 
            'Buy Order ID', 'Sell Order ID'
        ]
        
        writer = csv.writer(output)
        writer.writerow(headers)
        
        # Write trade data
        for trade in trades:
            row = [
                trade.get('trade_id', ''),
                trade.get('timestamp', '').strftime('%Y-%m-%d %H:%M:%S.%f') if isinstance(trade.get('timestamp'), datetime) else str(trade.get('timestamp', '')),
                trade.get('symbol', ''),
                trade.get('side', ''),
                trade.get('quantity', 0),
                f"{trade.get('price', 0):.4f}",
                f"{trade.get('quantity', 0) * trade.get('price', 0):.2f}",
                trade.get('buyer_id', ''),
                trade.get('seller_id', ''),
                trade.get('buy_order_id', ''),
                trade.get('sell_order_id', '')
            ]
            writer.writerow(row)
        
        csv_content = output.getvalue()
        output.close()
        
        return csv_content
    
    def export_orderbook_to_csv(self, orderbook_data):
        """
        Export order book snapshot data to CSV format
        
        Args:
            orderbook_data (dict): Dictionary with symbol -> orderbook snapshot
            
        Returns:
            str: CSV formatted string
        """
        if not orderbook_data:
            return ""
        
        # Create CSV in memory
        output = io.StringIO()
        
        # Define CSV headers
        headers = [
            'Symbol', 'Timestamp', 'Side', 'Price Level', 'Price', 
            'Quantity', 'Order Count', 'Cumulative Volume'
        ]
        
        writer = csv.writer(output)
        writer.writerow(headers)
        
        # Write orderbook data
        for symbol, snapshot in orderbook_data.items():
            if not snapshot:
                continue
                
            timestamp = snapshot.get('timestamp', datetime.now())
            timestamp_str = timestamp.strftime('%Y-%m-%d %H:%M:%S.%f') if isinstance(timestamp, datetime) else str(timestamp)
            
            # Write bid levels
            bids = snapshot.get('bids', [])
            cumulative_bid_volume = 0
            for i, bid_level in enumerate(bids):
                cumulative_bid_volume += bid_level.get('quantity', 0)
                row = [
                    symbol,
                    timestamp_str,
                    'BID',
                    i + 1,  # Price level (1 = best)
                    f"{bid_level.get('price', 0):.4f}",
                    bid_level.get('quantity', 0),
                    bid_level.get('order_count', 0),
                    cumulative_bid_volume
                ]
                writer.writerow(row)
            
            # Write ask levels
            asks = snapshot.get('asks', [])
            cumulative_ask_volume = 0
            for i, ask_level in enumerate(asks):
                cumulative_ask_volume += ask_level.get('quantity', 0)
                row = [
                    symbol,
                    timestamp_str,
                    'ASK',
                    i + 1,  # Price level (1 = best)
                    f"{ask_level.get('price', 0):.4f}",
                    ask_level.get('quantity', 0),
                    ask_level.get('order_count', 0),
                    cumulative_ask_volume
                ]
                writer.writerow(row)
        
        csv_content = output.getvalue()
        output.close()
        
        return csv_content
    
    def export_trader_performance_to_csv(self, traders):
        """
        Export trader performance data to CSV format
        
        Args:
            traders (list): List of trader objects
            
        Returns:
            str: CSV formatted string
        """
        if not traders:
            return ""
        
        # Create CSV in memory
        output = io.StringIO()
        
        # Define CSV headers
        headers = [
            'Trader ID', 'Initial Cash', 'Current Cash', 'Portfolio Value',
            'Total P&L', 'P&L %', 'Orders Sent', 'Orders Filled', 'Fill Rate %',
            'Total Volume', 'Avg Order Size'
        ]
        
        writer = csv.writer(output)
        writer.writerow(headers)
        
        # Write trader performance data
        for trader in traders:
            portfolio_value = trader.get_portfolio_value()
            total_pnl = trader.get_total_pnl()
            pnl_percentage = (total_pnl / trader.initial_cash * 100) if trader.initial_cash > 0 else 0
            fill_rate = (trader.orders_filled / max(1, trader.orders_sent) * 100)
            avg_order_size = trader.total_volume / max(1, trader.orders_filled)
            
            row = [
                trader.trader_id,
                f"{trader.initial_cash:.2f}",
                f"{trader.cash:.2f}",
                f"{portfolio_value:.2f}",
                f"{total_pnl:.2f}",
                f"{pnl_percentage:.2f}",
                trader.orders_sent,
                trader.orders_filled,
                f"{fill_rate:.2f}",
                trader.total_volume,
                f"{avg_order_size:.2f}"
            ]
            writer.writerow(row)
        
        csv_content = output.getvalue()
        output.close()
        
        return csv_content
    
    def export_market_summary_to_csv(self, market_summary):
        """
        Export market summary data to CSV format
        
        Args:
            market_summary (dict): Dictionary with symbol -> market stats
            
        Returns:
            str: CSV formatted string
        """
        if not market_summary:
            return ""
        
        # Create CSV in memory
        output = io.StringIO()
        
        # Define CSV headers
        headers = [
            'Symbol', 'Best Bid', 'Best Ask', 'Spread', 'Spread %',
            'Mid Price', 'VWAP', 'Volume', 'Trade Count'
        ]
        
        writer = csv.writer(output)
        writer.writerow(headers)
        
        # Write market summary data
        for symbol, stats in market_summary.items():
            best_bid = stats.get('best_bid', 0)
            best_ask = stats.get('best_ask', 0)
            spread = stats.get('spread', 0)
            mid_price = stats.get('mid_price', 0)
            
            # Calculate spread percentage
            spread_percentage = (spread / mid_price * 100) if mid_price > 0 else 0
            
            row = [
                symbol,
                f"{best_bid:.4f}" if best_bid else "",
                f"{best_ask:.4f}" if best_ask else "",
                f"{spread:.4f}" if spread else "",
                f"{spread_percentage:.4f}",
                f"{mid_price:.4f}" if mid_price else "",
                f"{stats.get('vwap', 0):.4f}",
                stats.get('volume', 0),
                stats.get('trade_count', 0)
            ]
            writer.writerow(row)
        
        csv_content = output.getvalue()
        output.close()
        
        return csv_content
    
    def export_performance_metrics_to_csv(self, performance_stats):
        """
        Export engine performance metrics to CSV format
        
        Args:
            performance_stats (dict): Dictionary with performance metrics
            
        Returns:
            str: CSV formatted string
        """
        if not performance_stats:
            return ""
        
        # Create CSV in memory
        output = io.StringIO()
        
        # Define CSV headers
        headers = ['Metric', 'Value', 'Unit']
        
        writer = csv.writer(output)
        writer.writerow(headers)
        
        # Write performance metrics
        metrics = [
            ('Total Trades', performance_stats.get('total_trades', 0), 'count'),
            ('Total Volume', performance_stats.get('total_volume', 0), 'shares'),
            ('Trades Per Second', f"{performance_stats.get('trades_per_second', 0):.2f}", 'trades/sec'),
            ('Orders Per Second', f"{performance_stats.get('orders_per_second', 0):.2f}", 'orders/sec'),
            ('Average Latency', f"{performance_stats.get('avg_latency_ms', 0):.2f}", 'milliseconds'),
            ('Active Orders', performance_stats.get('active_orders', 0), 'count'),
            ('Runtime', f"{performance_stats.get('runtime_seconds', 0):.2f}", 'seconds'),
            ('Active Symbols', performance_stats.get('symbols_active', 0), 'count')
        ]
        
        for metric_name, value, unit in metrics:
            writer.writerow([metric_name, value, unit])
        
        csv_content = output.getvalue()
        output.close()
        
        return csv_content
    
    def create_trade_analysis_dataframe(self, trades):
        """
        Create a pandas DataFrame from trade data for analysis
        
        Args:
            trades (list): List of trade dictionaries
            
        Returns:
            pd.DataFrame: DataFrame with trade data
        """
        if not trades:
            return pd.DataFrame()
        
        # Convert trades to DataFrame
        df_data = []
        for trade in trades:
            row = {
                'trade_id': trade.get('trade_id', ''),
                'timestamp': trade.get('timestamp', datetime.now()),
                'symbol': trade.get('symbol', ''),
                'side': trade.get('side', ''),
                'quantity': trade.get('quantity', 0),
                'price': trade.get('price', 0),
                'value': trade.get('quantity', 0) * trade.get('price', 0),
                'buyer_id': trade.get('buyer_id', ''),
                'seller_id': trade.get('seller_id', '')
            }
            df_data.append(row)
        
        df = pd.DataFrame(df_data)
        
        # Set timestamp as index if available
        if 'timestamp' in df.columns and not df.empty:
            df.set_index('timestamp', inplace=True)
        
        return df
    
    def calculate_trading_statistics(self, trades):
        """
        Calculate comprehensive trading statistics
        
        Args:
            trades (list): List of trade dictionaries
            
        Returns:
            dict: Dictionary with calculated statistics
        """
        if not trades:
            return {}
        
        df = self.create_trade_analysis_dataframe(trades)
        
        if df.empty:
            return {}
        
        stats = {}
        
        # Overall statistics
        stats['total_trades'] = len(df)
        stats['total_volume'] = df['quantity'].sum()
        stats['total_value'] = df['value'].sum()
        stats['average_price'] = df['price'].mean()
        stats['average_quantity'] = df['quantity'].mean()
        stats['average_trade_value'] = df['value'].mean()
        
        # Price statistics
        stats['price_std'] = df['price'].std()
        stats['price_min'] = df['price'].min()
        stats['price_max'] = df['price'].max()
        
        # Volume statistics
        stats['volume_std'] = df['quantity'].std()
        stats['volume_min'] = df['quantity'].min()
        stats['volume_max'] = df['quantity'].max()
        
        # Time-based statistics
        if len(df) > 1:
            time_diff = (df.index[-1] - df.index[0]).total_seconds()
            stats['trading_duration_seconds'] = time_diff
            stats['trades_per_minute'] = len(df) / (time_diff / 60) if time_diff > 0 else 0
        
        # Symbol-based statistics
        symbol_stats = df.groupby('symbol').agg({
            'quantity': ['count', 'sum', 'mean'],
            'value': 'sum',
            'price': ['mean', 'std', 'min', 'max']
        }).round(4)
        
        stats['by_symbol'] = symbol_stats.to_dict() if not symbol_stats.empty else {}
        
        return stats
