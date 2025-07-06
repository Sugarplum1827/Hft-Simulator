#ifndef ORDER_H
#define ORDER_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <memory>

/**
 * @brief Enumeration for order sides
 */
enum class OrderSide {
    BUY,
    SELL
};

/**
 * @brief Enumeration for order status
 */
enum class OrderStatus {
    PENDING,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED
};

/**
 * @brief Structure to represent a partial fill
 */
struct Fill {
    int quantity;
    double price;
    QDateTime timestamp;
    
    Fill(int qty, double prc, const QDateTime& time)
        : quantity(qty), price(prc), timestamp(time) {}
};

/**
 * @brief Represents a trading order in the system
 */
class Order {
public:
    /**
     * @brief Construct a new Order object
     * @param traderId ID of the trader placing the order
     * @param symbol Trading symbol (e.g., 'AAPL')
     * @param side BUY or SELL
     * @param quantity Number of shares
     * @param price Price per share
     */
    Order(const QString& traderId, const QString& symbol, OrderSide side, 
          int quantity, double price);
    
    // Getters
    QString getOrderId() const { return orderId; }
    QString getTraderId() const { return traderId; }
    QString getSymbol() const { return symbol; }
    OrderSide getSide() const { return side; }
    int getQuantity() const { return quantity; }
    int getOriginalQuantity() const { return originalQuantity; }
    double getPrice() const { return price; }
    OrderStatus getStatus() const { return status; }
    QDateTime getTimestamp() const { return timestamp; }
    const QVector<Fill>& getFills() const { return fills; }
    
    /**
     * @brief Fill part or all of the order
     * @param quantity Quantity being filled
     * @param price Fill price
     */
    void fill(int quantity, double price);
    
    /**
     * @brief Cancel the order
     */
    void cancel();
    
    /**
     * @brief Get total filled quantity
     */
    int getFilledQuantity() const;
    
    /**
     * @brief Get average fill price
     */
    double getAverageFillPrice() const;
    
    /**
     * @brief Check if order is completely filled
     */
    bool isComplete() const;
    
    /**
     * @brief Check if order is still active (can be filled)
     */
    bool isActive() const;
    
    /**
     * @brief Convert to string representation
     */
    QString toString() const;

private:
    QString orderId;
    QString traderId;
    QString symbol;
    OrderSide side;
    int quantity;
    int originalQuantity;
    double price;
    OrderStatus status;
    QDateTime timestamp;
    QVector<Fill> fills;
    
    /**
     * @brief Generate unique order ID
     */
    static QString generateOrderId();
};

using OrderPtr = std::shared_ptr<Order>;

#endif // ORDER_H