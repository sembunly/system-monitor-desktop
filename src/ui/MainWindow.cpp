#include "MainWindow.h"
#include "pages/DashboardPage.h"
#include "widgets/LiveChart.h"
#ifdef Q_OS_MACOS
#include "../monitor/MacAppIcon.h"
#endif

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QFileInfo>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>
#include <QDateTime>
#include <QHeaderView>
#include <QProcess>
#include <QProcessEnvironment>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableWidget>
#include <QThread>
#include <QTimer>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMonitorPages();
    refresh();
}

MainWindow::~MainWindow()
{
    timer_->stop();
    processTimeout_->stop();
    disconnect(process_, nullptr, this, nullptr);
    if (process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(1000);
    }
    delete process_;
}

void MainWindow::setupUi()
{
    setWindowTitle("System Monitor");
    resize(1440, 960);
    setMinimumSize(1000, 700);

    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *sidebar = new QWidget(central);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(200);

    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 28, 16, 20);

    auto *brand = new QLabel("SYSTEM MONITOR", sidebar);
    brand->setObjectName("brand");

    auto *navigation = new QListWidget(sidebar);
    navigation->addItems({
        "Dashboard",
        "CPU",
        "Memory",
        "Disk",
        "Processes",
        "Settings"
    });

    navigation->setCurrentRow(0);

    sidebarLayout->addWidget(brand);
    auto *brandSubtitle = new QLabel("DESKTOP PERFORMANCE", sidebar);
    brandSubtitle->setObjectName("mutedText");
    sidebarLayout->addWidget(brandSubtitle);
    sidebarLayout->addSpacing(24);
    sidebarLayout->addWidget(navigation);
    monitorStatus_ = new QLabel(sidebar);
    monitorStatus_->setObjectName("monitorStatus");
    sidebarLayout->addWidget(monitorStatus_);
    auto *credit = new QLabel("Powered by SEM Bunly", sidebar);
    credit->setObjectName("mutedText");
    sidebarLayout->addWidget(credit);

    pages_ = new QStackedWidget(central);
    dashboard_ = new DashboardPage(pages_);
    pages_->addWidget(dashboard_);
    connect(navigation, &QListWidget::currentRowChanged, pages_, &QStackedWidget::setCurrentIndex);
    connect(pages_, &QStackedWidget::currentChanged, this, [this](int index) {
        if ((index == 2 || index == 4) && process_) refreshProcesses();
    });

    root->addWidget(sidebar);
    root->addWidget(pages_, 1);

    setCentralWidget(central);

    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #090F16;
            color: #E7EDF7;
            font-family: ".AppleSystemUIFont", "Segoe UI", sans-serif;
            font-size: 13px;
        }
        QLabel { background: transparent; }
        #mutedText { color: #94A6BF; font-size: 12px; }
        #monitorStatus { color: #34D399; padding: 12px; background: #152A2C; border-radius: 8px; }

        #sidebar {
            background: #0D151F;
            border-right: 1px solid #25334A;
        }

        #brand {
            font-size: 15px;
            font-weight: bold;
            color: #38BDF8;
        }

        QListWidget {
            background: transparent;
            border: none;
            font-size: 14px;
            outline: none;
        }

        QListWidget::item {
            padding: 14px;
            margin-bottom: 5px;
            border-radius: 8px;
        }
        QListWidget::item:hover { background: #1C2C44; }

        QListWidget::item:selected {
            background: #1D4D78;
            color: white;
        }

        #pageTitle {
            font-size: 28px;
            font-weight: bold;
        }

        #metricCard {
            background: #0D141C;
            border: 1px solid #25303C;
            border-radius: 10px;
        }

        #metricTitle {
            font-size: 11px;
            color: #94A3B8;
        }

        #metricValue {
            font-size: 30px;
            font-weight: bold;
            color: #38BDF8;
        }

        QTableWidget, QSpinBox, QLineEdit {
            background: #111D30;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 6px;
            selection-background-color: #1D4D78;
        }
        QTableWidget { gridline-color: #25334A; }
        QPushButton:hover { background: #1D4D78; }
        QHeaderView::section, QPushButton {
            background: #334155;
            color: #F8FAFC;
            padding: 8px;
            border: none;
        }
        QProgressBar { border: 1px solid #334155; text-align: center; min-height: 24px; }
        QProgressBar::chunk { background: #2563EB; }
    )");
}

namespace {
class MemorySizeDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QString displayText(const QVariant &value, const QLocale &locale) const override
    {
        const double mb = value.toDouble();
        return mb >= 1000 ? locale.toString(mb / 1000, 'f', 2) + " GB" :
                           locale.toString(mb, 'f', 1) + " MB";
    }
};

QString percent(double value)
{
    return value < 0 ? QStringLiteral("--") : QString::number(value, 'f', 1) + "%";
}

QString capacity(quint64 used, quint64 total)
{
    constexpr double gb = 1000000000.0;
    return QString("%1 / %2 GB").arg(double(used) / gb, 0, 'f', 2)
        .arg(double(total) / gb, 0, 'f', 2);
}

QTableWidget *makeTable(const QStringList &headers, QWidget *parent)
{
    auto *table = new QTableWidget(0, headers.size(), parent);
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    return table;
}
}

void MainWindow::setupMonitorPages()
{
    auto createPage = [this](const QString &title, const QString &description) {
        auto *page = new QWidget(pages_);
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(30, 30, 30, 30);
        layout->setSpacing(20);
        auto *heading = new QLabel(title, page);
        heading->setObjectName("pageTitle");
        layout->addWidget(heading);
        auto *subtitle = new QLabel(description, page);
        subtitle->setObjectName("mutedText");
        subtitle->setWordWrap(true);
        layout->addWidget(subtitle);
        pages_->addWidget(page);
        return layout;
    };

    const QStringList titles{"CPU", "Memory", "Disk"};
    const QStringList descriptions{
        QString("System-wide CPU utilization between samples. Logical processors: %1.")
            .arg(QThread::idealThreadCount()),
#ifdef Q_OS_MACOS
        "Active + wired + compressed memory. Reclaimable inactive cache is excluded.",
#else
        "Physical memory in use, excluding available memory.",
#endif
        "Used space on the volume containing your home directory."};
    for (int i = 0; i < 3; ++i) {
        auto *layout = createPage(titles[i], descriptions[i]);
        metricLabels_[i] = new QLabel("Waiting for a sample…");
        metricLabels_[i]->setWordWrap(true);
        metricLabels_[i]->setObjectName("metricValue");
        metricBars_[i] = new QProgressBar;
        metricBars_[i]->setRange(0, 100);
        layout->addWidget(metricLabels_[i]);
        layout->addWidget(metricBars_[i]);
        const QList<QColor> colors{QColor("#38BDF8"), QColor("#A78BFA"), QColor("#34D399")};
        metricCharts_[i] = new LiveChart(titles[i] + " activity", {titles[i]}, {colors[i]}, pages_);
        if (i == 1) {
            layout->setSpacing(10);
            metricLabels_[i]->setStyleSheet("font-size: 24px; font-weight: bold; color: #A78BFA;");
            memoryStatus_ = new QLabel("Memory per process (RSS) · waiting for refresh");
            memoryStatus_->setObjectName("mutedText");
            layout->addWidget(memoryStatus_);
            auto *splitter = new QSplitter(Qt::Vertical);
            metricCharts_[i]->setMinimumHeight(160);
            splitter->addWidget(metricCharts_[i]);
            memoryTable_ = makeTable({"Process Name", "Memory", "PID"}, splitter);
            memoryTable_->setItemDelegateForColumn(1, new MemorySizeDelegate(memoryTable_));
            memoryTable_->setObjectName("memoryApps");
            memoryTable_->setMinimumHeight(80);
            memoryTable_->setIconSize(QSize(24, 24));
            memoryTable_->verticalHeader()->setDefaultSectionSize(36);
            memoryTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            memoryTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
            memoryTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
            memoryTable_->setSortingEnabled(true);
            memoryTable_->sortItems(1, Qt::DescendingOrder);
            splitter->addWidget(memoryTable_);
            splitter->setStretchFactor(0, 1);
            splitter->setStretchFactor(1, 2);
            splitter->setChildrenCollapsible(false);
            layout->addWidget(splitter, 1);
        } else {
            layout->addWidget(metricCharts_[i], 1);
        }
    }

    auto *processLayout = createPage("Processes",
        "Read-only process list. CPU is the operating system's ps average; RSS is resident memory. "
        "Process CPU can exceed 100% when using multiple cores.");
    processStatus_ = new QLabel("Waiting for refresh");
    processLayout->addWidget(processStatus_);
    auto *refreshButton = new QPushButton("Refresh processes");
    processLayout->addWidget(refreshButton);
    processSearch_ = new QLineEdit;
    processSearch_->setPlaceholderText("Search by PID or command…");
    processSearch_->setClearButtonEnabled(true);
    processSearch_->setAccessibleName("Search processes");
    processLayout->addWidget(processSearch_);
    processTable_ = makeTable({"PID", "CPU %", "RSS (MB)", "Command"}, pages_);
    processTable_->setIconSize(QSize(24, 24));
    processTable_->setSortingEnabled(true);
    processTable_->sortItems(1, Qt::DescendingOrder);
    processLayout->addWidget(processTable_, 1);
    connect(processSearch_, &QLineEdit::textChanged, this, &MainWindow::filterProcesses);
    connect(processTable_->model(), &QAbstractItemModel::layoutChanged,
            this, [this] { filterProcesses(); });

    auto *settingsLayout = createPage("Settings", "Preferences are saved automatically.");
    settingsLayout->addWidget(new QLabel("Refresh interval (seconds)"));
    auto *interval = new QSpinBox;
    interval->setRange(1, 30);
    QSettings settings;
    interval->setValue(std::clamp(settings.value("refreshSeconds", 2).toInt(), 1, 30));
    settingsLayout->addWidget(interval);
    auto *paused = new QCheckBox("Pause automatic updates");
    paused->setChecked(settings.value("paused", false).toBool());
    monitorStatus_->setText("●  Updates paused");
    monitorStatus_->setVisible(paused->isChecked());
    settingsLayout->addWidget(paused);
    auto *sampleNow = new QPushButton("Sample now");
    settingsLayout->addWidget(sampleNow);
    settingsLayout->addStretch();
    timer_ = new QTimer(this);
    timer_->setInterval(interval->value() * 1000);
    connect(timer_, &QTimer::timeout, this, &MainWindow::refresh);
    connect(sampleNow, &QPushButton::clicked, this, &MainWindow::refresh);
    connect(interval, &QSpinBox::valueChanged, this, [this](int seconds) {
        QSettings().setValue("refreshSeconds", seconds);
        timer_->setInterval(seconds * 1000);
    });
    connect(paused, &QCheckBox::toggled, this, [this](bool value) {
        QSettings().setValue("paused", value);
        monitorStatus_->setVisible(value);
        if (value) timer_->stop(); else timer_->start();
    });
    if (!paused->isChecked()) timer_->start();

    process_ = new QProcess(this);
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("LC_ALL", "C");
    process_->setProcessEnvironment(environment);
    processTimeout_ = new QTimer(this);
    processTimeout_->setSingleShot(true);
    connect(processTimeout_, &QTimer::timeout, this, [this] {
        process_->kill();
        setProcessStatus("Process refresh timed out. Try refreshing again.");
    });
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshProcesses);
    connect(process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        processTimeout_->stop();
        if (code == 0 && status == QProcess::NormalExit) readProcesses();
        else setProcessStatus("Process refresh failed. Try refreshing again.");
    });
    connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        processTimeout_->stop();
        setProcessStatus("Cannot read processes: " + process_->errorString());
    });
}

void MainWindow::refresh()
{
    const auto sample = sampler_.sample();
    dashboard_->updateSample(sample);
    const double values[]{sample.cpu, sample.memory, sample.disk};
    metricLabels_[0]->setText(sample.cpu < 0 ? "Waiting for CPU sample…" : percent(sample.cpu));
    metricLabels_[1]->setText(sample.memory < 0 ? "Unavailable" :
        percent(sample.memory) + " · " + capacity(sample.memoryUsed, sample.memoryTotal));
    metricLabels_[2]->setText(sample.disk < 0 ? "Unavailable" :
        percent(sample.disk) + "\n" + capacity(sample.diskUsed, sample.diskTotal) + "\n" + sample.diskPath);
    for (int i = 0; i < 3; ++i) {
        metricBars_[i]->setValue(values[i] < 0 ? 0 : qRound(values[i]));
        metricBars_[i]->setFormat(values[i] < 0 ? "Unavailable" : "%p%");
        metricCharts_[i]->appendSample({values[i]});
    }
    statusBar()->showMessage("Updated " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    if (pages_->currentIndex() == 2 || pages_->currentIndex() == 4) refreshProcesses();
}

void MainWindow::refreshProcesses()
{
    if (process_->state() != QProcess::NotRunning) return;
#ifdef Q_OS_WIN
    setProcessStatus("Process listing is currently supported on macOS and Linux.");
#else
    setProcessStatus("Refreshing…");
    process_->setProgram("/bin/ps");
    process_->setArguments({"-axo", "pid=,pcpu=,rss=,comm="});
    process_->start();
    processTimeout_->start(5000);
#endif
}

void MainWindow::readProcesses()
{
    struct Row { QString pid; double cpu; double memory; QString command; };
    QList<Row> rows;
    const auto lines = QString::fromLocal8Bit(process_->readAllStandardOutput()).split('\n');
    static const QRegularExpression pattern("^\\s*(\\d+)\\s+([\\d.]+)\\s+(\\d+)\\s+(.+)$");
    for (const auto &line : lines) {
        const auto match = pattern.match(line);
        if (!match.hasMatch()) continue;
        rows.append({match.captured(1), match.captured(2).toDouble(),
            match.captured(3).toDouble() * 1024.0 / 1000000.0, match.captured(4)});
    }
    std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) { return a.cpu > b.cpu; });
    processTable_->setSortingEnabled(false);
    processTable_->setRowCount(rows.size());
    memoryTable_->setSortingEnabled(false);
    memoryTable_->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const auto &row = rows[i];
        const auto icon = iconForProcess(row.command);
        const auto name = QFileInfo(row.command).fileName();
        auto *appItem = new QTableWidgetItem(icon, name);
        appItem->setToolTip(row.command);
        memoryTable_->setItem(i, 0, appItem);
        auto *memoryItem = new QTableWidgetItem;
        memoryItem->setData(Qt::DisplayRole, row.memory);
        memoryTable_->setItem(i, 1, memoryItem);
        auto *pidItem = new QTableWidgetItem;
        pidItem->setData(Qt::DisplayRole, row.pid.toLongLong());
        memoryTable_->setItem(i, 2, pidItem);
        const QStringList cells{row.pid, QString::number(row.cpu, 'f', 1),
            QString::number(row.memory, 'f', 1), row.command};
        for (int j = 0; j < cells.size(); ++j) {
            auto *item = new QTableWidgetItem;
            if (j == 0) item->setData(Qt::DisplayRole, row.pid.toLongLong());
            if (j == 1) item->setData(Qt::DisplayRole, row.cpu);
            if (j == 2) item->setData(Qt::DisplayRole, cells[j].toDouble());
            if (j == 3) {
                item->setText(cells[j]);
                item->setIcon(icon);
            }
            processTable_->setItem(i, j, item);
        }
    }
    processTable_->setSortingEnabled(true);
    memoryTable_->setSortingEnabled(true);
    filterProcesses();
    setProcessStatus(QString("%1 processes · updated %2")
        .arg(rows.size()).arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

QIcon MainWindow::iconForProcess(const QString &command)
{
    const auto appEnd = command.indexOf(".app/", 0, Qt::CaseInsensitive);
    const QString bundle = appEnd >= 0 ? command.left(appEnd + 4) : QString();
    const QString key = bundle.isEmpty() ? QStringLiteral("<background-process>") : bundle;
    if (auto *cached = appIcons_.object(key)) return *cached;
    QIcon icon;
    if (!bundle.isEmpty()) {
#ifdef Q_OS_MACOS
        icon = macAppIcon(bundle);
#else
        icon = iconProvider_.icon(QFileInfo(bundle));
#endif
    }
    if (icon.isNull()) icon = iconProvider_.icon(QFileIconProvider::File);
    appIcons_.insert(key, new QIcon(icon));
    return icon;
}

void MainWindow::setProcessStatus(const QString &text)
{
    processStatus_->setText(text);
    memoryStatus_->setText("Memory per process (RSS) · " + text);
}

void MainWindow::filterProcesses()
{
    const QString query = processSearch_->text().trimmed();
    for (int row = 0; row < processTable_->rowCount(); ++row) {
        const bool matches = processTable_->item(row, 0)->text().contains(query, Qt::CaseInsensitive)
            || processTable_->item(row, 3)->text().contains(query, Qt::CaseInsensitive);
        processTable_->setRowHidden(row, !matches);
    }
}
