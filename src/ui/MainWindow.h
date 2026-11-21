#pragma once

#include <QMainWindow>
#include <QCache>
#include <QFileIconProvider>
#include "../monitor/SystemSampler.h"

class QStackedWidget;
class DashboardPage;
class QLabel;
class QProgressBar;
class QTableWidget;
class QTimer;
class QProcess;
class LiveChart;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    void setupMonitorPages();
    void refresh();
    void refreshMemoryApps();
    void readMemoryApps();
    QIcon iconForApplication(const QString &command);
    void setMemoryAppsStatus(const QString &text);

    QStackedWidget *pages_;
    DashboardPage *dashboard_;
    SystemSampler sampler_;
    QLabel *metricLabels_[3]{};
    QProgressBar *metricBars_[3]{};
    LiveChart *metricCharts_[3]{};
    QTableWidget *memoryTable_ = nullptr;
    QLabel *memoryStatus_ = nullptr;
    QFileIconProvider iconProvider_;
    QCache<QString, QIcon> appIcons_{128};
    QTimer *timer_ = nullptr;
    QTimer *processTimeout_ = nullptr;
    QProcess *process_ = nullptr;
};
