# High-Frequency Trading (HFT) Simulation Suite

A comprehensive suite of high-frequency trading simulators featuring both Python web application and C++ desktop application implementations. Built for educational purposes and performance analysis of algorithmic trading systems.

## Overview

This project provides two complete HFT simulation implementations:

- **🐍 Python Web Application**: Real-time trading simulation with Streamlit interface
- **⚡ C++ Desktop Application**: High-performance Qt-based trading simulator

Both implementations feature realistic order matching engines, multi-threaded trading bots, performance analytics, and comprehensive data import/export capabilities.

## Project Structure

```
hft-trading-simulation/
├── README.md                    # This file - main project overview
├── python_hft/                 # Python web application
│   ├── README.md               # Python-specific documentation
│   ├── app.py                  # Main Streamlit application
│   ├── performance_benchmark.py # Performance testing tool
│   ├── sample_trades.csv       # Sample data for testing
│   ├── models/                 # Core trading engine
│   │   ├── engine.py          # Order matching engine
│   │   ├── order.py           # Order management
│   │   ├── orderbook.py       # Order book implementation
│   │   └── trader.py          # Trading bot logic
│   ├── utils/                  # Utility modules
│   │   ├── csv_importer.py    # CSV import functionality
│   │   └── data_export.py     # Data export utilities
│   └── .streamlit/            # Streamlit configuration
│       └── config.toml        # Server configuration
└── cpp_hft/                   # C++ desktop application
    ├── README.md              # C++ specific documentation
    ├── CMakeLists.txt         # Build configuration
    ├── sample_trades.csv      # Sample data for testing
    ├── include/               # Header files
    │   ├── engine.h          # Trading engine interface
    │   ├── order.h           # Order class definition
    │   ├── orderbook.h       # Order book interface
    │   ├── trader.h          # Trader class interface
    │   ├── mainwindow.h      # GUI main window
    │   └── csvexporter.h     # CSV export utilities
    └── src/                   # Source implementations
        ├── main.cpp          # Application entry point
        ├── engine.cpp        # Trading engine implementation
        ├── order.cpp         # Order class implementation
        ├── orderbook.cpp     # Order book implementation
        ├── trader.cpp        # Trader implementation
        ├── mainwindow.cpp    # GUI implementation
        └── csvexporter.cpp   # CSV utilities implementation
```

## Quick Start

### Python Web Application
```bash
cd python_hft
pip install streamlit pandas numpy plotly
streamlit run app.py --server.port 5000
```
Access at: http://localhost:5000

### C++ Desktop Application
```bash
cd cpp_hft
mkdir build && cd build
cmake ..
make -j$(nproc)
./hft_simulator
```

## Features Comparison

| Feature | Python Web App | C++ Desktop App |
|---------|----------------|-----------------|
| **Performance** | 500-2,000 TPS | 1,000-10,000+ TPS |
| **Interface** | Web Browser | Native Desktop GUI |
| **Real-time Updates** | 500ms refresh | Instant GUI updates |
| **Order Book Display** | Interactive charts | Color-coded tables |
| **CSV Import/Export** | ✅ Full support | ✅ Full support |
| **Multi-threading** | ✅ Thread-safe | ✅ Optimized threading |
| **HFT Mode** | ✅ 50ms orders | ✅ Sub-millisecond |
| **Platform** | Cross-platform | Cross-platform |
| **Dependencies** | Python, Streamlit | Qt 6, C++17 |

## Core Architecture

Both implementations share the same fundamental architecture:

### Trading Engine
- **Order Matching**: Price-time priority algorithm
- **Multi-threading**: Concurrent order processing
- **Performance Monitoring**: Real-time statistics
- **Thread Safety**: Mutex-protected operations

### Order Management
- **Order Lifecycle**: Pending → Partially Filled → Filled/Cancelled
- **Fill Tracking**: Detailed execution history
- **Price-Time Priority**: Fair matching algorithm

### Trading Bots
- **AI-like Behavior**: Realistic trading patterns
- **Portfolio Management**: Position and P&L tracking
- **Configurable Parameters**: Order size, frequency, volatility
- **Risk Management**: Position limits and cash constraints

### Data Management
- **CSV Import**: Order injection from external files
- **Export Capabilities**: Trades, order books, performance metrics
- **Sample Data**: Realistic test scenarios included

## Performance Characteristics

### Python Implementation
- **Throughput**: 500-2,000 trades per second
- **Latency**: 1-5ms order processing
- **Memory**: ~100-200MB for 20 traders
- **Best For**: Analysis, visualization, web deployment

### C++ Implementation  
- **Throughput**: 1,000-10,000+ trades per second
- **Latency**: Sub-millisecond processing
- **Memory**: ~50-100MB for 20 traders
- **Best For**: Performance testing, desktop deployment

## Use Cases

### Educational
- **Algorithm Development**: Test trading strategies
- **Market Microstructure**: Study order book dynamics
- **Performance Analysis**: Compare algorithm efficiency
- **Risk Management**: Analyze portfolio behavior

### Research
- **Market Simulation**: Model real trading scenarios
- **Latency Analysis**: Measure system performance
- **Capacity Planning**: Test system limits
- **Algorithm Backtesting**: Historical strategy evaluation

### Development
- **Prototype Testing**: Rapid algorithm development
- **Performance Benchmarking**: System optimization
- **Integration Testing**: External system validation
- **Load Testing**: Stress test trading systems

## Data Format

Both applications support standardized CSV import/export:

```csv
trader_id,symbol,side,quantity,price,timestamp
TRADER_001,AAPL,BUY,100,150.25,2025-07-06 10:00:00
TRADER_002,AAPL,SELL,75,150.50,2025-07-06 10:00:15
```

## Requirements

### Python Version
- Python 3.11+
- Streamlit
- Pandas, NumPy, Plotly

### C++ Version
- Qt 6.0+
- CMake 3.16+
- C++17 compiler (GCC 8+, Clang 8+, MSVC 2019+)

### System Requirements
- **Memory**: 2GB+ RAM recommended
- **CPU**: Multi-core processor for optimal performance
- **Storage**: 100MB+ free space
- **Network**: Internet connection for Python web interface

## Getting Started

1. **Choose Your Implementation**:
   - Python: For web-based analysis and visualization
   - C++: For maximum performance and desktop deployment

2. **Install Dependencies**:
   - Follow the README in your chosen directory
   - Use provided installation scripts

3. **Run Sample Simulation**:
   - Load included sample data
   - Configure trading parameters
   - Start simulation and observe results

4. **Import Your Data**:
   - Prepare CSV files in required format
   - Use import functionality to inject orders
   - Export results for analysis

## Contributing

This project is designed for educational purposes. Key areas for enhancement:

- **Additional Trading Strategies**: Implement market making, arbitrage
- **Enhanced Visualizations**: Real-time charts and analytics
- **Performance Optimizations**: Further latency reductions
- **Extended Data Formats**: Support for more market data types

## License

This project is provided for educational and demonstration purposes. See individual README files for specific implementation details.

## Support

For implementation-specific questions:
- **Python**: See `python_hft/README.md`
- **C++**: See `cpp_hft/README.md`

For general questions about high-frequency trading concepts, refer to the comprehensive documentation in each implementation directory.