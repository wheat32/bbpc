#pragma once

#include <QEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QTimer>

// Floating toast that shows a short message near the bottom of the parent window and disappears
// on its own after a configurable duration. A close button lets the user dismiss it early. Use the
// popup() factory: it handles lifetime by calling deleteLater once the fade-out finishes.
class ToastNotification : public QFrame
{
    static constexpr int DEFAULT_DURATION_MS = 4000;
    static constexpr int TOAST_LEFT_MARGIN = 14;
    static constexpr int TOAST_MARGIN = 8;
    static constexpr int TOAST_MSG_STRETCH = 1;
    static constexpr int TOAST_CLOSE_BTN_SIZE = 20;
    static constexpr int FADE_DURATION_MS = 400;
    static constexpr qreal OPACITY_SHOWN = 1.0;
    static constexpr qreal OPACITY_HIDDEN = 0.0;
    static constexpr int TOAST_BOTTOM_MARGIN = 72;
    static constexpr int TOAST_MAX_WIDTH = 640;

    Q_OBJECT

public:
    static void popup(const QWidget* anchor, const QString& message, int durationMs = DEFAULT_DURATION_MS)
    {
        // Self-managing lifetime: deleteLater in dismiss().
        new ToastNotification(message, anchor, durationMs);
    }

    explicit ToastNotification(const QString& message, const QWidget* anchor, int durationMs = DEFAULT_DURATION_MS)
        : QFrame(anchor != nullptr ? anchor->window() : nullptr)
    {
        setObjectName(QStringLiteral("toastNotification"));
        setMaximumWidth(TOAST_MAX_WIDTH);

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(TOAST_LEFT_MARGIN, TOAST_MARGIN, TOAST_MARGIN, TOAST_MARGIN);
        layout->setSpacing(TOAST_MARGIN);

        QLabel* messageLabel = new QLabel(message, this);
        messageLabel->setWordWrap(true);
        layout->addWidget(messageLabel, TOAST_MSG_STRETCH);

        QPushButton* closeButton = new QPushButton(QStringLiteral("✕"), this);
        closeButton->setObjectName(QStringLiteral("toastCloseBtn"));
        closeButton->setFixedSize(TOAST_CLOSE_BTN_SIZE, TOAST_CLOSE_BTN_SIZE);
        closeButton->setCursor(Qt::PointingHandCursor);
        connect(closeButton, &QPushButton::clicked, this, &ToastNotification::dismiss);
        layout->addWidget(closeButton);

        m_opacity = new QGraphicsOpacityEffect(this);
        m_opacity->setOpacity(OPACITY_SHOWN);
        setGraphicsEffect(m_opacity);

        m_fadeAnim = new QPropertyAnimation(m_opacity, "opacity", this);
        m_fadeAnim->setDuration(FADE_DURATION_MS);
        m_fadeAnim->setStartValue(OPACITY_SHOWN);
        m_fadeAnim->setEndValue(OPACITY_HIDDEN);
        connect(m_fadeAnim, &QPropertyAnimation::finished, this, &QObject::deleteLater);

        QTimer::singleShot(durationMs, this, &ToastNotification::dismiss);

        if (parentWidget() != nullptr)
        {
            parentWidget()->installEventFilter(this);
        }
        QFrame::show();
        raise();
        // Sized and placed once the layout has been resolved.
        QTimer::singleShot(0, this, [this]()
        {
            adjustSize();
            reposition();
        });
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == parentWidget() && event->type() == QEvent::Resize)
        {
            reposition();
        }
        return QFrame::eventFilter(watched, event);
    }

public slots:
    void dismiss()
    {
        if (m_dismissed) return;
        m_dismissed = true;
        m_fadeAnim->start();
    }

private:
    void reposition()
    {
        if (parentWidget() == nullptr) return;
        const QSize size = parentWidget()->size();
        move((size.width() - width()) / 2, size.height() - height() - TOAST_BOTTOM_MARGIN);
    }

    QGraphicsOpacityEffect* m_opacity = nullptr;
    QPropertyAnimation* m_fadeAnim = nullptr;
    bool m_dismissed = false;
};
