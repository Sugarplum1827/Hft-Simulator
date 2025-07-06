# HFT Trading Simulation

## Overview

This is a high-frequency trading (HFT) simulation platform built with Streamlit that provides real-time order matching and execution capabilities. The system simulates multiple trading bots operating on various financial instruments with full order book management and trade execution tracking.

## System Architecture

The application follows a modular architecture with clear separation of concerns:

**Frontend Layer**: Streamlit-based web interface providing real-time visualization and control
**Trading Engine**: Core matching engine handling order processing and execution
**Order Management**: Complete order book implementation with price-time priority matching
**Bot Framework**: Configurable trading bots with various strategies
**Data Layer**: In-memory data structures with export capabilities

## Key Components

### Trading Engine (`models/engine.py`)
- **Purpose**: Central order matching and execution system
- **Features**: 
  - Multi-threaded order processing with queue-based architecture
  - Real-time latency measurements and performance metrics
  - Thread-safe order book management
  - Trade history tracking with configurable limits (10,000 trades)
- **Design Choice**: Used threading with queue-based processing to handle high-frequency order flow while maintaining thread safety

### Order Management (`models/order.py`, `models/orderbook.py`)
- **Purpose**: Complete order lifecycle management
- **Features**:
  - Full order status tracking (PENDING, PARTIALLY_FILLED, FILLED, CANCELLED)
  - Price-time priority matching algorithm
  - Separate bid/ask sides with heap-based price level management
- **Design Choice**: Implemented heap-based priority queues for efficient price level management, essential for HFT performance

### Trading Bots (`models/trader.py`)
- **Purpose**: Simulated market participants
- **Features**:
  - Portfolio tracking with position management
  - Configurable trading parameters (order size, frequency, volatility)
  - Multi-symbol trading capability
  - Real-time P&L calculation
- **Design Choice**: Each trader runs in separate thread to simulate realistic concurrent trading behavior

### Data Export (`utils/data_export.py`)
- **Purpose**: Trade data export and analysis
- **Features**: 
  - CSV export functionality
  - In-memory data processing
  - Structured trade data format
- **Design Choice**: Used in-memory processing for real-time export capabilities without database dependencies

## Data Flow

1. **Order Generation**: Trading bots generate orders based on configured parameters
2. **Order Submission**: Orders are queued in the trading engine's order queue
3. **Order Processing**: Engine processes orders through matching algorithm
4. **Execution**: Matched orders generate trades and update portfolios
5. **Visualization**: Streamlit frontend displays real-time updates via session state
6. **Export**: Trade data can be exported for analysis

## External Dependencies

- **Streamlit**: Web interface framework
- **Plotly**: Interactive charting and visualization
- **Pandas**: Data manipulation and analysis
- **NumPy**: Numerical computing for trading calculations
- **Threading**: Built-in Python concurrency for multi-bot simulation

## Deployment Strategy

The application is designed for single-instance deployment using Streamlit's built-in server. All data is maintained in-memory using session state, making it suitable for development and demonstration environments.

**Scaling Considerations**: 
- Current architecture supports moderate trading volumes
- Memory usage scales with number of traders and trade history
- For production use, would require persistent storage and distributed architecture

## User Preferences

Preferred communication style: Simple, everyday language.

## Changelog

Changelog:
- July 06, 2025. Initial setup