#pragma once

#include <QFrame>
#include <QString>

class QLabel;

class MetricCard : public QFrame
{
public:
    explicit MetricCard(
        const QString &title,
        const QString &value,
        QWidget *parent = nullptr
    );

    void setValue(const QString &value);

private:
    QLabel *valueLabel_;
};
