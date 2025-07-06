#include "orderbook.h"
#include <QMutexLocker>
#include <QDebug>
#include <algorithm>

// OrderBookSide implementation
OrderBookSide::OrderBookSide(bool isBidSide) : isBidSide(isBidSide) {}

void OrderBookSide::addOrder(OrderPtr order) {
    QMutexLocker locker(&mutex);
    
    double price = order->getPrice();
    
    // Add to price level
    if (!priceLevels.contains(price)) {
        priceLevels[price] = PriceLevel(price);
    }
    
    priceLevels[price].orders.enqueue(order);
    priceLevels[price].totalQuantity += order->getQuantity();
    priceLevels[price].orderCount++;
    
    // Add to orders mapping
    orders[order->getOrderId()] = order;
}

bool OrderBookSide::removeOrder(const QString& orderId) {
    QMutexLocker locker(&mutex);
    
    if (!orders.contains(orderId)) {
        return false;
    }
    
    OrderPtr order = orders[orderId];
    double price = order->getPrice();
    
    // Remove from price level
    if (priceLevels.contains(price)) {
        PriceLevel& level = priceLevels[price];
        
        // Find and remove order from queue
        QQueue<OrderPtr> newQueue;
        bool found = false;
        
        while (!level.orders.isEmpty()) {
            OrderPtr currentOrder = level.orders.dequeue();
            if (currentOrder->getOrderId() == orderId) {
                found = true;
                level.totalQuantity -= currentOrder->getQuantity();
                level.orderCount--;
            } else {
                newQueue.enqueue(currentOrder);
            }
        }
        
        level.orders = newQueue;
        
        // Remove empty price level
        if (level.orders.isEmpty()) {
            priceLevels.remove(price);
        }
    }
    
    // Remove from orders mapping
    orders.remove(orderId);
    return true;
}

double OrderBookSide::getBestPrice() const {
    QMutexLocker locker(&mutex);
    
    if (priceLevels.isEmpty()) {
        return 0.0;
    }
    
    if (isBidSide) {
        // For bids, best price is highest
        return priceLevels.lastKey();
    } else {
        // For asks, best price is lowest
        return priceLevels.firstKey();
    }
}

OrderPtr OrderBookSide::getBestOrder() const {
    QMutexLocker locker(&mutex);
    
    double bestPrice = getBestPrice();
    if (bestPrice == 0.0 || !priceLevels.contains(bestPrice)) {
        return nullptr;
    }
    
    const PriceLevel& level = priceLevels[bestPrice];
    return level.orders.isEmpty() ? nullptr : level.orders.head();
}

QVector<OrderPtr> OrderBookSide::getOrdersAtPrice(double price) const {
    QMutexLocker locker(&mutex);
    
    QVector<OrderPtr> result;
    if (priceLevels.contains(price)) {
        const PriceLevel& level = priceLevels[price];
        for (const auto& order : level.orders) {
            result.append(order);
        }
    }
    return result;
}

QVector<PriceLevel> OrderBookSide::getTopLevels(int numLevels) const {
    QMutexLocker locker(&mutex);
    
    QVector<PriceLevel> levels;
    
    if (isBidSide) {
        // For bids, iterate from highest to lowest price
        auto it = priceLevels.end();
        while (it != priceLevels.begin() && levels.size() < numLevels) {
            --it;
            if (it.value().totalQuantity > 0) {
                levels.append(it.value());
            }
        }
    } else {
        // For asks, iterate from lowest to highest price
        auto it = priceLevels.begin();
        while (it != priceLevels.end() && levels.size() < numLevels) {
            if (it.value().totalQuantity > 0) {
                levels.append(it.value());
            }
            ++it;
        }
    }
    
    return levels;
}

int OrderBookSide::getTotalVolume() const {
    QMutexLocker locker(&mutex);
    
    int total = 0;
    for (const auto& order : orders) {
        total += order->getQuantity();
    }
    return total;
}

void OrderBookSide::clear() {
    QMutexLocker locker(&mutex);
    priceLevels.clear();
    orders.clear();
}

// OrderBook implementation
OrderBook::OrderBook(const QString& symbol) 
    : symbol(symbol), bids(true), asks(false) {}

void OrderBook::addOrder(OrderPtr order) {
    if (order->getSymbol() != symbol) {
        qWarning() << "Order symbol" << order->getSymbol() << "doesn't match book symbol" << symbol;
        return;
    }
    
    if (order->getSide() == OrderSide::BUY) {
        bids.addOrder(order);
    } else {
        asks.addOrder(order);
    }
}

bool OrderBook::removeOrder(const QString& orderId, OrderSide side) {
    if (side == OrderSide::BUY) {
        return bids.removeOrder(orderId);
    } else {
        return asks.removeOrder(orderId);
    }
}

OrderPtr OrderBook::getBestBid() const {
    return bids.getBestOrder();
}

OrderPtr OrderBook::getBestAsk() const {
    return asks.getBestOrder();
}

double OrderBook::getBestBidPrice() const {
    return bids.getBestPrice();
}

double OrderBook::getBestAskPrice() const {
    return asks.getBestPrice();
}

double OrderBook::getSpread() const {
    double bestBid = getBestBidPrice();
    double bestAsk = getBestAskPrice();
    
    if (bestBid > 0.0 && bestAsk > 0.0) {
        return bestAsk - bestBid;
    }
    return 0.0;
}

double OrderBook::getMidPrice() const {
    double bestBid = getBestBidPrice();
    double bestAsk = getBestAskPrice();
    
    if (bestBid > 0.0 && bestAsk > 0.0) {
        return (bestBid + bestAsk) / 2.0;
    }
    return 0.0;
}

QPair<QVector<PriceLevel>, QVector<PriceLevel>> OrderBook::getTopLevels(int numLevels) const {
    return qMakePair(bids.getTopLevels(numLevels), asks.getTopLevels(numLevels));
}

void OrderBook::addTrade(const Trade& trade) {
    QMutexLocker locker(&mutex);
    
    tradeHistory.append(trade);
    
    // Keep only recent trades
    if (tradeHistory.size() > MAX_TRADE_HISTORY) {
        tradeHistory.removeFirst();
    }
}

QVector<Trade> OrderBook::getRecentTrades(int count) const {
    QMutexLocker locker(&mutex);
    
    if (count <= 0 || tradeHistory.isEmpty()) {
        return tradeHistory;
    }
    
    int startIndex = qMax(0, tradeHistory.size() - count);
    return tradeHistory.mid(startIndex);
}

int OrderBook::getVolumeAtPrice(double price, OrderSide side) const {
    if (side == OrderSide::BUY) {
        auto orders = bids.getOrdersAtPrice(price);
        int total = 0;
        for (const auto& order : orders) {
            total += order->getQuantity();
        }
        return total;
    } else {
        auto orders = asks.getOrdersAtPrice(price);
        int total = 0;
        for (const auto& order : orders) {
            total += order->getQuantity();
        }
        return total;
    }
}

bool OrderBook::isCrossed() const {
    double bestBid = getBestBidPrice();
    double bestAsk = getBestAskPrice();
    
    if (bestBid > 0.0 && bestAsk > 0.0) {
        return bestBid >= bestAsk;
    }
    return false;
}

void OrderBook::clear() {
    QMutexLocker locker(&mutex);
    bids.clear();
    asks.clear();
    tradeHistory.clear();
}