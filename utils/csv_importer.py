import pandas as pd
import io
from datetime import datetime
from typing import List, Dict, Optional
import logging

from models.order import Order, OrderSide

class CSVImporter:
    """
    Utility class for importing trading data from CSV files
    """
    
    def __init__(self):
        """Initialize the CSV importer"""
        self.logger = logging.getLogger(__name__)
    
    def validate_csv_format(self, csv_content: str) -> Dict[str, any]:
        """
        Validate CSV format and return validation results
        
        Args:
            csv_content (str): CSV content as string
            
        Returns:
            dict: Validation results with success status and error messages
        """
        try:
            # Read CSV content
            df = pd.read_csv(io.StringIO(csv_content))
            
            # Required columns
            required_columns = ['trader_id', 'symbol', 'side', 'quantity', 'price']
            
            # Check for required columns
            missing_columns = [col for col in required_columns if col not in df.columns]
            if missing_columns:
                return {
                    'success': False,
                    'error': f"Missing required columns: {', '.join(missing_columns)}",
                    'required_columns': required_columns,
                    'found_columns': list(df.columns)
                }
            
            # Validate data types and values
            errors = []
            
            # Check quantity is numeric and positive
            if not pd.api.types.is_numeric_dtype(df['quantity']):
                errors.append("Quantity column must contain numeric values")
            elif (df['quantity'] <= 0).any():
                errors.append("Quantity values must be positive")
            
            # Check price is numeric and positive
            if not pd.api.types.is_numeric_dtype(df['price']):
                errors.append("Price column must contain numeric values")
            elif (df['price'] <= 0).any():
                errors.append("Price values must be positive")
            
            # Check side values are valid
            valid_sides = ['BUY', 'SELL', 'buy', 'sell']
            invalid_sides = df[~df['side'].isin(valid_sides)]['side'].unique()
            if len(invalid_sides) > 0:
                errors.append(f"Invalid side values: {', '.join(invalid_sides)}. Must be BUY or SELL")
            
            # Check for empty values in required fields
            for col in required_columns:
                if df[col].isna().any():
                    errors.append(f"{col} column contains empty values")
            
            if errors:
                return {
                    'success': False,
                    'error': '; '.join(errors),
                    'row_count': len(df)
                }
            
            return {
                'success': True,
                'row_count': len(df),
                'symbols': sorted(df['symbol'].unique().tolist()),
                'traders': sorted(df['trader_id'].unique().tolist()),
                'preview': df.head().to_dict('records')
            }
            
        except Exception as e:
            return {
                'success': False,
                'error': f"Error reading CSV: {str(e)}"
            }
    
    def import_orders_from_csv(self, csv_content: str, engine) -> Dict[str, any]:
        """
        Import orders from CSV content and submit them to the trading engine
        
        Args:
            csv_content (str): CSV content as string
            engine: Trading engine instance
            
        Returns:
            dict: Import results with success status and statistics
        """
        try:
            # Validate CSV first
            validation = self.validate_csv_format(csv_content)
            if not validation['success']:
                return validation
            
            # Read CSV content
            df = pd.read_csv(io.StringIO(csv_content))
            
            # Convert and submit orders
            orders_submitted = 0
            orders_failed = 0
            errors = []
            
            for index, row in df.iterrows():
                try:
                    # Parse order data
                    trader_id = str(row['trader_id'])
                    symbol = str(row['symbol']).upper()
                    side_str = str(row['side']).upper()
                    quantity = int(row['quantity'])
                    price = float(row['price'])
                    
                    # Convert side to enum
                    if side_str == 'BUY':
                        side = OrderSide.BUY
                    elif side_str == 'SELL':
                        side = OrderSide.SELL
                    else:
                        errors.append(f"Row {index + 1}: Invalid side '{side_str}'")
                        orders_failed += 1
                        continue
                    
                    # Create and submit order
                    order = Order(trader_id, symbol, side, quantity, price)
                    
                    # Add timestamp if provided
                    if 'timestamp' in row and pd.notna(row['timestamp']):
                        try:
                            order.timestamp = pd.to_datetime(row['timestamp'])
                        except:
                            # Use current time if timestamp parsing fails
                            order.timestamp = datetime.now()
                    
                    # Submit to engine
                    engine.submit_order(order)
                    orders_submitted += 1
                    
                except Exception as e:
                    errors.append(f"Row {index + 1}: {str(e)}")
                    orders_failed += 1
            
            return {
                'success': True,
                'orders_submitted': orders_submitted,
                'orders_failed': orders_failed,
                'total_rows': len(df),
                'errors': errors,
                'symbols_imported': sorted(df['symbol'].str.upper().unique().tolist()),
                'traders_imported': sorted(df['trader_id'].astype(str).unique().tolist())
            }
            
        except Exception as e:
            return {
                'success': False,
                'error': f"Error importing orders: {str(e)}"
            }
    
    def get_sample_csv_format(self) -> str:
        """
        Get a sample CSV format for reference
        
        Returns:
            str: Sample CSV content
        """
        sample_data = [
            {
                'trader_id': 'TRADER_001',
                'symbol': 'AAPL',
                'side': 'BUY',
                'quantity': 100,
                'price': 150.25,
                'timestamp': '2025-07-06 10:00:00'
            },
            {
                'trader_id': 'TRADER_002',
                'symbol': 'AAPL',
                'side': 'SELL',
                'quantity': 75,
                'price': 150.50,
                'timestamp': '2025-07-06 10:00:15'
            },
            {
                'trader_id': 'TRADER_001',
                'symbol': 'GOOGL',
                'side': 'BUY',
                'quantity': 50,
                'price': 2800.75,
                'timestamp': '2025-07-06 10:00:30'
            }
        ]
        
        # Convert to CSV string
        df = pd.DataFrame(sample_data)
        return df.to_csv(index=False)
    
    def extract_symbols_from_csv(self, csv_content: str) -> List[str]:
        """
        Extract unique symbols from CSV content
        
        Args:
            csv_content (str): CSV content as string
            
        Returns:
            list: List of unique symbols
        """
        try:
            df = pd.read_csv(io.StringIO(csv_content))
            if 'symbol' in df.columns:
                return sorted(df['symbol'].str.upper().unique().tolist())
            return []
        except:
            return []
    
    def create_traders_from_csv(self, csv_content: str, initial_cash: float = 100000) -> Dict[str, Dict]:
        """
        Create trader configurations from CSV data
        
        Args:
            csv_content (str): CSV content as string
            initial_cash (float): Initial cash amount for each trader
            
        Returns:
            dict: Dictionary of trader configurations
        """
        try:
            df = pd.read_csv(io.StringIO(csv_content))
            
            if 'trader_id' not in df.columns:
                return {}
            
            # Get unique traders and their symbols
            trader_configs = {}
            
            for trader_id in df['trader_id'].unique():
                trader_data = df[df['trader_id'] == trader_id]
                symbols = sorted(trader_data['symbol'].str.upper().unique().tolist())
                
                trader_configs[str(trader_id)] = {
                    'trader_id': str(trader_id),
                    'initial_cash': initial_cash,
                    'symbols': symbols,
                    'order_count': len(trader_data)
                }
            
            return trader_configs
            
        except Exception as e:
            self.logger.error(f"Error creating trader configs: {e}")
            return {}