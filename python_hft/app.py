import streamlit as st
import pandas as pd
import plotly.graph_objects as go
import plotly.express as px
from streamlit_autorefresh import st_autorefresh
from datetime import datetime

from models.engine import TradingEngine
from models.trader import Trader
from utils.data_export import DataExporter
from utils.csv_importer import CSVImporter

# Page configuration
st.set_page_config(page_title="HFT Trading Simulation",
                   page_icon="📈",
                   layout="wide",
                   initial_sidebar_state="expanded")

# Initialize session state
if 'engine' not in st.session_state:
    st.session_state.engine = TradingEngine()
    st.session_state.traders = []
    st.session_state.simulation_running = False
    st.session_state.last_update = datetime.now().timestamp()
    st.session_state.data_exporter = DataExporter()
    st.session_state.csv_importer = CSVImporter()
    st.session_state.custom_symbols = []
    st.session_state.imported_symbols = []

# Auto-refresh every 1000ms if simulation is running
if st.session_state.simulation_running:
    st_autorefresh(interval=1000, limit=None, key="hft_refresh")


@st.cache_data
def get_all_symbols(default, custom, imported):
    return sorted(list(set(default + custom + imported)))


def create_traders(num_traders, symbols, initial_cash, hft_mode=False):
    traders = []
    for i in range(num_traders):
        trader = Trader(trader_id=f"BOT_{i+1:03d}",
                        initial_cash=initial_cash,
                        symbols=symbols,
                        engine=st.session_state.engine)
        if hft_mode:
            trader.order_frequency = 0.05
            trader.min_order_size = 1  # ✅ Lowered from 5 or 10
            trader.max_order_size = 10  # ✅ Lowered to ensure affordability
            trader.price_volatility = 0.01
        traders.append(trader)
    return traders


def start_simulation():
    st.session_state.simulation_running = True


def stop_simulation():
    st.session_state.simulation_running = False
    st.session_state.engine.stop()
    for trader in st.session_state.traders:
        trader.stop_trading()


def create_orderbook_chart(symbol):
    orderbook = st.session_state.engine.get_orderbook(symbol)
    bids, asks = orderbook.get_top_levels(5)
    fig = go.Figure()
    if bids:
        fig.add_trace(
            go.Bar(x=[b.quantity for b in bids],
                   y=[b.price for b in bids],
                   orientation='h',
                   name='Bids',
                   marker_color='green',
                   opacity=0.7))
    if asks:
        fig.add_trace(
            go.Bar(x=[-a.quantity for a in asks],
                   y=[a.price for a in asks],
                   orientation='h',
                   name='Asks',
                   marker_color='red',
                   opacity=0.7))
    fig.update_layout(title=f"Order Book - {symbol}",
                      xaxis_title="Quantity",
                      yaxis_title="Price",
                      height=400,
                      barmode='overlay')
    return fig


def create_pnl_chart():
    pnl_data = [{
        'Trader': t.trader_id,
        'Cash': t.cash,
        'Portfolio Value': t.get_portfolio_value(),
        'Total P&L': t.get_total_pnl()
    } for t in st.session_state.traders]
    if pnl_data:
        df = pd.DataFrame(pnl_data)
        fig = px.bar(df,
                     x='Trader',
                     y='Total P&L',
                     color='Total P&L',
                     title="Trader P&L",
                     color_continuous_scale='RdYlGn')
        fig.update_layout(height=300)
        return fig
    return None


def create_performance_metrics():
    stats = st.session_state.engine.get_performance_stats()
    col1, col2, col3, col4 = st.columns(4)
    col1.metric("Total Trades", stats['total_trades'])
    col2.metric("Trades/Second", f"{stats['trades_per_second']:.2f}")
    col3.metric("Avg Latency (ms)", f"{stats['avg_latency_ms']:.2f}")
    col4.metric("Active Orders", stats['active_orders'])


# UI Start
st.title("🚀 High-Frequency Trading Simulation")
st.markdown("Real-time order book simulation with algorithmic trading bots")

# Sidebar
with st.sidebar:
    st.header("Simulation Controls")
    hft_mode = st.checkbox("🚀 High-Frequency Mode",
                           help="Enable 50ms order intervals")
    if hft_mode:
        st.info("⚡ HFT Mode Active")

    default_symbols = [
        "AAPL", "GOOGL", "MSFT", "TSLA", "AMZN", "NVDA", "META", "NFLX", "AMD",
        "INTC"
    ]
    all_available_symbols = get_all_symbols(default_symbols,
                                            st.session_state.custom_symbols,
                                            st.session_state.imported_symbols)
    symbols = st.multiselect("Trading Symbols",
                             all_available_symbols,
                             default=all_available_symbols[:2])

    custom_symbol = st.text_input("Add Custom Symbol")
    if st.button("Add") and custom_symbol:
        cs = custom_symbol.upper()
        if cs not in all_available_symbols:
            st.session_state.custom_symbols.append(cs)
            st.rerun()
        else:
            st.warning("Symbol already exists")

    num_traders = st.slider("Number of Bots", 1, 20, 5)
    initial_cash = st.number_input("Initial Cash per Bot",
                                   min_value=10000,
                                   max_value=1000000,
                                   value=100000,
                                   step=10000)

    col1, col2 = st.columns(2)
    if col1.button("Start Simulation",
                   disabled=st.session_state.simulation_running):
        if symbols:
            st.session_state.traders = create_traders(num_traders, symbols,
                                                      initial_cash, hft_mode)
            start_simulation()
            st.rerun()
        else:
            st.error("Please select symbols")

    if col2.button("Stop Simulation",
                   disabled=not st.session_state.simulation_running):
        stop_simulation()
        st.success("Simulation stopped")
        st.rerun()

    if st.button("Export Trades to CSV"):
        trades_data = st.session_state.engine.get_all_trades()
        if trades_data:
            csv_data = st.session_state.data_exporter.export_trades_to_csv(
                trades_data)
            st.download_button(
                "Download Trades CSV",
                data=csv_data,
                file_name=
                f"trades_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
                mime="text/csv")
        else:
            st.warning("No trades to export")

    if st.button("Export Order Book Snapshot"):
        orderbook_data = {
            sym: st.session_state.engine.get_orderbook(sym).get_snapshot()
            for sym in symbols
        }
        if any(orderbook_data.values()):
            csv_data = st.session_state.data_exporter.export_orderbook_to_csv(
                orderbook_data)
            st.download_button(
                "Download Order Book CSV",
                data=csv_data,
                file_name=
                f"orderbook_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
                mime="text/csv")
        else:
            st.warning("No order book data")

# Main Content
create_performance_metrics()

if symbols:
    selected_symbol = st.selectbox("📊 Select Symbol", symbols)
    if selected_symbol:
        col1, col2 = st.columns([1, 1])
        with col1:
            fig = create_orderbook_chart(selected_symbol)
            st.plotly_chart(fig, use_container_width=True)
        with col2:
            orderbook = st.session_state.engine.get_orderbook(selected_symbol)
            bids, asks = orderbook.get_top_levels(5)
            rows = []
            for ask in reversed(asks):
                rows.append({
                    'Side': 'ASK',
                    'Price': f"${ask.price:.2f}",
                    'Quantity': ask.quantity,
                    'Total': ask.price * ask.quantity
                })
            if bids and asks:
                rows.append({
                    'Side': '---',
                    'Price': '---',
                    'Quantity': '---',
                    'Total': '---'
                })
            for bid in bids:
                rows.append({
                    'Side': 'BID',
                    'Price': f"${bid.price:.2f}",
                    'Quantity': bid.quantity,
                    'Total': bid.price * bid.quantity
                })
            st.dataframe(pd.DataFrame(rows),
                         use_container_width=True,
                         hide_index=True)

st.subheader("📈 Recent Trades")
recent_trades = st.session_state.engine.get_recent_trades(20)
if recent_trades:
    trades_df = pd.DataFrame([{
        'Time':
        t['timestamp'].strftime('%H:%M:%S.%f')[:-3],
        'Symbol':
        t['symbol'],
        'Side':
        t['side'],
        'Price':
        f"${t['price']:.2f}",
        'Quantity':
        t['quantity'],
        'Buyer':
        t['buyer_id'],
        'Seller':
        t['seller_id']
    } for t in recent_trades])
    st.dataframe(trades_df, use_container_width=True, hide_index=True)
else:
    st.info("No trades yet")

if st.session_state.traders:
    st.subheader("💰 Trader Performance")
    pnl_fig = create_pnl_chart()
    if pnl_fig:
        st.plotly_chart(pnl_fig, use_container_width=True)
    trader_data = [{
        'Trader ID': t.trader_id,
        'Cash': f"${t.cash:,.2f}",
        'Portfolio Value': f"${t.get_portfolio_value():,.2f}",
        'Total P&L': f"${t.get_total_pnl():,.2f}",
        'Orders Sent': t.orders_sent,
        'Orders Filled': t.orders_filled
    } for t in st.session_state.traders]
    st.dataframe(pd.DataFrame(trader_data),
                 use_container_width=True,
                 hide_index=True)
    # Live metrics per trader
    st.subheader("📊 Live Trader Metrics")
    for trader in st.session_state.traders:
        col1, col2 = st.columns(2)
        with col1:
            st.metric(f"{trader.trader_id} Cash", f"${trader.cash:,.2f}")
        with col2:
            st.metric(f"{trader.trader_id} Portfolio",
                      f"${trader.get_portfolio_value():,.2f}")

status_col1, status_col2 = st.columns([4, 4])

status_col1.info("Simulation auto-refreshes every second.")

if st.session_state.simulation_running:
    status_col2.success("🟢 Running")
else:
    status_col2.error("🔴 Stopped")
