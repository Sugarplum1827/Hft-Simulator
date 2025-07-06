#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "order.h"
#include <QMap>
#include <QQueue>
#include <QMutex>
#include <QVector>

/**
 * @brief Structure to represent a price level in the order book
 */
struct PriceLevel {
    double price;
    int totalQuantity;
    int orderCount;
    QQueue<OrderPtr> orders;
    
    PriceLevel(double p = 0.0) : price(p), totalQuantity(0), orderCount(0) {}
};

/**
 * @brief Structure to represent a trade
 */
struct Trade {
    QString tradeId;
    QDateTime timestamp;
    QString symbol;
    int quantity;
    double price;
    QString buyerId;
    QString sellerId;
    QString buyOrderId;
    QString sellOrderId;
    
    Trade() = default;
    Trade(const QString& id, const QString& sym, int qty, double prc,
          const QString& buyer, const QString& seller,
          const QString& buyOrd, const QString& sellOrd)
        : tradeId(id), timestamp(QDateTime::currentDateTime()), symbol(sym)
        , quantity(qty), price(prc), buyerId(buyer), sellerId(seller)
        , buyOrderId(buyOrd), sellOrderId(sellOrd) {}
};

/**
 * @brief Represents one side of an order book (bids or asks)
 */
class OrderBookSide {
public:
    /**
     * @brief Construct order book side
     * @param isBidSide True for bid side (buy orders), False for ask side (sell orders)
     */
    explicit OrderBookSide(bool isBidSide = true);
    
    /**
     * @brief Add an order to this side of the book
     */
    void addOrder(OrderPtr order);
    
    /**
     * @brief Remove an order from this side of the book
     */
    bool removeOrder(const QString& orderId);
    
    /**
     * @brief Get the best price on this side
     */
    double getBestPrice() const;
    
    /**
     * @brief Get the best order (first order at best price)
     */
    OrderPtr getBestOrder() const;
    
    /**
     * @brief Get all orders at a specific price level
     */
    QVector<OrderPtr> getOrdersAtPrice(double price) const;
    
    /**
     * @brief Get top N price levels with their orders
     */
    QVector<PriceLevel> getTopLevels(int numLevels) const;
    
    /**
     * @brief Get total volume on this side
     */
    int getTotalVolume() const;
    
    /**
     * @brief Clear all orders
     */
    void clear();

private:
    bool isBidSide;
    QMap<double, PriceLevel> priceLevels;  // price -> price level
    QMap<QString, OrderPtr> orders;        // order_id -> order mapping
    mutable QMutex mutex;
};

/**
 * @brief Complete order book for a trading symbol
 */
class OrderBook {
public:
    /**
     * @brief Construct order book for a symbol
     * @param symbol Trading symbol (e.g., 'AAPL')
     */
    explicit OrderBook(const QString& symbol);
    
    /**
     * @brief Add an order to the appropriate side of the book
     */
    void addOrder(OrderPtr order);
    
    /**
     * @brief Remove an order from the book
     */
    bool removeOrder(const QString& orderId, OrderSide side);
    
    /**
     * @brief Get the best bid order
     */
    OrderPtr getBestBid() const;
    
    /**
     * @brief Get the best ask order
     */
    OrderPtr getBestAsk() const;
    
    /**
     * @brief Get the best bid price
     */
    double getBestBidPrice() const;
    
    /**
     * @brief Get the best ask price
     */
    double getBestAskPrice() const;
    
    /**
     * @brief Get the bid-ask spread
     */
    double getSpread() const;
    
    /**
     * @brief Get the mid price (average of best bid and ask)
     */
    double getMidPrice() const;
    
    /**
     * @brief Get top N levels from both sides of the book
     * @return QPair<QVector<PriceLevel>, QVector<PriceLevel>> (bids, asks)
     */
    QPair<QVector<PriceLevel>, QVector<PriceLevel>> getTopLevels(int numLevels = 5) const;
    
    /**
     * @brief Add a trade to the history
     */
    void addTrade(const Trade& trade);
    
    /**
     * @brief Get recent trades
     */
    QVector<Trade> getRecentTrades(int count = 10) const;
    
    /**
     * @brief Get total volume at a specific price
     */
    int getVolumeAtPrice(double price, OrderSide side) const;
    
    /**
     * @brief Check if the book is crossed (bid >= ask)
     */
    bool isCrossed() const;
    
    /**
     * @brief Get symbol
     */
    QString getSymbol() const { return symbol; }
    
    /**
     * @brief Clear the order book
     */
    void clear();

private:
    QString symbol;
    OrderBookSide bids;    // Buy orders
    OrderBookSide asks;    // Sell orders
    QVector<Trade> tradeHistory;
    mutable QMutex mutex;
    
    static constexpr int MAX_TRADE_HISTORY = 1000;
};

#endif // ORDERBOOK_H