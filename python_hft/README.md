# Python HFT Trading Simulation - Web Application

A comprehensive high-frequency trading simulation built with Streamlit, featuring real-time order matching, customizable trading symbols, CSV import/export, and performance analytics.

![Python HFT Simulator](https://img.shields.io/badge/Python-HFT_Simulator-blue.svg) ![Streamlit](https://img.shields.io/badge/Streamlit-Web_App-red.svg) ![Performance](https://img.shields.io/badge/Performance-2000+_TPS-green.svg)

## Features

### Core Trading Engine
- **Real-time Order Matching**: Price-time priority matching algorithm
- **Multi-threaded Architecture**: Separate threads for order processing and trading bots
- **High-Performance Processing**: Optimized batch processing for maximum throughput
- **Thread-safe Operations**: Mutex-protected data structures for concurrent access

### Web Interface
- **Modern Streamlit GUI**: Professional trading interface with real-time updates
- **High-Frequency Mode**: Toggle for maximum performance (50ms order generation)
- **Real-time Order Book**: Live visualization of top 5 bids and asks with interactive charts
- **Trade Execution Log**: Real-time trade history with detailed analytics
- **Performance Metrics**: Live trades per second, latency, and throughput monitoring

### Trading Simulation
- **Configurable Trading Bots**: 1-20 bots with customizable parameters
- **Dynamic Symbol Management**: Add custom symbols or import from CSV
- **Realistic Market Dynamics**: Market-making and price discovery simulation
- **Portfolio Management**: Real-time P&L tracking for each trader

### Data Management
- **CSV Import/Export**: Upload order files with validation and preview
- **Sample Data**: Included sample CSV files for testing
- **Order Book Snapshots**: Export current market state
- **Performance Analytics**: Comprehensive trading statistics

## Requirements

- Python 3.11+
- Streamlit
- Pandas
- NumPy
- Plotly

## Installation

### 1. Install Dependencies
```bash
pip install streamlit pandas numpy plotly
```

### 2. Run the Application
```bash
streamlit run app.py --server.port 5000
```

### 3. Access the Interface
Open your browser to `http://localhost:5000`

## Quick Start Guide

### 1. Configure Simulation
- Toggle "High-Frequency Mode" for maximum performance
- Set number of trading bots (1-20)
- Set initial cash per bot
- Select or add custom trading symbols

### 2. Import Data (Optional)
- Use "Import CSV" tab to upload order files
- Download sample CSV for format reference
- Load provided sample data for testing

### 3. Start Trading
- Click "Start Simulation" to begin
- Watch real-time order book updates
- Monitor performance metrics and trade executions

### 4. Export Results
- Export trade history to CSV
- Export order book snapshots
- Download performance analytics

## CSV Import Format

```csv
trader_id,symbol,side,quantity,price,timestamp
TRADER_001,AAPL,BUY,100,150.25,2025-07-06 10:00:00
TRADER_002,AAPL,SELL,75,150.50,2025-07-06 10:00:15
```

Required columns:
- `trader_id`: Unique trader identifier
- `symbol`: Trading symbol (e.g., AAPL)
- `side`: BUY or SELL
- `quantity`: Number of shares (positive integer)
- `price`: Price per share (positive decimal)
- `timestamp`: Optional timestamp

## Performance Modes

### Standard Mode
- Order generation: Every 100ms
- Suitable for visualization and analysis
- Balanced performance and UI responsiveness

### High-Frequency Mode
- Order generation: Every 50ms
- Batch processing enabled
- Maximum throughput for performance testing
- Optimized for speed over UI responsiveness

## Architecture

### Core Components

#### `models/engine.py` - Trading Engine
- Central order processing and matching system
- Batch processing for high-frequency performance
- Real-time statistics and performance monitoring

#### `models/order.py` - Order Management
- Individual order representation and lifecycle
- Fill tracking and status management
- Thread-safe operations

#### `models/orderbook.py` - Order Book
- Heap-based price level management
- Efficient price-time priority matching
- Real-time market depth calculation

#### `models/trader.py` - Trading Bots
- Simulated traders with AI-like behavior
- Configurable trading parameters
- Portfolio tracking and P&L calculation

#### `utils/csv_importer.py` - Data Import
- CSV validation and processing
- Error handling and reporting
- Symbol and trader extraction

#### `utils/data_export.py` - Data Export
- Multiple export formats
- Trading analytics and statistics
- Performance metrics calculation

## Performance Characteristics

### Typical Performance
- **Throughput**: 500-2,000+ trades per second
- **Latency**: Sub-millisecond order processing
- **Update Rate**: 500ms GUI refresh in HFT mode
- **Memory Usage**: ~100-200MB for 20 traders

### Optimization Features
- Batch order processing
- Efficient data structures
- Minimal GUI blocking
- Real-time performance monitoring

## Configuration Options

### Trading Parameters
- Order frequency: 50ms (HFT) to 500ms (standard)
- Order size range: 5-50 (HFT) or 10-100 (standard)
- Price volatility: 1-2% variation
- Position limits: Configurable per trader

### Performance Tuning
- HFT mode toggle
- Batch processing size
- GUI update frequency
- Trade history limits

## Troubleshooting

### Common Issues

1. **Slow Performance**
   - Enable High-Frequency Mode
   - Reduce number of traders
   - Close other browser tabs

2. **CSV Import Errors**
   - Check required columns are present
   - Verify data types (quantity: integer, price: decimal)
   - Ensure no empty values in required fields

3. **Memory Issues**
   - Reduce trade history limit
   - Clear data periodically
   - Monitor system resources

## Sample Data

The application includes sample trading data for testing:
- Multiple trader scenarios
- Various symbols (AAPL, GOOGL, MSFT, TSLA, AMZN, etc.)
- Realistic order patterns
- Time-based order sequences

## License

This project is provided for educational and demonstration purposes.