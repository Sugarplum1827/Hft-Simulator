#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include "orderbook.h"
#include "trader.h"
#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QTextStream>

/**
 * @brief Utility class for exporting trading data to CSV format
 */
class CSVExporter {
public:
    /**
     * @brief Construct CSV exporter
     */
    CSVExporter() = default;
    
    /**
     * @brief Export trade data to CSV format
     * @param trades List of trades
     * @param filename Output filename (optional)
     * @return CSV content as string
     */
    QString exportTradesToCSV(const QVector<Trade>& trades, const QString& filename = "") const;
    
    /**
     * @brief Export order book snapshot data to CSV format
     * @param orderBooks Map of symbol to order book
     * @param filename Output filename (optional)
     * @return CSV content as string
     */
    QString exportOrderBookToCSV(const QMap<QString, std::shared_ptr<OrderBook>>& orderBooks, 
                                 const QString& filename = "") const;
    
    /**
     * @brief Export trader performance data to CSV format
     * @param traders List of traders
     * @param filename Output filename (optional)
     * @return CSV content as string
     */
    QString exportTraderPerformanceToCSV(const QVector<Trader*>& traders, 
                                        const QString& filename = "") const;
    
    /**
     * @brief Export engine performance metrics to CSV format
     * @param performanceStats Performance statistics map
     * @param filename Output filename (optional)
     * @return CSV content as string
     */
    QString exportPerformanceMetricsToCSV(const QMap<QString, QVariant>& performanceStats, 
                                         const QString& filename = "") const;
    
    /**
     * @brief Import orders from CSV file
     * @param filename CSV filename
     * @param engine Trading engine to submit orders to
     * @return Import results with success status and statistics
     */
    QMap<QString, QVariant> importOrdersFromCSV(const QString& filename, TradingEngine* engine) const;
    
    /**
     * @brief Validate CSV format for order import
     * @param filename CSV filename
     * @return Validation results
     */
    QMap<QString, QVariant> validateCSVFormat(const QString& filename) const;
    
    /**
     * @brief Get sample CSV format for reference
     * @return Sample CSV content
     */
    QString getSampleCSVFormat() const;

private:
    /**
     * @brief Write CSV content to file
     * @param content CSV content
     * @param filename Output filename
     * @return Success status
     */
    bool writeToFile(const QString& content, const QString& filename) const;
    
    /**
     * @brief Escape CSV field if needed
     * @param field Field content
     * @return Escaped field
     */
    QString escapeCsvField(const QString& field) const;
    
    /**
     * @brief Format timestamp for CSV
     * @param timestamp QDateTime object
     * @return Formatted timestamp string
     */
    QString formatTimestamp(const QDateTime& timestamp) const;
};

#endif // CSVEXPORTER_H