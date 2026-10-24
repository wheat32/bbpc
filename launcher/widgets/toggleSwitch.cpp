#include "toggleSwitch.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>

namespace
{
constexpr int ANIM_DURATION_MS = 150;
constexpr qreal KNOB_POS_OFF = 0.0;
constexpr qreal KNOB_POS_ON = 1.0;
constexpr int KNOB_INSET = 4;
constexpr int KNOB_OFFSET = 2;
constexpr int DISABLED_ALPHA = 90;
constexpr int FOCUS_RING_WIDTH = 1;

QColor blend(const QColor& from, const QColor& to, qreal amount)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount,
                            from.greenF() + (to.greenF() - from.greenF()) * amount,
                            from.blueF() + (to.blueF() - from.blueF()) * amount);
}
} // namespace

ToggleSwitch::ToggleSwitch(QWidget* parent) : QWidget(parent)
{
    setFixedSize(ToggleSwitch::sizeHint());
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    m_anim = new QPropertyAnimation(this, "knobPos", this);
    m_anim->setDuration(ANIM_DURATION_MS);
    m_anim->setEasingCurve(QEasingCurve::InOutQuad);
}

void ToggleSwitch::setOn(bool on, bool animate)
{
    if (m_on == on) return;
    m_on = on;
    m_anim->stop();
    if (animate)
    {
        m_anim->setStartValue(m_knobPos);
        m_anim->setEndValue(on ? KNOB_POS_ON : KNOB_POS_OFF);
        m_anim->start();
    }
    else
    {
        setKnobPos(on ? KNOB_POS_ON : KNOB_POS_OFF);
    }
}

void ToggleSwitch::setKnobPos(qreal value)
{
    m_knobPos = value;
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const int radius = height() / 2;
    QColor track = blend(palette().color(QPalette::Mid), palette().color(QPalette::Highlight), m_knobPos);
    QColor knob = Qt::white;
    if (isEnabled() == false)
    {
        track.setAlpha(DISABLED_ALPHA);
        knob.setAlpha(DISABLED_ALPHA);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(track);
    painter.drawRoundedRect(rect(), radius, radius);
    if (hasFocus())
    {
        painter.setPen(QPen(palette().color(QPalette::Highlight), FOCUS_RING_WIDTH));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(0, 0, -FOCUS_RING_WIDTH, -FOCUS_RING_WIDTH), radius, radius);
        painter.setPen(Qt::NoPen);
    }
    const int knobSize = height() - KNOB_INSET;
    const int knobMax = width() - knobSize - KNOB_OFFSET;
    const int knobX = static_cast<int>(KNOB_OFFSET + (knobMax - KNOB_OFFSET) * m_knobPos);
    painter.setBrush(knob);
    painter.drawEllipse(knobX, KNOB_OFFSET, knobSize, knobSize);
}

void ToggleSwitch::flip()
{
    setOn(m_on == false);
    emit toggled(m_on);
}

void ToggleSwitch::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        flip();
    }
    QWidget::mousePressEvent(event);
}

void ToggleSwitch::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        flip();
        return;
    }
    QWidget::keyPressEvent(event);
}
