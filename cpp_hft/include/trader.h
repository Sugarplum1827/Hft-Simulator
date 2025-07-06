#ifndef TRADER_H
#define TRADER_H

#include "order.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QRandomGenerator>
#include <memory>

class TradingEngine;

/**
 * @brief Simulated trading bot that generates orders
 */
class Trader : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Construct a new Trader object
     * @param traderId Unique identifier for the trader
     * @param initialCash Starting cash amount
     * @param symbols List of symbols to trade
     * @param engine Reference to the trading engine
     * @param parent Parent QObject
     */
    explicit Trader(const QString& traderId, double initialCash, 
                   const QStringList& symbols, TradingEngine* engine,
                   QObject* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~Trader();
    
    // Getters
    QString getTraderId() const { return traderId; }
    double getCash() const { return cash; }
    double getInitialCash() const { return initialCash; }
    QStringList getSymbols() const { return symbols; }
    int getOrdersSent() const { return ordersSent; }
    int getOrdersFilled() const { return ordersFilled; }
    int getTotalVolume() const { return totalVolume; }
    QMap<QString, int> getPositions() const;
    
    /**
     * @brief Start the trading bot
     */
    void startTrading();
    
    /**
     * @brief Stop the trading bot
     */
    void stopTrading();
    
    /**
     * @brief Check if trader is active
     */
    bool isActive() const { return active; }
    
    /**
     * @brief Calculate current portfolio value based on market prices
     */
    double getPortfolioValue() const;
    
    /**
     * @brief Calculate total profit/loss
     */
    double getTotalPnL() const;
    
    /**
     * @brief Calculate P&L for a specific position
     */
    double getPositionPnL(const QString& symbol) const;
    
    /**
     * @brief Get trading statistics
     */
    QMap<QString, QVariant> getTradingStats() const;

public slots:
    /**
     * @brief Callback when an order is filled (called by the engine)
     * @param order The filled order
     * @param fillQuantity Quantity filled
     * @param fillPrice Price of the fill
     */
    void onOrderFilled(OrderPtr order, int fillQuantity, double fillPrice);

private slots:
    /**
     * @brief Generate and submit a random order
     */
    void generateOrder();

private:
    QString traderId;
    double initialCash;
    double cash;
    QStringList symbols;
    TradingEngine* engine;
    
    // Portfolio tracking
    QMap<QString, int> positions;         // symbol -> share count
    QMap<QString, double> averageCosts;   // symbol -> average cost basis
    QMap<QString, double> marketPriceCache; // symbol -> last known price
    
    // Trading statistics
    int ordersSent;
    int ordersFilled;
    int totalVolume;
    
    // Trading parameters
    int minOrderSize;
    int maxOrderSize;
    double priceVolatility;     // Price variation percentage
    int orderFrequencyMs;       // Milliseconds between orders
    
    // Threading and control
    bool active;
    QTimer* tradingTimer;
    mutable QMutex mutex;
    QRandomGenerator* random;
    
    /**
     * @brief Estimate current market price based on recent trades and order book
     */
    double estimateMarketPrice(const QString& symbol);
    
    /**
     * @brief Decide whether to buy or sell based on current position and market conditions
     */
    OrderSide decideOrderSide(const QString& symbol);
    
    /**
     * @brief Update position and cash after a fill
     */
    void updatePosition(const QString& symbol, OrderSide side, int quantity, double price);
};

#endif // TRADER_H