#!/bin/bash

# Python HFT Trading Simulation - Installation Script

echo "🐍 Installing Python HFT Trading Simulation"
echo "==========================================="

# Check Python version
python_version=$(python3 --version 2>&1 | cut -d" " -f2)
echo "Python version: $python_version"

# Install requirements
echo "Installing Python dependencies..."
pip3 install -r requirements.txt

echo ""
echo "✅ Installation complete!"
echo ""
echo "To run the application:"
echo "  streamlit run app.py --server.port 5000"
echo ""
echo "Then open your browser to: http://localhost:5000"
echo ""
echo "Features:"
echo "  🚀 High-Frequency Mode for maximum performance"
echo "  📊 Real-time order book visualization"
echo "  💰 Trader P&L tracking"
echo "  📈 Performance analytics"
echo "  📁 CSV import/export capabilities"
echo ""