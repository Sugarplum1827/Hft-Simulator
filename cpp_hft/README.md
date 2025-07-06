# HFT Trading Simulation - C++ Desktop Application

A comprehensive high-frequency trading simulation built with Qt 6 and C++17, featuring real-time order matching, multi-threaded trading bots, and a modern GUI interface.

## Features

### Core Trading Engine
- **Real-time Order Matching**: Price-time priority matching algorithm
- **Multi-threaded Architecture**: Separate threads for order processing and trading bots
- **High-Performance Processing**: Optimized for low-latency order execution
- **Thread-safe Operations**: Mutex-protected data structures for concurrent access

### Trading Simulation
- **Configurable Trading Bots**: Multiple bots with customizable parameters
- **Multiple Asset Support**: Trade on various symbols simultaneously
- **Realistic Market Dynamics**: Market-making and price discovery simulation
- **Portfolio Management**: Real-time P&L tracking and position management

### User Interface
- **Modern Qt GUI**: Professional trading interface with dark theme
- **Real-time Order Book**: Live display of top 5 bids and asks
- **Trade Execution Log**: Real-time trade history with filtering
- **Performance Metrics**: Trades per second, latency, and throughput monitoring
- **Trader Analytics**: Individual bot performance and P&L tracking

### Data Management
- **CSV Import/Export**: Import orders and export trade data
- **Order Book Snapshots**: Export current market state
- **Performance Reports**: Detailed analytics export
- **Sample Data**: Included sample CSV files for testing

## Requirements

### System Dependencies
- Qt 6.0 or higher
- CMake 3.16+
- C++17 compatible compiler (GCC 8+, Clang 8+, MSVC 2019+)
- pkg-config

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config qt6-base-dev qt6-charts-dev
```

### macOS
```bash
brew install qt6 cmake pkg-config
```

### Windows
Download and install:
- Qt 6 from https://www.qt.io/download
- CMake from https://cmake.org/download/
- Visual Studio 2019 or newer

## Build Instructions

### 1. Clone and Navigate
```bash
cd cpp_hft
```

### 2. Create Build Directory
```bash
mkdir build
cd build
```

### 3. Configure with CMake
```bash
# Linux/macOS
cmake ..

# Windows with Visual Studio
cmake .. -G "Visual Studio 16 2019"
```

### 4. Build the Application
```bash
# Linux/macOS
make -j$(nproc)

# Windows
cmake --build . --config Release
```

### 5. Run the Application
```bash
# Linux/macOS
./hft_simulator

# Windows
Release/hft_simulator.exe
```

## Quick Start Guide

### 1. Configure Simulation
- Set number of trading bots (1-50)
- Set initial cash per bot
- Select trading symbols (default: AAPL, GOOGL, MSFT, TSLA, AMZN)
- Add custom symbols if needed

### 2. Start Trading
- Click "Start Simulation" to begin
- Bots will automatically start generating orders
- Watch real-time order book updates and trade executions

### 3. Monitor Performance
- View live performance metrics (trades/sec, latency)
- Track individual trader P&L
- Monitor order book depth and spreads

### 4. Import/Export Data
- Import orders from CSV files
- Export trade history and order book snapshots
- Use provided sample CSV for testing

## CSV Format

### Order Import Format
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
- `timestamp`: Optional timestamp (YYYY-MM-DD HH:MM:SS format)

## Architecture

### Core Components

#### Order (`src/order.cpp`)
- Represents individual trading orders
- Tracks order status, fills, and execution history
- Thread-safe fill and cancel operations

#### OrderBook (`src/orderbook.cpp`)
- Maintains bid/ask price levels using priority queues
- Efficient price-time priority matching
- Real-time market depth calculation

#### TradingEngine (`src/engine.cpp`)
- Central order processing and matching system
- Multi-threaded order queue processing
- Performance monitoring and statistics

#### Trader (`src/trader.cpp`)
- Simulated trading bot with AI-like behavior
- Configurable trading parameters and strategies
- Portfolio tracking and risk management

#### MainWindow (`src/mainwindow.cpp`)
- Qt-based GUI with professional trading interface
- Real-time data visualization
- User controls and configuration options

### Threading Model
- **Main Thread**: GUI updates and user interaction
- **Engine Thread**: Order processing and matching
- **Trader Threads**: Individual bot trading logic
- **Timer Threads**: Periodic updates and statistics

## Configuration Options

### Trading Parameters
- **Order Frequency**: Time between orders (milliseconds)
- **Order Size Range**: Min/max order quantities
- **Price Volatility**: Random price variation percentage
- **Position Limits**: Maximum position size per symbol

### Performance Tuning
- **Processing Interval**: Engine processing frequency (1ms default)
- **Update Frequency**: GUI refresh rate (1000ms default)
- **Trade History Limit**: Maximum stored trades (10,000 default)

## Debugging

### Enable Debug Output
```bash
# Set Qt logging environment variable
export QT_LOGGING_RULES="*.debug=true"
./hft_simulator
```

### Common Issues

1. **Qt Not Found**
   ```bash
   export Qt6_DIR=/path/to/qt6/lib/cmake/Qt6
   ```

2. **Missing Charts Module**
   ```bash
   sudo apt install qt6-charts-dev  # Ubuntu/Debian
   brew install qt6                 # macOS
   ```

3. **Compilation Errors**
   - Ensure C++17 compiler support
   - Check Qt version compatibility
   - Verify all dependencies are installed

## Performance Characteristics

### Typical Performance
- **Throughput**: 1,000-10,000 trades per second
- **Latency**: Sub-millisecond order processing
- **Memory Usage**: ~50-100MB for 20 traders
- **CPU Usage**: 10-30% on modern systems

### Optimization Tips
- Reduce GUI update frequency for higher throughput
- Adjust number of traders based on system capabilities
- Use Release build for production performance

## License

This project is provided as-is for educational and demonstration purposes.

## Support

For build issues or questions:
1. Check Qt installation and version compatibility
2. Verify all system dependencies are installed
3. Ensure C++17 compiler support
4. Review CMake configuration output for errors

## Sample Data

The included `sample_trades.csv` contains example order data for testing the import functionality. Use this file to understand the required CSV format and test the application's import capabilities.