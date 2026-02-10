#pragma once

#include <QLabel>
#include <QMouseEvent>

class ClickLabel : public QLabel
{
    Q_OBJECT
public:
    using QLabel::QLabel;
Q_SIGNALS:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            Q_EMIT clicked();
        }

        QLabel::mousePressEvent(e);
    }
};