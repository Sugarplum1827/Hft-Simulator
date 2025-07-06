#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "engine.h"
#include "trader.h"
#include "csvexporter.h"
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QTimer>
#include <QProgressBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <QSplitter>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <memory>

/**
 * @brief Main window for the HFT Trading Simulation application
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Construct the main window
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MainWindow();

private slots:
    /**
     * @brief Start the trading simulation
     */
    void startSimulation();
    
    /**
     * @brief Stop the trading simulation
     */
    void stopSimulation();
    
    /**
     * @brief Update the GUI with latest data
     */
    void updateDisplay();
    
    /**
     * @brief Handle trade execution updates
     */
    void onTradeExecuted(const Trade& trade);
    
    /**
     * @brief Handle performance stats updates
     */
    void onPerformanceStatsUpdated(const QMap<QString, QVariant>& stats);
    
    /**
     * @brief Add a custom symbol
     */
    void addCustomSymbol();
    
    /**
     * @brief Remove selected symbol
     */
    void removeSymbol();
    
    /**
     * @brief Import CSV orders
     */
    void importCsvOrders();
    
    /**
     * @brief Export trades to CSV
     */
    void exportTrades();
    
    /**
     * @brief Export order book snapshot
     */
    void exportOrderBook();
    
    /**
     * @brief Clear all data
     */
    void clearAllData();
    
    /**
     * @brief Show about dialog
     */
    void showAbout();

private:
    // Core components
    std::unique_ptr<TradingEngine> engine;
    QVector<std::unique_ptr<Trader>> traders;
    std::unique_ptr<CSVExporter> csvExporter;
    
    // GUI components
    QWidget* centralWidget;
    QSplitter* mainSplitter;
    QSplitter* rightSplitter;
    
    // Control panel
    QGroupBox* controlGroup;
    QSpinBox* numTradersSpinBox;
    QDoubleSpinBox* initialCashSpinBox;
    QLineEdit* customSymbolEdit;
    QPushButton* addSymbolButton;
    QPushButton* removeSymbolButton;
    QTableWidget* symbolsTable;
    QPushButton* startButton;
    QPushButton* stopButton;
    QPushButton* clearButton;
    QPushButton* importCsvButton;
    
    // Performance metrics
    QGroupBox* metricsGroup;
    QLabel* totalTradesLabel;
    QLabel* tradesPerSecondLabel;
    QLabel* avgLatencyLabel;
    QLabel* activeOrdersLabel;
    QProgressBar* performanceBar;
    
    // Order book display
    QGroupBox* orderBookGroup;
    QComboBox* symbolComboBox;
    QTableWidget* orderBookTable;
    
    // Trade log
    QGroupBox* tradeLogGroup;
    QTableWidget* tradeLogTable;
    
    // Trader P&L
    QGroupBox* traderPnlGroup;
    QTableWidget* traderPnlTable;
    
    // Menu and toolbar
    QMenuBar* menuBar;
    QStatusBar* statusBar;
    QAction* exportTradesAction;
    QAction* exportOrderBookAction;
    QAction* aboutAction;
    QAction* exitAction;
    
    // Update timer
    QTimer* updateTimer;
    
    // State
    bool simulationRunning;
    QStringList activeSymbols;
    
    /**
     * @brief Setup the user interface
     */
    void setupUI();
    
    /**
     * @brief Setup menus and actions
     */
    void setupMenus();
    
    /**
     * @brief Setup connections between signals and slots
     */
    void setupConnections();
    
    /**
     * @brief Create the control panel
     */
    QWidget* createControlPanel();
    
    /**
     * @brief Create the performance metrics panel
     */
    QWidget* createMetricsPanel();
    
    /**
     * @brief Create the order book display
     */
    QWidget* createOrderBookPanel();
    
    /**
     * @brief Create the trade log display
     */
    QWidget* createTradeLogPanel();
    
    /**
     * @brief Create the trader P&L display
     */
    QWidget* createTraderPnlPanel();
    
    /**
     * @brief Update order book display for current symbol
     */
    void updateOrderBookDisplay();
    
    /**
     * @brief Update trade log display
     */
    void updateTradeLogDisplay();
    
    /**
     * @brief Update trader P&L display
     */
    void updateTraderPnlDisplay();
    
    /**
     * @brief Update performance metrics display
     */
    void updateMetricsDisplay();
    
    /**
     * @brief Create traders based on current settings
     */
    void createTraders();
    
    /**
     * @brief Get selected symbols from the symbols table
     */
    QStringList getSelectedSymbols() const;
    
    /**
     * @brief Update symbols table
     */
    void updateSymbolsTable();
    
    /**
     * @brief Format currency value for display
     */
    QString formatCurrency(double value) const;
    
    /**
     * @brief Format number with thousands separators
     */
    QString formatNumber(int value) const;
    
    /**
     * @brief Set simulation status and update UI accordingly
     */
    void setSimulationStatus(bool running);
};

#endif // MAINWINDOW_H