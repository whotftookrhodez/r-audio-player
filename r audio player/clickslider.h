#pragma once

#include <QSlider>
#include <QMouseEvent>
#include <QStyleOptionSlider>
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
            QStyleOptionSlider opt;

            initStyleOption(&opt);

            const QRect handle = style()->subControlRect(
                QStyle::CC_Slider,
                &opt,
                QStyle::SC_SliderHandle,
                this
            );

            if (handle.contains(e->pos()))
            {
                QSlider::mousePressEvent(e);

                return;
            }

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

            setSliderDown(true);
            setSliderPosition(val);
            setValue(val);

            QSlider::mousePressEvent(e);

            return;
        }

        QSlider::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        QSlider::mouseReleaseEvent(e);

        setSliderDown(false);
    }
};