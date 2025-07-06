#include "mainwindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QDateTime>
#include <QDir>
#include <algorithm>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , engine(std::make_unique<TradingEngine>(this))
    , csvExporter(std::make_unique<CSVExporter>())
    , updateTimer(new QTimer(this))
    , simulationRunning(false)
{
    activeSymbols = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"};
    
    setupUI();
    setupMenus();
    setupConnections();
    
    // Initialize displays
    updateSymbolsTable();
    setSimulationStatus(false);
    
    // Set initial window properties
    setWindowTitle("HFT Trading Simulation");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // Center window
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
}

MainWindow::~MainWindow() {
    if (simulationRunning) {
        stopSimulation();
    }
}

void MainWindow::setupUI() {
    centralWidget = new QWidget;
    setCentralWidget(centralWidget);
    
    // Create main splitter
    mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Create control panel (left side)
    QWidget* leftPanel = new QWidget;
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(createControlPanel());
    leftLayout->addWidget(createMetricsPanel());
    leftLayout->addStretch();
    
    // Create right splitter for displays
    rightSplitter = new QSplitter(Qt::Vertical);
    rightSplitter->addWidget(createOrderBookPanel());
    rightSplitter->addWidget(createTradeLogPanel());
    rightSplitter->addWidget(createTraderPnlPanel());
    
    // Add to main splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightSplitter);
    
    // Set splitter proportions
    mainSplitter->setSizes({300, 900});
    rightSplitter->setSizes({300, 300, 300});
    
    // Main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);
    mainLayout->setContentsMargins(5, 5, 5, 5);
}

void MainWindow::setupMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    importCsvButton = new QPushButton("Import CSV Orders");
    QAction* importAction = new QAction("&Import CSV Orders", this);
    importAction->setShortcut(QKeySequence::Open);
    connect(importAction, &QAction::triggered, this, &MainWindow::importCsvOrders);
    fileMenu->addAction(importAction);
    
    fileMenu->addSeparator();
    
    exportTradesAction = new QAction("Export &Trades", this);
    exportTradesAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(exportTradesAction, &QAction::triggered, this, &MainWindow::exportTrades);
    fileMenu->addAction(exportTradesAction);
    
    exportOrderBookAction = new QAction("Export &Order Book", this);
    exportOrderBookAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(exportOrderBookAction, &QAction::triggered, this, &MainWindow::exportOrderBook);
    fileMenu->addAction(exportOrderBookAction);
    
    fileMenu->addSeparator();
    
    exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
    
    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    
    QAction* clearAction = new QAction("&Clear All Data", this);
    clearAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    connect(clearAction, &QAction::triggered, this, &MainWindow::clearAllData);
    toolsMenu->addAction(clearAction);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    
    aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAction);
    
    // Status bar
    statusBar = this->statusBar();
    statusBar->showMessage("Ready");
}

void MainWindow::setupConnections() {
    // Engine connections
    connect(engine.get(), &TradingEngine::tradeExecuted, 
            this, &MainWindow::onTradeExecuted);
    connect(engine.get(), &TradingEngine::performanceStatsUpdated, 
            this, &MainWindow::onPerformanceStatsUpdated);
    
    // Update timer
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateDisplay);
    updateTimer->setInterval(1000); // Update every second
}

QWidget* MainWindow::createControlPanel() {
    controlGroup = new QGroupBox("Simulation Controls");
    QVBoxLayout* layout = new QVBoxLayout(controlGroup);
    
    // Trading configuration
    QGridLayout* configLayout = new QGridLayout;
    
    configLayout->addWidget(new QLabel("Number of Traders:"), 0, 0);
    numTradersSpinBox = new QSpinBox;
    numTradersSpinBox->setRange(1, 50);
    numTradersSpinBox->setValue(5);
    configLayout->addWidget(numTradersSpinBox, 0, 1);
    
    configLayout->addWidget(new QLabel("Initial Cash:"), 1, 0);
    initialCashSpinBox = new QDoubleSpinBox;
    initialCashSpinBox->setRange(1000.0, 10000000.0);
    initialCashSpinBox->setValue(100000.0);
    initialCashSpinBox->setPrefix("$");
    initialCashSpinBox->setDecimals(0);
    configLayout->addWidget(initialCashSpinBox, 1, 1);
    
    layout->addLayout(configLayout);
    
    // Symbols configuration
    layout->addWidget(new QLabel("Trading Symbols:"));
    
    symbolsTable = new QTableWidget(0, 2);
    symbolsTable->setHorizontalHeaderLabels({"Symbol", "Active"});
    symbolsTable->horizontalHeader()->setStretchLastSection(true);
    symbolsTable->setMaximumHeight(150);
    symbolsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(symbolsTable);
    
    // Add/remove symbols
    QHBoxLayout* symbolLayout = new QHBoxLayout;
    customSymbolEdit = new QLineEdit;
    customSymbolEdit->setPlaceholderText("Enter symbol (e.g., NVDA)");
    customSymbolEdit->setMaxLength(10);
    symbolLayout->addWidget(customSymbolEdit);
    
    addSymbolButton = new QPushButton("Add");
    connect(addSymbolButton, &QPushButton::clicked, this, &MainWindow::addCustomSymbol);
    connect(customSymbolEdit, &QLineEdit::returnPressed, this, &MainWindow::addCustomSymbol);
    symbolLayout->addWidget(addSymbolButton);
    
    removeSymbolButton = new QPushButton("Remove");
    connect(removeSymbolButton, &QPushButton::clicked, this, &MainWindow::removeSymbol);
    symbolLayout->addWidget(removeSymbolButton);
    
    layout->addLayout(symbolLayout);
    
    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    
    startButton = new QPushButton("Start Simulation");
    startButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }");
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startSimulation);
    buttonLayout->addWidget(startButton);
    
    stopButton = new QPushButton("Stop Simulation");
    stopButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; }");
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopSimulation);
    buttonLayout->addWidget(stopButton);
    
    layout->addLayout(buttonLayout);
    
    // Import/Clear buttons
    QHBoxLayout* toolLayout = new QHBoxLayout;
    
    QPushButton* importButton = new QPushButton("Import CSV");
    connect(importButton, &QPushButton::clicked, this, &MainWindow::importCsvOrders);
    toolLayout->addWidget(importButton);
    
    clearButton = new QPushButton("Clear All");
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearAllData);
    toolLayout->addWidget(clearButton);
    
    layout->addLayout(toolLayout);
    
    return controlGroup;
}

QWidget* MainWindow::createMetricsPanel() {
    metricsGroup = new QGroupBox("Performance Metrics");
    QGridLayout* layout = new QGridLayout(metricsGroup);
    
    layout->addWidget(new QLabel("Total Trades:"), 0, 0);
    totalTradesLabel = new QLabel("0");
    totalTradesLabel->setStyleSheet("font-weight: bold; color: blue;");
    layout->addWidget(totalTradesLabel, 0, 1);
    
    layout->addWidget(new QLabel("Trades/Second:"), 1, 0);
    tradesPerSecondLabel = new QLabel("0.00");
    tradesPerSecondLabel->setStyleSheet("font-weight: bold; color: green;");
    layout->addWidget(tradesPerSecondLabel, 1, 1);
    
    layout->addWidget(new QLabel("Avg Latency (ms):"), 2, 0);
    avgLatencyLabel = new QLabel("0.00");
    avgLatencyLabel->setStyleSheet("font-weight: bold; color: orange;");
    layout->addWidget(avgLatencyLabel, 2, 1);
    
    layout->addWidget(new QLabel("Active Orders:"), 3, 0);
    activeOrdersLabel = new QLabel("0");
    activeOrdersLabel->setStyleSheet("font-weight: bold; color: purple;");
    layout->addWidget(activeOrdersLabel, 3, 1);
    
    performanceBar = new QProgressBar;
    performanceBar->setRange(0, 100);
    performanceBar->setValue(0);
    performanceBar->setFormat("Engine Load: %p%");
    layout->addWidget(performanceBar, 4, 0, 1, 2);
    
    return metricsGroup;
}

QWidget* MainWindow::createOrderBookPanel() {
    orderBookGroup = new QGroupBox("Order Book");
    QVBoxLayout* layout = new QVBoxLayout(orderBookGroup);
    
    // Symbol selector
    QHBoxLayout* symbolLayout = new QHBoxLayout;
    symbolLayout->addWidget(new QLabel("Symbol:"));
    symbolComboBox = new QComboBox;
    connect(symbolComboBox, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &MainWindow::updateOrderBookDisplay);
    symbolLayout->addWidget(symbolComboBox);
    symbolLayout->addStretch();
    layout->addLayout(symbolLayout);
    
    // Order book table
    orderBookTable = new QTableWidget(0, 4);
    orderBookTable->setHorizontalHeaderLabels({"Side", "Price", "Quantity", "Total"});
    orderBookTable->horizontalHeader()->setStretchLastSection(true);
    orderBookTable->setAlternatingRowColors(true);
    orderBookTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(orderBookTable);
    
    return orderBookGroup;
}

QWidget* MainWindow::createTradeLogPanel() {
    tradeLogGroup = new QGroupBox("Trade Execution Log");
    QVBoxLayout* layout = new QVBoxLayout(tradeLogGroup);
    
    tradeLogTable = new QTableWidget(0, 7);
    tradeLogTable->setHorizontalHeaderLabels({"Time", "Symbol", "Side", "Price", "Quantity", "Buyer", "Seller"});
    tradeLogTable->horizontalHeader()->setStretchLastSection(true);
    tradeLogTable->setAlternatingRowColors(true);
    tradeLogTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    tradeLogTable->setSortingEnabled(true);
    layout->addWidget(tradeLogTable);
    
    return tradeLogGroup;
}

QWidget* MainWindow::createTraderPnlPanel() {
    traderPnlGroup = new QGroupBox("Trader P&L");
    QVBoxLayout* layout = new QVBoxLayout(traderPnlGroup);
    
    traderPnlTable = new QTableWidget(0, 6);
    traderPnlTable->setHorizontalHeaderLabels({"Trader ID", "Cash", "Portfolio Value", "Total P&L", "Orders Sent", "Fill Rate"});
    traderPnlTable->horizontalHeader()->setStretchLastSection(true);
    traderPnlTable->setAlternatingRowColors(true);
    traderPnlTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    traderPnlTable->setSortingEnabled(true);
    layout->addWidget(traderPnlTable);
    
    return traderPnlGroup;
}

void MainWindow::startSimulation() {
    if (simulationRunning) return;
    
    QStringList selectedSymbols = getSelectedSymbols();
    if (selectedSymbols.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select at least one trading symbol.");
        return;
    }
    
    // Update symbol combo box
    symbolComboBox->clear();
    symbolComboBox->addItems(selectedSymbols);
    
    // Create traders
    createTraders();
    
    // Start engine
    engine->start();
    
    // Start traders
    for (auto& trader : traders) {
        engine->registerTrader(trader.get());
        trader->startTrading();
    }
    
    setSimulationStatus(true);
    statusBar->showMessage("Simulation running...");
    
    // Start update timer
    updateTimer->start();
}

void MainWindow::stopSimulation() {
    if (!simulationRunning) return;
    
    // Stop update timer
    updateTimer->stop();
    
    // Stop traders
    for (auto& trader : traders) {
        trader->stopTrading();
    }
    
    // Stop engine
    engine->stop();
    
    setSimulationStatus(false);
    statusBar->showMessage("Simulation stopped");
    
    // Final update
    updateDisplay();
}

void MainWindow::updateDisplay() {
    if (!simulationRunning) return;
    
    updateOrderBookDisplay();
    updateTradeLogDisplay();
    updateTraderPnlDisplay();
    updateMetricsDisplay();
}

void MainWindow::onTradeExecuted(const Trade& trade) {
    // Update trade log immediately when trade occurs
    QMetaObject::invokeMethod(this, [this]() {
        updateTradeLogDisplay();
    }, Qt::QueuedConnection);
}

void MainWindow::onPerformanceStatsUpdated(const QMap<QString, QVariant>& stats) {
    // Update metrics immediately when stats are updated
    QMetaObject::invokeMethod(this, [this, stats]() {
        totalTradesLabel->setText(formatNumber(stats.value("total_trades").toInt()));
        tradesPerSecondLabel->setText(QString::number(stats.value("trades_per_second").toDouble(), 'f', 2));
        avgLatencyLabel->setText(QString::number(stats.value("avg_latency_ms").toDouble(), 'f', 2));
        activeOrdersLabel->setText(formatNumber(stats.value("active_orders").toInt()));
        
        // Update performance bar based on trades per second (scale 0-100 trades/sec to 0-100%)
        int performance = qMin(100, static_cast<int>(stats.value("trades_per_second").toDouble()));
        performanceBar->setValue(performance);
    }, Qt::QueuedConnection);
}

void MainWindow::addCustomSymbol() {
    QString symbol = customSymbolEdit->text().trimmed().toUpper();
    if (symbol.isEmpty()) return;
    
    if (!activeSymbols.contains(symbol)) {
        activeSymbols.append(symbol);
        updateSymbolsTable();
        customSymbolEdit->clear();
    } else {
        QMessageBox::information(this, "Information", "Symbol already exists.");
    }
}

void MainWindow::removeSymbol() {
    int currentRow = symbolsTable->currentRow();
    if (currentRow >= 0 && currentRow < activeSymbols.size()) {
        QString symbol = activeSymbols[currentRow];
        activeSymbols.removeAt(currentRow);
        updateSymbolsTable();
        
        // Remove from combo box if simulation is running
        if (simulationRunning) {
            int comboIndex = symbolComboBox->findText(symbol);
            if (comboIndex >= 0) {
                symbolComboBox->removeItem(comboIndex);
            }
        }
    }
}

void MainWindow::importCsvOrders() {
    QString filename = QFileDialog::getOpenFileName(this, 
        "Import CSV Orders", 
        QDir::homePath(), 
        "CSV files (*.csv);;All files (*.*)");
    
    if (filename.isEmpty()) return;
    
    auto result = csvExporter->importOrdersFromCSV(filename, engine.get());
    
    if (result["success"].toBool()) {
        int ordersSubmitted = result["orders_submitted"].toInt();
        int ordersFailed = result["orders_failed"].toInt();
        
        QString message = QString("Successfully imported %1 orders").arg(ordersSubmitted);
        if (ordersFailed > 0) {
            message += QString("\n%1 orders failed to import").arg(ordersFailed);
        }
        
        QMessageBox::information(this, "Import Complete", message);
        
        // Add imported symbols to active symbols if not already present
        QStringList importedSymbols = result["symbols_imported"].toStringList();
        for (const QString& symbol : importedSymbols) {
            if (!activeSymbols.contains(symbol)) {
                activeSymbols.append(symbol);
            }
        }
        updateSymbolsTable();
        
    } else {
        QMessageBox::warning(this, "Import Failed", result["error"].toString());
    }
}

void MainWindow::exportTrades() {
    auto trades = engine->getAllTrades();
    if (trades.isEmpty()) {
        QMessageBox::information(this, "Information", "No trades to export.");
        return;
    }
    
    QString filename = QFileDialog::getSaveFileName(this,
        "Export Trades",
        QString("trades_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV files (*.csv)");
    
    if (!filename.isEmpty()) {
        csvExporter->exportTradesToCSV(trades, filename);
        QMessageBox::information(this, "Export Complete", 
            QString("Exported %1 trades to %2").arg(trades.size()).arg(filename));
    }
}

void MainWindow::exportOrderBook() {
    QMap<QString, std::shared_ptr<OrderBook>> orderBooks;
    QStringList selectedSymbols = getSelectedSymbols();
    
    for (const QString& symbol : selectedSymbols) {
        orderBooks[symbol] = engine->getOrderBook(symbol);
    }
    
    if (orderBooks.isEmpty()) {
        QMessageBox::information(this, "Information", "No order book data to export.");
        return;
    }
    
    QString filename = QFileDialog::getSaveFileName(this,
        "Export Order Book Snapshot",
        QString("orderbook_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV files (*.csv)");
    
    if (!filename.isEmpty()) {
        csvExporter->exportOrderBookToCSV(orderBooks, filename);
        QMessageBox::information(this, "Export Complete", 
            QString("Exported order book snapshot to %1").arg(filename));
    }
}

void MainWindow::clearAllData() {
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Clear All Data",
        "Are you sure you want to clear all trading data?\nThis action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (simulationRunning) {
            stopSimulation();
        }
        
        engine->clear();
        traders.clear();
        
        // Clear all tables
        orderBookTable->setRowCount(0);
        tradeLogTable->setRowCount(0);
        traderPnlTable->setRowCount(0);
        
        // Reset metrics
        totalTradesLabel->setText("0");
        tradesPerSecondLabel->setText("0.00");
        avgLatencyLabel->setText("0.00");
        activeOrdersLabel->setText("0");
        performanceBar->setValue(0);
        
        statusBar->showMessage("All data cleared");
    }
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About HFT Trading Simulation",
        "HFT Trading Simulation v1.0\n\n"
        "A high-frequency trading simulator built with Qt.\n\n"
        "Features:\n"
        "• Real-time order matching engine\n"
        "• Multiple trading bots\n"
        "• Order book visualization\n"
        "• Performance analytics\n"
        "• CSV import/export\n\n"
        "Built with Qt 6 and C++17");
}

void MainWindow::updateOrderBookDisplay() {
    QString currentSymbol = symbolComboBox->currentText();
    if (currentSymbol.isEmpty()) return;
    
    auto orderBook = engine->getOrderBook(currentSymbol);
    auto levels = orderBook->getTopLevels(5);
    const auto& bids = levels.first;
    const auto& asks = levels.second;
    
    orderBookTable->setRowCount(0);
    
    // Add asks (in reverse order for proper display)
    for (int i = asks.size() - 1; i >= 0; --i) {
        const auto& ask = asks[i];
        int row = orderBookTable->rowCount();
        orderBookTable->insertRow(row);
        
        orderBookTable->setItem(row, 0, new QTableWidgetItem("ASK"));
        orderBookTable->setItem(row, 1, new QTableWidgetItem(QString::number(ask.price, 'f', 2)));
        orderBookTable->setItem(row, 2, new QTableWidgetItem(QString::number(ask.totalQuantity)));
        orderBookTable->setItem(row, 3, new QTableWidgetItem(formatCurrency(ask.price * ask.totalQuantity)));
        
        // Color coding
        orderBookTable->item(row, 0)->setBackground(QColor(255, 200, 200)); // Light red for asks
    }
    
    // Add separator if both bids and asks exist
    if (!bids.isEmpty() && !asks.isEmpty()) {
        int row = orderBookTable->rowCount();
        orderBookTable->insertRow(row);
        
        orderBookTable->setItem(row, 0, new QTableWidgetItem("---"));
        orderBookTable->setItem(row, 1, new QTableWidgetItem("---"));
        orderBookTable->setItem(row, 2, new QTableWidgetItem("---"));
        orderBookTable->setItem(row, 3, new QTableWidgetItem("---"));
        
        orderBookTable->item(row, 0)->setBackground(QColor(220, 220, 220)); // Gray separator
    }
    
    // Add bids
    for (const auto& bid : bids) {
        int row = orderBookTable->rowCount();
        orderBookTable->insertRow(row);
        
        orderBookTable->setItem(row, 0, new QTableWidgetItem("BID"));
        orderBookTable->setItem(row, 1, new QTableWidgetItem(QString::number(bid.price, 'f', 2)));
        orderBookTable->setItem(row, 2, new QTableWidgetItem(QString::number(bid.totalQuantity)));
        orderBookTable->setItem(row, 3, new QTableWidgetItem(formatCurrency(bid.price * bid.totalQuantity)));
        
        // Color coding
        orderBookTable->item(row, 0)->setBackground(QColor(200, 255, 200)); // Light green for bids
    }
    
    orderBookTable->resizeColumnsToContents();
}

void MainWindow::updateTradeLogDisplay() {
    auto recentTrades = engine->getRecentTrades(20);
    
    // Only update if the number of trades has changed
    if (recentTrades.size() == tradeLogTable->rowCount()) return;
    
    tradeLogTable->setRowCount(0);
    
    for (const auto& trade : recentTrades) {
        int row = tradeLogTable->rowCount();
        tradeLogTable->insertRow(row);
        
        tradeLogTable->setItem(row, 0, new QTableWidgetItem(trade.timestamp.toString("hh:mm:ss.zzz")));
        tradeLogTable->setItem(row, 1, new QTableWidgetItem(trade.symbol));
        tradeLogTable->setItem(row, 2, new QTableWidgetItem("BUY")); // Assuming aggressive side
        tradeLogTable->setItem(row, 3, new QTableWidgetItem(QString::number(trade.price, 'f', 2)));
        tradeLogTable->setItem(row, 4, new QTableWidgetItem(QString::number(trade.quantity)));
        tradeLogTable->setItem(row, 5, new QTableWidgetItem(trade.buyerId));
        tradeLogTable->setItem(row, 6, new QTableWidgetItem(trade.sellerId));
    }
    
    // Scroll to bottom to show latest trades
    tradeLogTable->scrollToBottom();
    tradeLogTable->resizeColumnsToContents();
}

void MainWindow::updateTraderPnlDisplay() {
    if (traders.isEmpty()) {
        traderPnlTable->setRowCount(0);
        return;
    }
    
    traderPnlTable->setRowCount(traders.size());
    
    for (int i = 0; i < traders.size(); ++i) {
        const auto& trader = traders[i];
        
        double portfolioValue = trader->getPortfolioValue();
        double totalPnL = trader->getTotalPnL();
        double fillRate = trader->getOrdersSent() > 0 ? 
            (static_cast<double>(trader->getOrdersFilled()) / trader->getOrdersSent() * 100.0) : 0.0;
        
        traderPnlTable->setItem(i, 0, new QTableWidgetItem(trader->getTraderId()));
        traderPnlTable->setItem(i, 1, new QTableWidgetItem(formatCurrency(trader->getCash())));
        traderPnlTable->setItem(i, 2, new QTableWidgetItem(formatCurrency(portfolioValue)));
        
        QTableWidgetItem* pnlItem = new QTableWidgetItem(formatCurrency(totalPnL));
        if (totalPnL > 0) {
            pnlItem->setForeground(QColor(0, 150, 0)); // Green for profit
        } else if (totalPnL < 0) {
            pnlItem->setForeground(QColor(200, 0, 0)); // Red for loss
        }
        traderPnlTable->setItem(i, 3, pnlItem);
        
        traderPnlTable->setItem(i, 4, new QTableWidgetItem(formatNumber(trader->getOrdersSent())));
        traderPnlTable->setItem(i, 5, new QTableWidgetItem(QString::number(fillRate, 'f', 1) + "%"));
    }
    
    traderPnlTable->resizeColumnsToContents();
}

void MainWindow::updateMetricsDisplay() {
    auto stats = engine->getPerformanceStats();
    
    totalTradesLabel->setText(formatNumber(stats.value("total_trades").toInt()));
    tradesPerSecondLabel->setText(QString::number(stats.value("trades_per_second").toDouble(), 'f', 2));
    avgLatencyLabel->setText(QString::number(stats.value("avg_latency_ms").toDouble(), 'f', 2));
    activeOrdersLabel->setText(formatNumber(stats.value("active_orders").toInt()));
    
    // Update performance bar based on trades per second
    int performance = qMin(100, static_cast<int>(stats.value("trades_per_second").toDouble()));
    performanceBar->setValue(performance);
}

void MainWindow::createTraders() {
    traders.clear();
    
    int numTraders = numTradersSpinBox->value();
    double initialCash = initialCashSpinBox->value();
    QStringList selectedSymbols = getSelectedSymbols();
    
    for (int i = 0; i < numTraders; ++i) {
        QString traderId = QString("BOT_%1").arg(i + 1, 3, 10, QChar('0'));
        auto trader = std::make_unique<Trader>(traderId, initialCash, selectedSymbols, engine.get(), this);
        traders.push_back(std::move(trader));
    }
}

QStringList MainWindow::getSelectedSymbols() const {
    QStringList selected;
    
    for (int i = 0; i < symbolsTable->rowCount(); ++i) {
        QTableWidgetItem* checkItem = symbolsTable->item(i, 1);
        if (checkItem && checkItem->checkState() == Qt::Checked) {
            QTableWidgetItem* symbolItem = symbolsTable->item(i, 0);
            if (symbolItem) {
                selected.append(symbolItem->text());
            }
        }
    }
    
    return selected;
}

void MainWindow::updateSymbolsTable() {
    symbolsTable->setRowCount(activeSymbols.size());
    
    for (int i = 0; i < activeSymbols.size(); ++i) {
        QTableWidgetItem* symbolItem = new QTableWidgetItem(activeSymbols[i]);
        symbolItem->setFlags(symbolItem->flags() & ~Qt::ItemIsEditable);
        symbolsTable->setItem(i, 0, symbolItem);
        
        QTableWidgetItem* checkItem = new QTableWidgetItem();
        checkItem->setCheckState(i < 2 ? Qt::Checked : Qt::Unchecked); // Check first 2 by default
        checkItem->setFlags(checkItem->flags() & ~Qt::ItemIsEditable);
        symbolsTable->setItem(i, 1, checkItem);
    }
    
    symbolsTable->resizeColumnsToContents();
}

QString MainWindow::formatCurrency(double value) const {
    return QString("$%1").arg(QString::number(value, 'f', 2));
}

QString MainWindow::formatNumber(int value) const {
    return QString("%L1").arg(value);
}

void MainWindow::setSimulationStatus(bool running) {
    simulationRunning = running;
    
    startButton->setEnabled(!running);
    stopButton->setEnabled(running);
    numTradersSpinBox->setEnabled(!running);
    initialCashSpinBox->setEnabled(!running);
    symbolsTable->setEnabled(!running);
    addSymbolButton->setEnabled(!running);
    removeSymbolButton->setEnabled(!running);
    customSymbolEdit->setEnabled(!running);
}