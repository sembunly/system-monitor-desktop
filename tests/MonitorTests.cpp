#include "monitor/SystemSampler.h"
#include "ui/MainWindow.h"
#include "ui/pages/DashboardPage.h"
#include "ui/widgets/LiveChart.h"
#include "ui/widgets/CoreBarChart.h"
#include "ui/widgets/MetricCard.h"

#include <QApplication>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <iostream>

static void wait(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

#define CHECK(condition) do { if (!(condition)) { \
    std::cerr << "Failed: " #condition << " at line " << __LINE__ << '\n'; return 1; \
} } while (false)

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir preferences;
    CHECK(preferences.isValid());
    QCoreApplication::setOrganizationName("SystemMonitorTests");
    QCoreApplication::setApplicationName("IsolatedTests");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, preferences.path());

    SystemSampler sampler;
    CHECK(sampler.sample().cpu == -1);
    QThread::msleep(150);
    const auto sample = sampler.sample();
    CHECK(sample.cpu >= 0 && sample.cpu <= 100);
    CHECK(sample.memoryTotal > 0 && sample.memoryUsed <= sample.memoryTotal);
    CHECK(sample.memory >= 0 && sample.memory <= 100);
    CHECK(sample.diskTotal > 0 && sample.diskUsed <= sample.diskTotal);
    CHECK(sample.disk >= 0 && sample.disk <= 100);
#ifdef Q_OS_MACOS
    CHECK(sample.swap >= 0 && sample.swap <= 100);
    CHECK(sample.swapUsed <= sample.swapTotal);
    CHECK(!sample.cpuCores.isEmpty());
    for (const double usage : sample.cpuCores) CHECK(usage >= 0 && usage <= 100);
#endif

    MainWindow window;
    window.show();
    const auto charts = window.findChildren<LiveChart *>();
    CHECK(charts.size() == 6);
    CHECK(window.findChildren<MetricCard *>().size() == 4);
    auto *coreChart = window.findChild<CoreBarChart *>();
    CHECK(coreChart);
#ifdef Q_OS_MACOS
    CHECK(coreChart->coreCount() == sample.cpuCores.size());
#endif
    for (auto *chart : charts) CHECK(chart->sampleCount() == 1);
    auto *navigation = window.findChild<QListWidget *>();
    auto *pages = window.findChild<QStackedWidget *>();
    CHECK(navigation && pages && navigation->count() == 5 && pages->count() == 5);
    for (int i = 0; i < 5; ++i) {
        navigation->setCurrentRow(i);
        CHECK(pages->currentIndex() == i);
    }
    CHECK(navigation->item(4)->text() == "Settings");
    auto *interval = pages->widget(4)->findChild<QSpinBox *>();
    auto *paused = pages->widget(4)->findChild<QCheckBox *>();
    CHECK(interval && paused);
    int updates = 0;
    QObject::connect(window.statusBar(), &QStatusBar::messageChanged, &window,
        [&updates](const QString &) { ++updates; });
    interval->setValue(1);
    wait(1200);
    CHECK(updates >= 1);
    for (auto *chart : charts) CHECK(chart->sampleCount() >= 2);
    paused->setChecked(true);
    const int count = updates;
    const int chartCount = charts.first()->sampleCount();
    wait(1200);
    CHECK(updates == count);
    for (auto *chart : charts) CHECK(chart->sampleCount() == chartCount);
    CHECK(QSettings().value("refreshSeconds").toInt() == 1);
    CHECK(QSettings().value("paused").toBool());
#ifndef Q_OS_WIN
    navigation->setCurrentRow(2);
    auto *memoryApps = pages->widget(2)->findChild<QTableWidget *>("memoryApps");
    CHECK(memoryApps);
    QElapsedTimer timeout;
    timeout.start();
    while (!memoryApps->rowCount() && timeout.elapsed() < 6000) wait(50);
    CHECK(memoryApps->rowCount() > 0);
    auto *memoryDelegate = qobject_cast<QStyledItemDelegate *>(memoryApps->itemDelegateForColumn(1));
    CHECK(memoryDelegate);
    CHECK(memoryDelegate->displayText(999.0, QLocale::c()) == "999.0 MB");
    CHECK(memoryDelegate->displayText(1000.0, QLocale::c()) == "1.00 GB");
    CHECK(!memoryApps->item(0, 0)->icon().isNull());
    for (int i = 1; i < memoryApps->rowCount(); ++i)
        CHECK(memoryApps->item(i - 1, 1)->data(Qt::DisplayRole).toDouble() >=
              memoryApps->item(i, 1)->data(Qt::DisplayRole).toDouble());
    if (qEnvironmentVariableIsSet("MEMORY_PREVIEW"))
        CHECK(window.grab().save(qEnvironmentVariable("MEMORY_PREVIEW")));
#endif
    navigation->setCurrentRow(4);
    auto *sampleNow = pages->widget(4)->findChild<QPushButton *>();
    CHECK(sampleNow);
    sampleNow->click();
    CHECK(updates > count);
    for (auto *chart : charts) CHECK(chart->sampleCount() == chartCount + 1);
    LiveChart boundedChart("Test", {"CPU"}, {QColor("#38BDF8")});
    boundedChart.resize(600, 250);
    for (int i = 0; i < 4000; ++i) boundedChart.appendSample({double(i % 101)});
    CHECK(boundedChart.sampleCount() == 3600);
    boundedChart.setZoomControlsVisible(true);
    const auto ranges = boundedChart.findChildren<QPushButton *>();
    CHECK(ranges.size() == 4);
    for (auto *range : ranges) if (range->text() == "5m") range->click();
    CHECK(boundedChart.timeWindow() == 300000);
    boundedChart.setTimeWindow(0);
    CHECK(boundedChart.timeWindow() == 0);
    boundedChart.setScale(16, "GB");
    boundedChart.appendSample({-1});
    CHECK(!boundedChart.grab().isNull());
    navigation->setCurrentRow(0);
    window.resize(1000, 700);
    wait(80);
    CHECK(!window.grab().isNull());
    const auto cards = window.findChildren<MetricCard *>();
    CHECK(cards[0]->y() != cards[2]->y());
    window.resize(1440, 960);
    wait(80);
    CHECK(cards[0]->y() == cards[3]->y());
    SystemSample emptySwap = sample;
    emptySwap.swapTotal = emptySwap.swapUsed = 0;
    emptySwap.swap = 0;
    emptySwap.cpu = -1;
    DashboardPage edgeCaseDashboard;
    edgeCaseDashboard.resize(1000, 800);
    edgeCaseDashboard.updateSample(emptySwap);
    CHECK(!edgeCaseDashboard.grab().isNull());
    if (qEnvironmentVariableIsSet("MONITOR_PREVIEW"))
        CHECK(window.grab().save(qEnvironmentVariable("MONITOR_PREVIEW")));
    std::cout << "Native metrics, charts, navigation, refresh, pause, and preferences passed.\n";
    return 0;
}
