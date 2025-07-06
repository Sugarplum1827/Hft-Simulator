#include "order.h"
#include <QUuid>
#include <QDebug>

Order::Order(const QString& traderId, const QString& symbol, OrderSide side, 
             int quantity, double price)
    : orderId(generateOrderId())
    , traderId(traderId)
    , symbol(symbol)
    , side(side)
    , quantity(quantity)
    , originalQuantity(quantity)
    , price(price)
    , status(OrderStatus::PENDING)
    , timestamp(QDateTime::currentDateTime())
{
}

void Order::fill(int fillQuantity, double fillPrice) {
    if (fillQuantity > quantity) {
        qWarning() << "Fill quantity exceeds remaining order quantity";
        return;
    }
    
    // Record the fill
    fills.append(Fill(fillQuantity, fillPrice, QDateTime::currentDateTime()));
    
    // Update remaining quantity
    quantity -= fillQuantity;
    
    // Update status
    if (quantity == 0) {
        status = OrderStatus::FILLED;
    } else {
        status = OrderStatus::PARTIALLY_FILLED;
    }
}

void Order::cancel() {
    if (status == OrderStatus::PENDING || status == OrderStatus::PARTIALLY_FILLED) {
        status = OrderStatus::CANCELLED;
    }
}

int Order::getFilledQuantity() const {
    int total = 0;
    for (const auto& fill : fills) {
        total += fill.quantity;
    }
    return total;
}

double Order::getAverageFillPrice() const {
    if (fills.isEmpty()) {
        return 0.0;
    }
    
    double totalValue = 0.0;
    int totalQuantity = 0;
    
    for (const auto& fill : fills) {
        totalValue += fill.quantity * fill.price;
        totalQuantity += fill.quantity;
    }
    
    return totalQuantity > 0 ? totalValue / totalQuantity : 0.0;
}

bool Order::isComplete() const {
    return status == OrderStatus::FILLED;
}

bool Order::isActive() const {
    return status == OrderStatus::PENDING || status == OrderStatus::PARTIALLY_FILLED;
}

QString Order::toString() const {
    QString sideStr = (side == OrderSide::BUY) ? "BUY" : "SELL";
    return QString("Order(%1, %2, %3, %4, %5@%6)")
           .arg(orderId.left(8))
           .arg(traderId)
           .arg(symbol)
           .arg(sideStr)
           .arg(quantity)
           .arg(price, 0, 'f', 2);
}

QString Order::generateOrderId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}