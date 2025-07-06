#include "engine.h"
#include <QMutexLocker>
#include <QDebug>
#include <QUuid>
#include <algorithm>

TradingEngine::TradingEngine(QObject* parent)
    : QObject(parent)
    , totalTrades(0)
    , totalVolume(0)
    , running(false)
    , processingTimer(new QTimer(this))
    , statsTimer(new QTimer(this))
    , ordersPerSecond(0)
    , ordersProcessedSinceLastUpdate(0)
{
    // Connect timers
    connect(processingTimer, &QTimer::timeout, this, &TradingEngine::processOrders);
    connect(statsTimer, &QTimer::timeout, this, &TradingEngine::updateStats);
    
    // Set timer intervals
    processingTimer->setSingleShot(false);
    processingTimer->setInterval(1); // 1ms for high frequency processing
    
    statsTimer->setSingleShot(false);
    statsTimer->setInterval(1000); // 1 second for stats updates
}

TradingEngine::~TradingEngine() {
    stop();
}

void TradingEngine::start() {
    if (!running) {
        running = true;
        startTime.start();
        lastStatsUpdate.start();
        
        processingTimer->start();
        statsTimer->start();
        
        qDebug() << "Trading engine started";
    }
}

void TradingEngine::stop() {
    if (running) {
        running = false;
        processingTimer->stop();
        statsTimer->stop();
        
        qDebug() << "Trading engine stopped";
    }
}

void TradingEngine::registerTrader(Trader* trader) {
    if (trader) {
        traders[trader->getTraderId()] = trader;
        
        // Connect trader signals
        connect(this, &TradingEngine::orderFilled, 
                trader, &Trader::onOrderFilled);
    }
}

std::shared_ptr<OrderBook> TradingEngine::getOrderBook(const QString& symbol) {
    if (!orderBooks.contains(symbol)) {
        orderBooks[symbol] = std::make_shared<OrderBook>(symbol);
    }
    return orderBooks[symbol];
}

void TradingEngine::submitOrder(OrderPtr order) {
    QMutexLocker locker(&orderQueueMutex);
    orderQueue.enqueue(order);
}

bool TradingEngine::cancelOrder(const QString& orderId) {
    QMutexLocker locker(&orderQueueMutex);
    
    if (activeOrders.contains(orderId)) {
        OrderPtr order = activeOrders[orderId];
        order->cancel();
        
        // Remove from order book
        auto orderBook = getOrderBook(order->getSymbol());
        orderBook->removeOrder(orderId, order->getSide());
        
        // Remove from active orders
        activeOrders.remove(orderId);
        return true;
    }
    return false;
}

QVector<Trade> TradingEngine::getRecentTrades(int count) const {
    QMutexLocker locker(&tradeHistoryMutex);
    
    if (count <= 0 || tradeHistory.isEmpty()) {
        return tradeHistory;
    }
    
    int startIndex = qMax(0, tradeHistory.size() - count);
    return tradeHistory.mid(startIndex);
}

QVector<Trade> TradingEngine::getRecentTradesForSymbol(const QString& symbol, int count) const {
    QMutexLocker locker(&tradeHistoryMutex);
    
    QVector<Trade> symbolTrades;
    for (const auto& trade : tradeHistory) {
        if (trade.symbol == symbol) {
            symbolTrades.append(trade);
        }
    }
    
    if (count <= 0 || symbolTrades.isEmpty()) {
        return symbolTrades;
    }
    
    int startIndex = qMax(0, symbolTrades.size() - count);
    return symbolTrades.mid(startIndex);
}

QVector<Trade> TradingEngine::getAllTrades() const {
    QMutexLocker locker(&tradeHistoryMutex);
    return tradeHistory;
}

QMap<QString, QVariant> TradingEngine::getPerformanceStats() const {
    QMutexLocker locker(&statsMutex);
    
    QMap<QString, QVariant> stats;
    
    qint64 runtimeMs = startTime.elapsed();
    double runtimeSeconds = runtimeMs / 1000.0;
    
    // Calculate trades per second
    double tradesPerSecond = totalTrades / qMax(1.0, runtimeSeconds);
    
    // Calculate average latency
    double avgLatencyMs = 0.0;
    if (!latencyMeasurements.isEmpty()) {
        qint64 totalLatency = 0;
        for (qint64 latency : latencyMeasurements) {
            totalLatency += latency;
        }
        avgLatencyMs = totalLatency / static_cast<double>(latencyMeasurements.size());
    }
    
    // Count active orders
    int activeOrdersCount = activeOrders.size();
    
    stats["total_trades"] = totalTrades;
    stats["total_volume"] = totalVolume;
    stats["trades_per_second"] = tradesPerSecond;
    stats["orders_per_second"] = ordersPerSecond;
    stats["avg_latency_ms"] = avgLatencyMs;
    stats["active_orders"] = activeOrdersCount;
    stats["runtime_seconds"] = runtimeSeconds;
    stats["symbols_active"] = orderBooks.size();
    
    return stats;
}

QMap<QString, QVariant> TradingEngine::getMarketSummary() const {
    QMap<QString, QVariant> summary;
    
    for (auto it = orderBooks.begin(); it != orderBooks.end(); ++it) {
        const QString& symbol = it.key();
        auto orderBook = it.value();
        
        auto recentTrades = orderBook->getRecentTrades(5);
        
        // Calculate volume-weighted average price (VWAP) for recent trades
        double vwap = 0.0;
        if (!recentTrades.isEmpty()) {
            double totalValue = 0.0;
            int totalVolume = 0;
            
            for (const auto& trade : recentTrades) {
                totalValue += trade.price * trade.quantity;
                totalVolume += trade.quantity;
            }
            
            if (totalVolume > 0) {
                vwap = totalValue / totalVolume;
            }
        }
        
        QMap<QString, QVariant> symbolSummary;
        symbolSummary["best_bid"] = orderBook->getBestBidPrice();
        symbolSummary["best_ask"] = orderBook->getBestAskPrice();
        symbolSummary["spread"] = orderBook->getSpread();
        symbolSummary["mid_price"] = orderBook->getMidPrice();
        symbolSummary["vwap"] = vwap;
        symbolSummary["trade_count"] = recentTrades.size();
        
        summary[symbol] = symbolSummary;
    }
    
    return summary;
}

QVector<OrderPtr> TradingEngine::getTraderOrders(const QString& traderId) const {
    QMutexLocker locker(&orderQueueMutex);
    
    QVector<OrderPtr> traderOrders;
    for (const auto& order : activeOrders) {
        if (order->getTraderId() == traderId) {
            traderOrders.append(order);
        }
    }
    return traderOrders;
}

QMap<QString, QVariant> TradingEngine::getSymbolStatistics(const QString& symbol) const {
    QMap<QString, QVariant> stats;
    
    if (!orderBooks.contains(symbol)) {
        return stats;
    }
    
    auto orderBook = orderBooks[symbol];
    auto recentTrades = getRecentTradesForSymbol(symbol, 100);
    
    // Calculate price statistics
    if (!recentTrades.isEmpty()) {
        QVector<double> prices;
        for (const auto& trade : recentTrades) {
            prices.append(trade.price);
        }
        
        std::sort(prices.begin(), prices.end());
        
        double highPrice = prices.last();
        double lowPrice = prices.first();
        double lastPrice = recentTrades.last().price;
        
        // Calculate VWAP
        double totalValue = 0.0;
        int totalVolume = 0;
        for (const auto& trade : recentTrades) {
            totalValue += trade.price * trade.quantity;
            totalVolume += trade.quantity;
        }
        double vwap = totalVolume > 0 ? totalValue / totalVolume : 0.0;
        
        stats["symbol"] = symbol;
        stats["last_price"] = lastPrice;
        stats["high_price"] = highPrice;
        stats["low_price"] = lowPrice;
        stats["vwap"] = vwap;
        stats["total_volume"] = totalVolume;
        stats["trade_count"] = recentTrades.size();
    }
    
    return stats;
}

void TradingEngine::clear() {
    QMutexLocker orderLocker(&orderQueueMutex);
    QMutexLocker tradeLocker(&tradeHistoryMutex);
    QMutexLocker statsLocker(&statsMutex);
    
    orderBooks.clear();
    activeOrders.clear();
    orderQueue.clear();
    tradeHistory.clear();
    latencyMeasurements.clear();
    
    totalTrades = 0;
    totalVolume = 0;
    ordersPerSecond = 0;
    ordersProcessedSinceLastUpdate = 0;
}

void TradingEngine::processOrders() {
    if (!running) {
        return;
    }
    
    // Process up to 10 orders per cycle to prevent blocking
    int ordersToProcess = qMin(10, orderQueue.size());
    
    for (int i = 0; i < ordersToProcess; ++i) {
        QMutexLocker locker(&orderQueueMutex);
        
        if (orderQueue.isEmpty()) {
            break;
        }
        
        OrderPtr order = orderQueue.dequeue();
        locker.unlock();
        
        processOrder(order);
    }
}

void TradingEngine::updateStats() {
    QMutexLocker locker(&statsMutex);
    
    qint64 elapsed = lastStatsUpdate.restart();
    if (elapsed > 0) {
        ordersPerSecond = static_cast<int>((ordersProcessedSinceLastUpdate * 1000.0) / elapsed);
        ordersProcessedSinceLastUpdate = 0;
    }
    
    // Emit performance stats update
    emit performanceStatsUpdated(getPerformanceStats());
}

void TradingEngine::processOrder(OrderPtr order) {
    QElapsedTimer processTimer;
    processTimer.start();
    
    QMutexLocker locker(&orderQueueMutex);
    
    // Add to active orders
    activeOrders[order->getOrderId()] = order;
    
    // Get order book
    auto orderBook = getOrderBook(order->getSymbol());
    
    locker.unlock();
    
    // Try to match the order
    matchOrder(order, orderBook);
    
    locker.relock();
    
    // If order still has quantity, add to book
    if (order->isActive() && order->getQuantity() > 0) {
        orderBook->addOrder(order);
    } else {
        // Remove from active orders if completely filled or cancelled
        if (activeOrders.contains(order->getOrderId())) {
            activeOrders.remove(order->getOrderId());
        }
    }
    
    // Record processing latency
    qint64 latencyMs = processTimer.elapsed();
    
    QMutexLocker statsLocker(&statsMutex);
    latencyMeasurements.append(latencyMs);
    
    // Keep only recent measurements
    if (latencyMeasurements.size() > MAX_LATENCY_MEASUREMENTS) {
        latencyMeasurements.removeFirst();
    }
    
    ordersProcessedSinceLastUpdate++;
}

void TradingEngine::matchOrder(OrderPtr incomingOrder, std::shared_ptr<OrderBook> orderBook) {
    if (incomingOrder->getSide() == OrderSide::BUY) {
        matchBuyOrder(incomingOrder, orderBook);
    } else {
        matchSellOrder(incomingOrder, orderBook);
    }
}

void TradingEngine::matchBuyOrder(OrderPtr buyOrder, std::shared_ptr<OrderBook> orderBook) {
    while (buyOrder->getQuantity() > 0 && buyOrder->isActive()) {
        OrderPtr bestAsk = orderBook->getBestAsk();
        
        if (!bestAsk || bestAsk->getPrice() > buyOrder->getPrice()) {
            break; // No more matching orders
        }
        
        // Execute trade
        int tradeQuantity = qMin(buyOrder->getQuantity(), bestAsk->getQuantity());
        double tradePrice = bestAsk->getPrice(); // Price-time priority: use ask price
        
        executeTrade(buyOrder, bestAsk, tradeQuantity, tradePrice, orderBook);
        
        // Remove ask order if completely filled
        if (bestAsk->getQuantity() == 0) {
            orderBook->removeOrder(bestAsk->getOrderId(), OrderSide::SELL);
            
            QMutexLocker locker(&orderQueueMutex);
            if (activeOrders.contains(bestAsk->getOrderId())) {
                activeOrders.remove(bestAsk->getOrderId());
            }
        }
    }
}

void TradingEngine::matchSellOrder(OrderPtr sellOrder, std::shared_ptr<OrderBook> orderBook) {
    while (sellOrder->getQuantity() > 0 && sellOrder->isActive()) {
        OrderPtr bestBid = orderBook->getBestBid();
        
        if (!bestBid || bestBid->getPrice() < sellOrder->getPrice()) {
            break; // No more matching orders
        }
        
        // Execute trade
        int tradeQuantity = qMin(sellOrder->getQuantity(), bestBid->getQuantity());
        double tradePrice = bestBid->getPrice(); // Price-time priority: use bid price
        
        executeTrade(bestBid, sellOrder, tradeQuantity, tradePrice, orderBook);
        
        // Remove bid order if completely filled
        if (bestBid->getQuantity() == 0) {
            orderBook->removeOrder(bestBid->getOrderId(), OrderSide::BUY);
            
            QMutexLocker locker(&orderQueueMutex);
            if (activeOrders.contains(bestBid->getOrderId())) {
                activeOrders.remove(bestBid->getOrderId());
            }
        }
    }
}

void TradingEngine::executeTrade(OrderPtr buyOrder, OrderPtr sellOrder, 
                                int quantity, double price, std::shared_ptr<OrderBook> orderBook) {
    // Fill both orders
    buyOrder->fill(quantity, price);
    sellOrder->fill(quantity, price);
    
    // Create trade record
    Trade trade(generateTradeId(), buyOrder->getSymbol(), quantity, price,
                buyOrder->getTraderId(), sellOrder->getTraderId(),
                buyOrder->getOrderId(), sellOrder->getOrderId());
    
    // Add to trade history
    {
        QMutexLocker locker(&tradeHistoryMutex);
        tradeHistory.append(trade);
        
        // Keep only recent trades
        if (tradeHistory.size() > MAX_TRADE_HISTORY) {
            tradeHistory.removeFirst();
        }
        
        totalTrades++;
        totalVolume += quantity;
    }
    
    // Add to order book trade history
    orderBook->addTrade(trade);
    
    // Emit signals
    emit tradeExecuted(trade);
    emit orderFilled(buyOrder, quantity, price);
    emit orderFilled(sellOrder, quantity, price);
    
    // Notify traders of fills
    notifyTraderFill(buyOrder, quantity, price);
    notifyTraderFill(sellOrder, quantity, price);
}

void TradingEngine::notifyTraderFill(OrderPtr order, int quantity, double price) {
    QString traderId = order->getTraderId();
    if (traders.contains(traderId)) {
        try {
            Trader* trader = traders[traderId];
            trader->onOrderFilled(order, quantity, price);
        } catch (...) {
            qWarning() << "Error notifying trader" << traderId;
        }
    }
}

QString TradingEngine::generateTradeId() {
    return QString("%1").arg(totalTrades + 1, 6, 10, QChar('0'));
}