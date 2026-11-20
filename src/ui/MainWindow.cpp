#include "MainWindow.h"
#include "pages/DashboardPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
}

void MainWindow::setupUi()
{
    setWindowTitle("System Monitor");
    resize(1200, 760);
    setMinimumSize(900, 600);

    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *sidebar = new QWidget(central);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(230);

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
        "History",
        "Settings"
    });

    navigation->setCurrentRow(0);

    sidebarLayout->addWidget(brand);
    sidebarLayout->addSpacing(24);
    sidebarLayout->addWidget(navigation);

    pages_ = new QStackedWidget(central);
    dashboard_ = new DashboardPage(pages_);
    pages_->addWidget(dashboard_);

    root->addWidget(sidebar);
    root->addWidget(pages_, 1);

    setCentralWidget(central);

    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #0F172A;
            color: #F8FAFC;
            font-family: Arial;
        }

        #sidebar {
            background: #111827;
            border-right: 1px solid #334155;
        }

        #brand {
            font-size: 16px;
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
            border-radius: 8px;
        }

        QListWidget::item:selected {
            background: #1E40AF;
            color: white;
        }

        #pageTitle {
            font-size: 28px;
            font-weight: bold;
        }

        #metricCard {
            background: #1E293B;
            border: 1px solid #334155;
            border-radius: 12px;
        }

        #metricTitle {
            font-size: 14px;
            color: #94A3B8;
        }

        #metricValue {
            font-size: 30px;
            font-weight: bold;
            color: #38BDF8;
        }
    )");
}
