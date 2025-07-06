#include "csvexporter.h"
#include "engine.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QRegularExpression>

QString CSVExporter::exportTradesToCSV(const QVector<Trade>& trades, const QString& filename) const {
    QString csvContent;
    QTextStream stream(&csvContent);
    
    // Write headers
    stream << "Trade ID,Timestamp,Symbol,Side,Quantity,Price,Value,Buyer ID,Seller ID,Buy Order ID,Sell Order ID\n";
    
    // Write trade data
    for (const auto& trade : trades) {
        stream << escapeCsvField(trade.tradeId) << ","
               << formatTimestamp(trade.timestamp) << ","
               << escapeCsvField(trade.symbol) << ","
               << "BUY" << ","  // From perspective of aggressive order
               << trade.quantity << ","
               << QString::number(trade.price, 'f', 4) << ","
               << QString::number(trade.quantity * trade.price, 'f', 2) << ","
               << escapeCsvField(trade.buyerId) << ","
               << escapeCsvField(trade.sellerId) << ","
               << escapeCsvField(trade.buyOrderId) << ","
               << escapeCsvField(trade.sellOrderId) << "\n";
    }
    
    // Write to file if filename provided
    if (!filename.isEmpty()) {
        writeToFile(csvContent, filename);
    }
    
    return csvContent;
}

QString CSVExporter::exportOrderBookToCSV(const QMap<QString, std::shared_ptr<OrderBook>>& orderBooks, 
                                          const QString& filename) const {
    QString csvContent;
    QTextStream stream(&csvContent);
    
    // Write headers
    stream << "Symbol,Timestamp,Side,Price Level,Price,Quantity,Order Count,Cumulative Volume\n";
    
    QDateTime timestamp = QDateTime::currentDateTime();
    
    // Write order book data
    for (auto it = orderBooks.begin(); it != orderBooks.end(); ++it) {
        const QString& symbol = it.key();
        auto orderBook = it.value();
        
        auto levels = orderBook->getTopLevels(10);
        const auto& bids = levels.first;
        const auto& asks = levels.second;
        
        // Write bid levels
        int cumulativeBidVolume = 0;
        for (int i = 0; i < bids.size(); ++i) {
            const auto& bid = bids[i];
            cumulativeBidVolume += bid.totalQuantity;
            
            stream << escapeCsvField(symbol) << ","
                   << formatTimestamp(timestamp) << ","
                   << "BID" << ","
                   << (i + 1) << ","  // Price level (1 = best)
                   << QString::number(bid.price, 'f', 4) << ","
                   << bid.totalQuantity << ","
                   << bid.orderCount << ","
                   << cumulativeBidVolume << "\n";
        }
        
        // Write ask levels
        int cumulativeAskVolume = 0;
        for (int i = 0; i < asks.size(); ++i) {
            const auto& ask = asks[i];
            cumulativeAskVolume += ask.totalQuantity;
            
            stream << escapeCsvField(symbol) << ","
                   << formatTimestamp(timestamp) << ","
                   << "ASK" << ","
                   << (i + 1) << ","  // Price level (1 = best)
                   << QString::number(ask.price, 'f', 4) << ","
                   << ask.totalQuantity << ","
                   << ask.orderCount << ","
                   << cumulativeAskVolume << "\n";
        }
    }
    
    // Write to file if filename provided
    if (!filename.isEmpty()) {
        writeToFile(csvContent, filename);
    }
    
    return csvContent;
}

QString CSVExporter::exportTraderPerformanceToCSV(const QVector<Trader*>& traders, 
                                                  const QString& filename) const {
    QString csvContent;
    QTextStream stream(&csvContent);
    
    // Write headers
    stream << "Trader ID,Initial Cash,Current Cash,Portfolio Value,Total P&L,P&L %,Orders Sent,Orders Filled,Fill Rate %,Total Volume,Avg Order Size\n";
    
    // Write trader performance data
    for (const auto& trader : traders) {
        double portfolioValue = trader->getPortfolioValue();
        double totalPnL = trader->getTotalPnL();
        double pnlPercentage = trader->getInitialCash() > 0 ? (totalPnL / trader->getInitialCash() * 100.0) : 0.0;
        double fillRate = trader->getOrdersSent() > 0 ? (static_cast<double>(trader->getOrdersFilled()) / trader->getOrdersSent() * 100.0) : 0.0;
        double avgOrderSize = trader->getOrdersFilled() > 0 ? (static_cast<double>(trader->getTotalVolume()) / trader->getOrdersFilled()) : 0.0;
        
        stream << escapeCsvField(trader->getTraderId()) << ","
               << QString::number(trader->getInitialCash(), 'f', 2) << ","
               << QString::number(trader->getCash(), 'f', 2) << ","
               << QString::number(portfolioValue, 'f', 2) << ","
               << QString::number(totalPnL, 'f', 2) << ","
               << QString::number(pnlPercentage, 'f', 2) << ","
               << trader->getOrdersSent() << ","
               << trader->getOrdersFilled() << ","
               << QString::number(fillRate, 'f', 2) << ","
               << trader->getTotalVolume() << ","
               << QString::number(avgOrderSize, 'f', 2) << "\n";
    }
    
    // Write to file if filename provided
    if (!filename.isEmpty()) {
        writeToFile(csvContent, filename);
    }
    
    return csvContent;
}

QString CSVExporter::exportPerformanceMetricsToCSV(const QMap<QString, QVariant>& performanceStats, 
                                                   const QString& filename) const {
    QString csvContent;
    QTextStream stream(&csvContent);
    
    // Write headers
    stream << "Metric,Value,Unit\n";
    
    // Write performance metrics
    QVector<QPair<QString, QPair<QString, QString>>> metrics = {
        {"Total Trades", {performanceStats.value("total_trades").toString(), "count"}},
        {"Total Volume", {performanceStats.value("total_volume").toString(), "shares"}},
        {"Trades Per Second", {QString::number(performanceStats.value("trades_per_second").toDouble(), 'f', 2), "trades/sec"}},
        {"Orders Per Second", {QString::number(performanceStats.value("orders_per_second").toDouble(), 'f', 2), "orders/sec"}},
        {"Average Latency", {QString::number(performanceStats.value("avg_latency_ms").toDouble(), 'f', 2), "milliseconds"}},
        {"Active Orders", {performanceStats.value("active_orders").toString(), "count"}},
        {"Runtime", {QString::number(performanceStats.value("runtime_seconds").toDouble(), 'f', 2), "seconds"}},
        {"Active Symbols", {performanceStats.value("symbols_active").toString(), "count"}}
    };
    
    for (const auto& metric : metrics) {
        stream << escapeCsvField(metric.first) << ","
               << escapeCsvField(metric.second.first) << ","
               << escapeCsvField(metric.second.second) << "\n";
    }
    
    // Write to file if filename provided
    if (!filename.isEmpty()) {
        writeToFile(csvContent, filename);
    }
    
    return csvContent;
}

QMap<QString, QVariant> CSVExporter::importOrdersFromCSV(const QString& filename, TradingEngine* engine) const {
    QMap<QString, QVariant> result;
    
    // First validate the CSV format
    auto validation = validateCSVFormat(filename);
    if (!validation["success"].toBool()) {
        return validation;
    }
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = QString("Could not open file: %1").arg(filename);
        return result;
    }
    
    QTextStream stream(&file);
    QString header = stream.readLine(); // Skip header
    
    int ordersSubmitted = 0;
    int ordersFailed = 0;
    QStringList errors;
    QStringList symbolsImported;
    QStringList tradersImported;
    
    int lineNumber = 1;
    while (!stream.atEnd()) {
        lineNumber++;
        QString line = stream.readLine().trimmed();
        
        if (line.isEmpty()) continue;
        
        QStringList fields = line.split(',');
        if (fields.size() < 5) {
            errors.append(QString("Line %1: Insufficient fields").arg(lineNumber));
            ordersFailed++;
            continue;
        }
        
        try {
            QString traderId = fields[0].trimmed();
            QString symbol = fields[1].trimmed().toUpper();
            QString sideStr = fields[2].trimmed().toUpper();
            int quantity = fields[3].trimmed().toInt();
            double price = fields[4].trimmed().toDouble();
            
            // Validate data
            if (traderId.isEmpty() || symbol.isEmpty()) {
                errors.append(QString("Line %1: Empty trader ID or symbol").arg(lineNumber));
                ordersFailed++;
                continue;
            }
            
            OrderSide side;
            if (sideStr == "BUY") {
                side = OrderSide::BUY;
            } else if (sideStr == "SELL") {
                side = OrderSide::SELL;
            } else {
                errors.append(QString("Line %1: Invalid side '%2'").arg(lineNumber).arg(sideStr));
                ordersFailed++;
                continue;
            }
            
            if (quantity <= 0 || price <= 0.0) {
                errors.append(QString("Line %1: Invalid quantity or price").arg(lineNumber));
                ordersFailed++;
                continue;
            }
            
            // Create and submit order
            auto order = std::make_shared<Order>(traderId, symbol, side, quantity, price);
            engine->submitOrder(order);
            
            ordersSubmitted++;
            
            // Track symbols and traders
            if (!symbolsImported.contains(symbol)) {
                symbolsImported.append(symbol);
            }
            if (!tradersImported.contains(traderId)) {
                tradersImported.append(traderId);
            }
            
        } catch (const std::exception& e) {
            errors.append(QString("Line %1: %2").arg(lineNumber).arg(e.what()));
            ordersFailed++;
        }
    }
    
    file.close();
    
    result["success"] = true;
    result["orders_submitted"] = ordersSubmitted;
    result["orders_failed"] = ordersFailed;
    result["total_rows"] = lineNumber - 1;
    result["errors"] = errors;
    result["symbols_imported"] = symbolsImported;
    result["traders_imported"] = tradersImported;
    
    return result;
}

QMap<QString, QVariant> CSVExporter::validateCSVFormat(const QString& filename) const {
    QMap<QString, QVariant> result;
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = QString("Could not open file: %1").arg(filename);
        return result;
    }
    
    QTextStream stream(&file);
    if (stream.atEnd()) {
        result["success"] = false;
        result["error"] = "File is empty";
        return result;
    }
    
    QString header = stream.readLine().trimmed();
    QStringList headerFields = header.split(',');
    
    // Required columns
    QStringList requiredColumns = {"trader_id", "symbol", "side", "quantity", "price"};
    QStringList foundColumns;
    
    for (const QString& field : headerFields) {
        foundColumns.append(field.trimmed().toLower());
    }
    
    // Check for required columns
    QStringList missingColumns;
    for (const QString& required : requiredColumns) {
        if (!foundColumns.contains(required)) {
            missingColumns.append(required);
        }
    }
    
    if (!missingColumns.isEmpty()) {
        result["success"] = false;
        result["error"] = QString("Missing required columns: %1").arg(missingColumns.join(", "));
        result["required_columns"] = requiredColumns;
        result["found_columns"] = foundColumns;
        return result;
    }
    
    // Count rows
    int rowCount = 0;
    while (!stream.atEnd()) {
        stream.readLine();
        rowCount++;
    }
    
    file.close();
    
    result["success"] = true;
    result["row_count"] = rowCount;
    
    return result;
}

QString CSVExporter::getSampleCSVFormat() const {
    return QString(
        "trader_id,symbol,side,quantity,price,timestamp\n"
        "TRADER_001,AAPL,BUY,100,150.25,2025-07-06 10:00:00\n"
        "TRADER_002,AAPL,SELL,75,150.50,2025-07-06 10:00:15\n"
        "TRADER_001,GOOGL,BUY,50,2800.75,2025-07-06 10:00:30\n"
    );
}

bool CSVExporter::writeToFile(const QString& content, const QString& filename) const {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write to file:" << filename;
        return false;
    }
    
    QTextStream stream(&file);
    stream << content;
    file.close();
    
    return true;
}

QString CSVExporter::escapeCsvField(const QString& field) const {
    if (field.contains(',') || field.contains('"') || field.contains('\n')) {
        QString escaped = field;
        escaped.replace('"', "\"\"");
        return QString("\"%1\"").arg(escaped);
    }
    return field;
}

QString CSVExporter::formatTimestamp(const QDateTime& timestamp) const {
    return timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
}