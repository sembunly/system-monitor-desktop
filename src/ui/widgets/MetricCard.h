#pragma once

#include <QFrame>
#include <QString>
#include <QColor>

class QLabel;
class UsageRing;

class MetricCard : public QFrame
{
    Q_OBJECT
public:
    explicit MetricCard(
        const QString &title,
        const QString &value,
        QWidget *parent = nullptr
    );

    void setValue(const QString &value);
    void setUsage(double usage);

private:
    QLabel *valueLabel_;
    UsageRing *ring_;
};
