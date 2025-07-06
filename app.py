import streamlit as st
import pandas as pd
import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots
import time
import threading
from datetime import datetime
import io

from models.engine import TradingEngine
from models.trader import Trader
from utils.data_export import DataExporter

# Page configuration
st.set_page_config(
    page_title="HFT Trading Simulation",
    page_icon="📈",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Initialize session state
if 'engine' not in st.session_state:
    st.session_state.engine = TradingEngine()
    st.session_state.traders = []
    st.session_state.simulation_running = False
    st.session_state.last_update = time.time()
    st.session_state.data_exporter = DataExporter()

def create_traders(num_traders, symbols, initial_cash):
    """Create trading bots with specified parameters"""
    traders = []
    for i in range(num_traders):
        trader = Trader(
            trader_id=f"BOT_{i+1:03d}",
            initial_cash=initial_cash,
            symbols=symbols,
            engine=st.session_state.engine
        )
        traders.append(trader)
    return traders

def start_simulation():
    """Start the trading simulation"""
    st.session_state.simulation_running = True
    
    # Start trader threads
    for trader in st.session_state.traders:
        trader.start_trading()
    
    # Start engine
    st.session_state.engine.start()

def stop_simulation():
    """Stop the trading simulation"""
    st.session_state.simulation_running = False
    
    # Stop engine
    st.session_state.engine.stop()
    
    # Stop traders
    for trader in st.session_state.traders:
        trader.stop_trading()

def create_orderbook_chart(symbol):
    """Create order book visualization"""
    orderbook = st.session_state.engine.get_orderbook(symbol)
    bids, asks = orderbook.get_top_levels(5)
    
    fig = go.Figure()
    
    if bids:
        bid_prices = [bid.price for bid in bids]
        bid_quantities = [bid.quantity for bid in bids]
        
        fig.add_trace(go.Bar(
            x=bid_quantities,
            y=bid_prices,
            orientation='h',
            name='Bids',
            marker_color='green',
            opacity=0.7
        ))
    
    if asks:
        ask_prices = [ask.price for ask in asks]
        ask_quantities = [-ask.quantity for ask in asks]  # Negative for left side
        
        fig.add_trace(go.Bar(
            x=ask_quantities,
            y=ask_prices,
            orientation='h',
            name='Asks',
            marker_color='red',
            opacity=0.7
        ))
    
    fig.update_layout(
        title=f"Order Book - {symbol}",
        xaxis_title="Quantity",
        yaxis_title="Price",
        height=400,
        barmode='overlay'
    )
    
    return fig

def create_pnl_chart():
    """Create P&L chart for all traders"""
    pnl_data = []
    for trader in st.session_state.traders:
        pnl_data.append({
            'Trader': trader.trader_id,
            'Cash': trader.cash,
            'Portfolio Value': trader.get_portfolio_value(),
            'Total P&L': trader.get_total_pnl()
        })
    
    if pnl_data:
        df = pd.DataFrame(pnl_data)
        fig = px.bar(df, x='Trader', y='Total P&L', 
                    title="Trader P&L", 
                    color='Total P&L',
                    color_continuous_scale='RdYlGn')
        fig.update_layout(height=300)
        return fig
    return None

def create_performance_metrics():
    """Create performance metrics display"""
    stats = st.session_state.engine.get_performance_stats()
    
    col1, col2, col3, col4 = st.columns(4)
    
    with col1:
        st.metric("Total Trades", stats['total_trades'])
    
    with col2:
        st.metric("Trades/Second", f"{stats['trades_per_second']:.2f}")
    
    with col3:
        st.metric("Avg Latency (ms)", f"{stats['avg_latency_ms']:.2f}")
    
    with col4:
        st.metric("Active Orders", stats['active_orders'])

# Main UI
st.title("🚀 High-Frequency Trading Simulation")
st.markdown("Real-time order book simulation with algorithmic trading bots")

# Sidebar controls
with st.sidebar:
    st.header("Simulation Controls")
    
    # Configuration
    st.subheader("Configuration")
    symbols = st.multiselect(
        "Trading Symbols", 
        ["AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"],
        default=["AAPL", "GOOGL"]
    )
    
    num_traders = st.slider("Number of Trading Bots", 1, 20, 5)
    initial_cash = st.number_input("Initial Cash per Bot", 10000, 1000000, 100000)
    
    # Control buttons
    col1, col2 = st.columns(2)
    
    with col1:
        if st.button("Start Simulation", disabled=st.session_state.simulation_running):
            if symbols:
                st.session_state.traders = create_traders(num_traders, symbols, initial_cash)
                start_simulation()
                st.success("Simulation started!")
                st.rerun()
            else:
                st.error("Please select at least one symbol")
    
    with col2:
        if st.button("Stop Simulation", disabled=not st.session_state.simulation_running):
            stop_simulation()
            st.success("Simulation stopped!")
            st.rerun()
    
    # Export controls
    st.subheader("Data Export")
    
    if st.button("Export Trades to CSV"):
        trades_data = st.session_state.engine.get_all_trades()
        if trades_data:
            csv_data = st.session_state.data_exporter.export_trades_to_csv(trades_data)
            st.download_button(
                label="Download Trades CSV",
                data=csv_data,
                file_name=f"trades_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
                mime="text/csv"
            )
        else:
            st.warning("No trades to export")
    
    if st.button("Export Order Book Snapshot"):
        orderbook_data = {}
        for symbol in symbols:
            orderbook = st.session_state.engine.get_orderbook(symbol)
            orderbook_data[symbol] = orderbook.get_snapshot()
        
        if any(orderbook_data.values()):
            csv_data = st.session_state.data_exporter.export_orderbook_to_csv(orderbook_data)
            st.download_button(
                label="Download Order Book CSV",
                data=csv_data,
                file_name=f"orderbook_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
                mime="text/csv"
            )
        else:
            st.warning("No order book data to export")

# Main content area
if st.session_state.simulation_running:
    # Auto-refresh every second
    time.sleep(1)
    st.rerun()

# Performance metrics
create_performance_metrics()

# Order books and charts
if symbols:
    # Order book tabs
    tabs = st.tabs([f"📊 {symbol}" for symbol in symbols])
    
    for i, symbol in enumerate(symbols):
        with tabs[i]:
            col1, col2 = st.columns([1, 1])
            
            with col1:
                # Order book chart
                fig = create_orderbook_chart(symbol)
                st.plotly_chart(fig, use_container_width=True)
            
            with col2:
                # Order book table
                orderbook = st.session_state.engine.get_orderbook(symbol)
                bids, asks = orderbook.get_top_levels(5)
                
                st.subheader("Order Book")
                
                # Create order book table
                orderbook_data = []
                
                # Add asks (reversed for proper display)
                for ask in reversed(asks):
                    orderbook_data.append({
                        'Side': 'ASK',
                        'Price': f"${ask.price:.2f}",
                        'Quantity': ask.quantity,
                        'Total': ask.price * ask.quantity
                    })
                
                # Add separator
                if asks and bids:
                    orderbook_data.append({
                        'Side': '---',
                        'Price': '---',
                        'Quantity': '---',
                        'Total': '---'
                    })
                
                # Add bids
                for bid in bids:
                    orderbook_data.append({
                        'Side': 'BID',
                        'Price': f"${bid.price:.2f}",
                        'Quantity': bid.quantity,
                        'Total': bid.price * bid.quantity
                    })
                
                if orderbook_data:
                    df = pd.DataFrame(orderbook_data)
                    st.dataframe(df, use_container_width=True, hide_index=True)
                else:
                    st.info("No orders in the book")

# Recent trades
st.subheader("📈 Recent Trades")
recent_trades = st.session_state.engine.get_recent_trades(20)
if recent_trades:
    trades_df = pd.DataFrame([
        {
            'Time': trade['timestamp'].strftime('%H:%M:%S.%f')[:-3],
            'Symbol': trade['symbol'],
            'Side': trade['side'],
            'Price': f"${trade['price']:.2f}",
            'Quantity': trade['quantity'],
            'Buyer': trade['buyer_id'],
            'Seller': trade['seller_id']
        }
        for trade in recent_trades
    ])
    st.dataframe(trades_df, use_container_width=True, hide_index=True)
else:
    st.info("No trades executed yet")

# Trader P&L
if st.session_state.traders:
    st.subheader("💰 Trader Performance")
    
    # P&L chart
    pnl_fig = create_pnl_chart()
    if pnl_fig:
        st.plotly_chart(pnl_fig, use_container_width=True)
    
    # Trader details table
    trader_data = []
    for trader in st.session_state.traders:
        trader_data.append({
            'Trader ID': trader.trader_id,
            'Cash': f"${trader.cash:,.2f}",
            'Portfolio Value': f"${trader.get_portfolio_value():,.2f}",
            'Total P&L': f"${trader.get_total_pnl():,.2f}",
            'Orders Sent': trader.orders_sent,
            'Orders Filled': trader.orders_filled
        })
    
    if trader_data:
        traders_df = pd.DataFrame(trader_data)
        st.dataframe(traders_df, use_container_width=True, hide_index=True)

# Status indicator
status_col1, status_col2 = st.columns([1, 4])
with status_col1:
    if st.session_state.simulation_running:
        st.success("🟢 Running")
    else:
        st.error("🔴 Stopped")

with status_col2:
    if st.session_state.simulation_running:
        st.info("Simulation is running. Data updates automatically every second.")
    else:
        st.info("Configure settings and click 'Start Simulation' to begin trading.")
