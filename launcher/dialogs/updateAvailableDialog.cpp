#include "updateAvailableDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/updateCheck.h"
#include "debug.h"
#include "widgets/uiHelpers.h"

namespace
{
constexpr int UPDATE_MIN_WIDTH = 420;
constexpr int UPDATE_SPACING = 12;
constexpr int UPDATE_H_MARGIN = 24;
constexpr int UPDATE_TOP_MARGIN = 20;
constexpr int UPDATE_BOTTOM_MARGIN = 16;
constexpr int UPDATE_BUTTON_SPACING = 8;
} // namespace

UpdateAvailableDialog::UpdateAvailableDialog(const QString& currentVersion, const QString& newVersion, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Update available"));
    setMinimumWidth(UPDATE_MIN_WIDTH);
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(UPDATE_SPACING);
    layout->setContentsMargins(UPDATE_H_MARGIN, UPDATE_TOP_MARGIN, UPDATE_H_MARGIN, UPDATE_BOTTOM_MARGIN);

    QLabel* titleLabel = new QLabel(QStringLiteral("<h2>%1</h2>").arg(tr("Update available").toHtmlEscaped()), this);
    titleLabel->setObjectName(QStringLiteral("aboutTitle"));
    titleLabel->setTextFormat(Qt::RichText);
    layout->addWidget(titleLabel);

    QLabel* bodyLabel = new QLabel(QStringLiteral("<p>%1</p><p>%2<br>%3</p>")
                                       .arg(tr("A new version of bbpc is available.").toHtmlEscaped(),
                                            tr("Current version: <b>v%1</b>").arg(currentVersion.toHtmlEscaped()),
                                            tr("New version: <b>v%1</b>").arg(newVersion.toHtmlEscaped())),
                                   this);
    bodyLabel->setTextFormat(Qt::RichText);
    bodyLabel->setWordWrap(true);
    layout->addWidget(bodyLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(UPDATE_BUTTON_SPACING);

    QPushButton* laterButton = new QPushButton(tr("Later"), this);
    connect(laterButton, &QPushButton::clicked, this, &QDialog::reject);

    QPushButton* downloadButton = new QPushButton(tr("Download"), this);
    downloadButton->setObjectName(QStringLiteral("dialogButton"));
    downloadButton->setCursor(Qt::PointingHandCursor);
    downloadButton->setDefault(true);
    connect(downloadButton, &QPushButton::clicked, this, [this]()
    {
        if (UiHelpers::openInBrowser(UpdateCheck::RELEASES_URL) == false)
        {
            DBG_APP(QStringLiteral("Could not open the browser for the update: ") + UpdateCheck::RELEASES_URL);
            QMessageBox::information(this, tr("Open manually"),
                                     tr("Could not open your browser. The new version is on this page:\n\n%1")
                                         .arg(UpdateCheck::RELEASES_URL));
        }
        accept();
    });

    buttonLayout->addWidget(laterButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(downloadButton);
    layout->addLayout(buttonLayout);
}
