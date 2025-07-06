cmake_minimum_required(VERSION 3.16)
project(HFTSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Qt6 setup
qt6_standard_project_setup()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/order.cpp
    src/orderbook.cpp
    src/trader.cpp
    src/engine.cpp
    src/mainwindow.cpp
    src/csvexporter.cpp
)

# Header files
set(HEADERS
    include/order.h
    include/orderbook.h
    include/trader.h
    include/engine.h
    include/mainwindow.h
    include/csvexporter.h
)

# Create executable
qt6_add_executable(hft_simulator ${SOURCES} ${HEADERS})

# Link Qt6 libraries
target_link_libraries(hft_simulator Qt6::Core Qt6::Widgets Qt6::Charts)

# Copy sample data file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sample_trades.csv
               ${CMAKE_CURRENT_BINARY_DIR}/sample_trades.csv COPYONLY)