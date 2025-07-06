#include "trader.h"
#include "engine.h"
#include <QMutexLocker>
#include <QDebug>
#include <QVariant>
#include <cmath>

Trader::Trader(const QString& traderId, double initialCash, 
               const QStringList& symbols, TradingEngine* engine, QObject* parent)
    : QObject(parent)
    , traderId(traderId)
    , initialCash(initialCash)
    , cash(initialCash)
    , symbols(symbols)
    , engine(engine)
    , ordersSent(0)
    , ordersFilled(0)
    , totalVolume(0)
    , minOrderSize(10)
    , maxOrderSize(100)
    , priceVolatility(0.02)  // 2% price variation
    , orderFrequencyMs(50)   // 50ms between orders for HFT speed
    , active(false)
    , tradingTimer(new QTimer(this))
    , random(QRandomGenerator::global())
{
    // Initialize positions and market price cache
    for (const QString& symbol : symbols) {
        positions[symbol] = 0;
        averageCosts[symbol] = 0.0;
        marketPriceCache[symbol] = 100.0; // Starting price
    }
    
    // Connect timer
    connect(tradingTimer, &QTimer::timeout, this, &Trader::generateOrder);
    
    // Set random interval for trading
    tradingTimer->setSingleShot(false);
}

Trader::~Trader() {
    stopTrading();
}

QMap<QString, int> Trader::getPositions() const {
    QMutexLocker locker(&mutex);
    return positions;
}

void Trader::startTrading() {
    QMutexLocker locker(&mutex);
    
    if (!active) {
        active = true;
        // Start with random delay
        int delay = random->bounded(100, orderFrequencyMs);
        tradingTimer->start(delay);
    }
}

void Trader::stopTrading() {
    QMutexLocker locker(&mutex);
    
    if (active) {
        active = false;
        tradingTimer->stop();
    }
}

double Trader::getPortfolioValue() const {
    QMutexLocker locker(&mutex);
    
    double portfolioValue = cash;
    
    for (auto it = positions.begin(); it != positions.end(); ++it) {
        const QString& symbol = it.key();
        int position = it.value();
        
        if (position > 0) {
            double marketPrice = estimateMarketPrice(symbol);
            portfolioValue += position * marketPrice;
        }
    }
    
    return portfolioValue;
}

double Trader::getTotalPnL() const {
    return getPortfolioValue() - initialCash;
}

double Trader::getPositionPnL(const QString& symbol) const {
    QMutexLocker locker(&mutex);
    
    int position = positions.value(symbol, 0);
    if (position == 0) {
        return 0.0;
    }
    
    double marketPrice = estimateMarketPrice(symbol);
    double marketValue = position * marketPrice;
    double costBasis = position * averageCosts.value(symbol, 0.0);
    
    return marketValue - costBasis;
}

QMap<QString, QVariant> Trader::getTradingStats() const {
    QMutexLocker locker(&mutex);
    
    QMap<QString, QVariant> stats;
    stats["trader_id"] = traderId;
    stats["cash"] = cash;
    stats["portfolio_value"] = getPortfolioValue();
    stats["total_pnl"] = getTotalPnL();
    stats["orders_sent"] = ordersSent;
    stats["orders_filled"] = ordersFilled;
    stats["total_volume"] = totalVolume;
    stats["fill_rate"] = ordersFilled / qMax(1, ordersSent);
    
    return stats;
}

void Trader::onOrderFilled(OrderPtr order, int fillQuantity, double fillPrice) {
    QMutexLocker locker(&mutex);
    
    updatePosition(order->getSymbol(), order->getSide(), fillQuantity, fillPrice);
    ordersFilled++;
    totalVolume += fillQuantity;
}

void Trader::generateOrder() {
    if (!active || symbols.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&mutex);
    
    // Choose random symbol
    QString symbol = symbols[random->bounded(symbols.size())];
    
    // Get current market price estimate
    double marketPrice = estimateMarketPrice(symbol);
    
    // Decide order side (buy/sell) with some bias based on position
    OrderSide side = decideOrderSide(symbol);
    
    // Generate order parameters
    int quantity = random->bounded(minOrderSize, maxOrderSize + 1);
    
    // Generate price with some randomness around market price
    double priceVariation = random->generateDouble() * priceVolatility * 2 - priceVolatility;
    double price;
    
    if (side == OrderSide::BUY) {
        // Buyers typically bid below market price
        price = marketPrice * (1 - std::abs(priceVariation));
    } else {
        // Sellers typically ask above market price
        price = marketPrice * (1 + std::abs(priceVariation));
    }
    
    // Round price to 2 decimal places
    price = std::round(price * 100.0) / 100.0;
    
    // Check if we can afford the order (for buy orders)
    if (side == OrderSide::BUY && quantity * price > cash) {
        int affordableQuantity = static_cast<int>(cash / price);
        if (affordableQuantity < minOrderSize) {
            // Schedule next order
            tradingTimer->start(random->bounded(100, orderFrequencyMs));
            return;
        }
        quantity = affordableQuantity;
    }
    
    // Check if we have enough shares to sell
    if (side == OrderSide::SELL && quantity > positions.value(symbol, 0)) {
        int availableShares = positions.value(symbol, 0);
        if (availableShares < minOrderSize) {
            // Schedule next order
            tradingTimer->start(random->bounded(100, orderFrequencyMs));
            return;
        }
        quantity = availableShares;
    }
    
    // Create and submit order
    auto order = std::make_shared<Order>(traderId, symbol, side, quantity, price);
    
    locker.unlock(); // Unlock before calling engine
    
    engine->submitOrder(order);
    
    locker.relock();
    ordersSent++;
    
    // Schedule next order with random delay
    int nextDelay = random->bounded(100, orderFrequencyMs);
    tradingTimer->start(nextDelay);
}

double Trader::estimateMarketPrice(const QString& symbol) const {
    // This method would normally query the engine for recent trades and order book
    // For simplicity, we'll use a basic random walk model
    
    double currentPrice = marketPriceCache.value(symbol, 100.0);
    
    // Random walk with small steps
    double change = (random->generateDouble() - 0.5) * 0.02; // ±1% change
    currentPrice *= (1 + change);
    currentPrice = qMax(1.0, currentPrice); // Minimum price of $1
    
    // Update cache (note: this is not thread-safe, but acceptable for simulation)
    const_cast<Trader*>(this)->marketPriceCache[symbol] = currentPrice;
    
    return currentPrice;
}

OrderSide Trader::decideOrderSide(const QString& symbol) {
    int position = positions.value(symbol, 0);
    
    // If we have a large position, bias toward selling
    if (position > 500) {
        return (random->generateDouble() < 0.7) ? OrderSide::SELL : OrderSide::BUY;
    }
    // If we have no position, bias toward buying
    else if (position == 0) {
        return (random->generateDouble() < 0.7) ? OrderSide::BUY : OrderSide::SELL;
    }
    // Otherwise, random
    else {
        return (random->generateDouble() < 0.5) ? OrderSide::BUY : OrderSide::SELL;
    }
}

void Trader::updatePosition(const QString& symbol, OrderSide side, int quantity, double price) {
    if (side == OrderSide::BUY) {
        // Update cash and position
        double cost = quantity * price;
        cash -= cost;
        
        // Update average cost basis
        int oldPosition = positions.value(symbol, 0);
        double oldCostBasis = averageCosts.value(symbol, 0.0) * oldPosition;
        double newCostBasis = oldCostBasis + cost;
        int newPosition = oldPosition + quantity;
        
        positions[symbol] = newPosition;
        if (newPosition > 0) {
            averageCosts[symbol] = newCostBasis / newPosition;
        }
    } else { // SELL
        // Update cash and position
        double proceeds = quantity * price;
        cash += proceeds;
        positions[symbol] -= quantity;
        
        // If position goes to zero, reset average cost
        if (positions[symbol] == 0) {
            averageCosts[symbol] = 0.0;
        }
    }
}