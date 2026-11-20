#pragma once

#include <QWidget>
#include "../../monitor/SystemSampler.h"

class MetricCard;
class LiveChart;
class CoreBarChart;
class QLabel;
class QGridLayout;

class DashboardPage : public QWidget
{
public:
    explicit DashboardPage(QWidget *parent = nullptr);
    void updateSample(const SystemSample &sample);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    MetricCard *cards_[4]{};
    QGridLayout *cardsLayout_;
    LiveChart *cpuChart_;
    LiveChart *ramChart_;
    LiveChart *swapChart_;
    CoreBarChart *cores_;
    QLabel *systemInfo_;
    QLabel *summary_;
    int cardColumns_ = 0;
};
