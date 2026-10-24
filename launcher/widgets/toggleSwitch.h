#pragma once

#include <QWidget>

class QPropertyAnimation;

// Animated on/off switch. The track takes the palette's highlight color when on.
class ToggleSwitch : public QWidget
{
    static constexpr int TOGGLE_WIDTH = 44;
    static constexpr int TOGGLE_HEIGHT = 24;

    Q_OBJECT
    Q_PROPERTY(qreal knobPos READ knobPos WRITE setKnobPos)

public:
    explicit ToggleSwitch(QWidget* parent = nullptr);

    [[nodiscard]] bool isOn() const
    {
        return m_on;
    }
    void setOn(bool on, bool animate = true);

    [[nodiscard]] qreal knobPos() const
    {
        return m_knobPos;
    }
    void setKnobPos(qreal value);

    [[nodiscard]] QSize sizeHint() const override
    {
        return { TOGGLE_WIDTH, TOGGLE_HEIGHT };
    }

signals:
    // Only for the user's own clicks and key presses, not for setOn().
    void toggled(bool on);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void flip();

    bool m_on = false;
    qreal m_knobPos = 0.0;
    QPropertyAnimation* m_anim = nullptr;
};
