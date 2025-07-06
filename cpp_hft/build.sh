#!/bin/bash

# C++ HFT Trading Simulation - Build Script

echo "⚡ Building C++ HFT Trading Simulation"
echo "======================================"

# Check dependencies
echo "Checking dependencies..."

# Check for Qt6
if ! pkg-config --exists Qt6Core; then
    echo "❌ Qt6 not found. Please install Qt6 development packages."
    echo "   Ubuntu/Debian: sudo apt install qt6-base-dev qt6-charts-dev"
    echo "   macOS: brew install qt6"
    exit 1
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Please install CMake."
    exit 1
fi

# Check for compiler
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "❌ C++ compiler not found. Please install GCC or Clang."
    exit 1
fi

echo "✅ All dependencies found"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring build..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building application..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "To run the application:"
    echo "  ./hft_simulator"
    echo ""
    echo "Features:"
    echo "  🚀 Sub-millisecond order processing"
    echo "  🖥️  Professional Qt GUI with dark theme"
    echo "  📊 Real-time order book display"
    echo "  💰 Individual trader P&L tracking"
    echo "  📈 Live performance metrics"
    echo "  📁 CSV import/export capabilities"
    echo ""
else
    echo "❌ Build failed. Please check error messages above."
    exit 1
fi