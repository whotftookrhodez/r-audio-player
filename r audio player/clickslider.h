#pragma once

#include <QSlider>
#include <QMouseEvent>
#include <QStyle>

class ClickSlider : public QSlider
{
public:
    using QSlider::QSlider;
protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            int val;

            if (orientation() == Qt::Horizontal)
            {
                val = QStyle::sliderValueFromPosition(
                    minimum(),
                    maximum(),
                    e->pos().x(),
                    width()
                );
            }
            else
            {
                val = QStyle::sliderValueFromPosition(
                    minimum(),
                    maximum(),
                    height() - e->pos().y(),
                    height()
                );
            }

            setValue(val);
            e->accept();
        }

        QSlider::mousePressEvent(e);
    }
};