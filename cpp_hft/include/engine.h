#ifndef ENGINE_H
#define ENGINE_H

#include "order.h"
#include "orderbook.h"
#include "trader.h"
#include <QObject>
#include <QMap>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <QElapsedTimer>
#include <memory>

/**
 * @brief Main trading engine that handles order matching and execution
 */
class TradingEngine : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Construct a new Trading Engine object
     * @param parent Parent QObject
     */
    explicit TradingEngine(QObject* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~TradingEngine();
    
    /**
     * @brief Start the trading engine
     */
    void start();
    
    /**
     * @brief Stop the trading engine
     */
    void stop();
    
    /**
     * @brief Check if engine is running
     */
    bool isRunning() const { return running; }
    
    /**
     * @brief Register a trader with the engine
     */
    void registerTrader(Trader* trader);
    
    /**
     * @brief Get or create order book for a symbol
     */
    std::shared_ptr<OrderBook> getOrderBook(const QString& symbol);
    
    /**
     * @brief Submit an order for processing
     */
    void submitOrder(OrderPtr order);
    
    /**
     * @brief Cancel an order
     */
    bool cancelOrder(const QString& orderId);
    
    /**
     * @brief Get recent trades across all symbols
     */
    QVector<Trade> getRecentTrades(int count = 20) const;
    
    /**
     * @brief Get recent trades for a specific symbol
     */
    QVector<Trade> getRecentTradesForSymbol(const QString& symbol, int count = 10) const;
    
    /**
     * @brief Get all trades for export
     */
    QVector<Trade> getAllTrades() const;
    
    /**
     * @brief Get engine performance statistics
     */
    QMap<QString, QVariant> getPerformanceStats() const;
    
    /**
     * @brief Get market summary
     */
    QMap<QString, QVariant> getMarketSummary() const;
    
    /**
     * @brief Get all active orders for a trader
     */
    QVector<OrderPtr> getTraderOrders(const QString& traderId) const;
    
    /**
     * @brief Get detailed statistics for a symbol
     */
    QMap<QString, QVariant> getSymbolStatistics(const QString& symbol) const;
    
    /**
     * @brief Clear all data (orders, trades, order books)
     */
    void clear();

signals:
    /**
     * @brief Emitted when a trade is executed
     */
    void tradeExecuted(const Trade& trade);
    
    /**
     * @brief Emitted when an order is filled
     */
    void orderFilled(OrderPtr order, int fillQuantity, double fillPrice);
    
    /**
     * @brief Emitted when performance stats are updated
     */
    void performanceStatsUpdated(const QMap<QString, QVariant>& stats);

private slots:
    /**
     * @brief Process pending orders
     */
    void processOrders();
    
    /**
     * @brief Update processing statistics
     */
    void updateStats();

private:
    // Core data structures
    QMap<QString, std::shared_ptr<OrderBook>> orderBooks;  // symbol -> OrderBook
    QMap<QString, OrderPtr> activeOrders;                  // order_id -> order
    QMap<QString, Trader*> traders;                        // trader_id -> trader
    
    // Order processing
    QQueue<OrderPtr> orderQueue;
    QMutex orderQueueMutex;
    
    // Trade execution tracking
    QVector<Trade> tradeHistory;
    mutable QMutex tradeHistoryMutex;
    
    // Performance metrics
    QElapsedTimer startTime;
    int totalTrades;
    int totalVolume;
    QVector<qint64> latencyMeasurements;
    mutable QMutex statsMutex;
    
    // Threading and control
    bool running;
    QTimer* processingTimer;
    QTimer* statsTimer;
    
    // Processing statistics
    int ordersPerSecond;
    QElapsedTimer lastStatsUpdate;
    int ordersProcessedSinceLastUpdate;
    
    static constexpr int MAX_TRADE_HISTORY = 10000;
    static constexpr int MAX_LATENCY_MEASUREMENTS = 1000;
    
    /**
     * @brief Process a single order
     */
    void processOrder(OrderPtr order);
    
    /**
     * @brief Match an incoming order against the order book
     */
    void matchOrder(OrderPtr incomingOrder, std::shared_ptr<OrderBook> orderBook);
    
    /**
     * @brief Match a buy order against asks
     */
    void matchBuyOrder(OrderPtr buyOrder, std::shared_ptr<OrderBook> orderBook);
    
    /**
     * @brief Match a sell order against bids
     */
    void matchSellOrder(OrderPtr sellOrder, std::shared_ptr<OrderBook> orderBook);
    
    /**
     * @brief Execute a trade between two orders
     */
    void executeTrade(OrderPtr buyOrder, OrderPtr sellOrder, 
                     int quantity, double price, std::shared_ptr<OrderBook> orderBook);
    
    /**
     * @brief Notify a trader that their order was filled
     */
    void notifyTraderFill(OrderPtr order, int quantity, double price);
    
    /**
     * @brief Generate unique trade ID
     */
    QString generateTradeId();
};

#endif // ENGINE_H