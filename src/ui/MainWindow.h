#pragma once

#include <QMainWindow>

class QStackedWidget;
class DashboardPage;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();

    QStackedWidget *pages_;
    DashboardPage *dashboard_;
};
