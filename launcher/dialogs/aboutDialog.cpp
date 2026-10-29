#include "aboutDialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "core/platformUtils.h"
#include "core/portPaths.h"

namespace
{
constexpr int ABOUT_MIN_WIDTH = 560;
constexpr int ABOUT_MIN_HEIGHT = 560;
constexpr int ABOUT_LAYOUT_SPACING = 10;
constexpr int VERSION_GRID_TOP_MARGIN = 4;
constexpr int VERSION_GRID_BOT_MARGIN = 8;
constexpr int VERSION_GRID_H_SPACING = 8;
constexpr int VERSION_GRID_V_SPACING = 4;
constexpr int VERSION_KEY_COLUMN = 0;
constexpr int VERSION_VALUE_COLUMN = 1;
constexpr int VERSION_COL_STRETCH = 1;
constexpr int BROWSER_STRETCH = 1;
} // namespace

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("About bbpc"));
    setMinimumSize(ABOUT_MIN_WIDTH, ABOUT_MIN_HEIGHT);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(ABOUT_LAYOUT_SPACING);

    //  Title / subtitle
    QLabel* titleLabel = new QLabel(QStringLiteral("<h2>%1</h2>").arg(QStringLiteral("bbpc")), this);
    titleLabel->setObjectName(QStringLiteral("aboutTitle"));
    titleLabel->setTextFormat(Qt::RichText);
    layout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel(tr("A native Linux port of Bloodborne, with a Qt launcher."), this);
    subtitleLabel->setObjectName(QStringLiteral("aboutSubtitle"));
    subtitleLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(subtitleLabel);

    //  Version table
    QWidget* versionWidget = new QWidget(this);
    QGridLayout* versionGrid = new QGridLayout(versionWidget);
    versionGrid->setContentsMargins(0, VERSION_GRID_TOP_MARGIN, 0, VERSION_GRID_BOT_MARGIN);
    versionGrid->setHorizontalSpacing(VERSION_GRID_H_SPACING);
    versionGrid->setVerticalSpacing(VERSION_GRID_V_SPACING);
    versionGrid->setColumnStretch(VERSION_VALUE_COLUMN, VERSION_COL_STRETCH);

    // The upstream port's version (its VERSION file), when the port has one.
    const QString portVersion = PortPaths::portVersion();
    const QList<QPair<QString, QString>> rows = {
        { tr("App version:"), QCoreApplication::applicationVersion() },
        { tr("Based on bbport:"), portVersion.isEmpty() ? tr("unknown") : portVersion },
        { tr("Qt version:"), QString::fromLatin1(qVersion()) },
        { tr("Package:"), packageTypeName() },
        { tr("Port folder:"), PortPaths::portDir() },
        { tr("Data folder:"), PortPaths::dataDir() },
    };
    for (int row = 0; row < rows.size(); ++row)
    {
        QLabel* key = new QLabel(QStringLiteral("<b>%1</b>").arg(rows.at(row).first.toHtmlEscaped()), versionWidget);
        key->setTextFormat(Qt::RichText);
        QLabel* value = new QLabel(rows.at(row).second, versionWidget);
        value->setTextFormat(Qt::PlainText);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        versionGrid->addWidget(key, row, VERSION_KEY_COLUMN);
        versionGrid->addWidget(value, row, VERSION_VALUE_COLUMN);
    }
    layout->addWidget(versionWidget);

    //  Disclaimer + Credits
    QTextBrowser* browser = new QTextBrowser(this);
    browser->setObjectName(QStringLiteral("aboutBrowser"));
    browser->setOpenExternalLinks(true);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setHtml(
        QStringLiteral(
            "<p><b>⚠ %1</b> %2</p>"
            "<hr/>"
            "<h3>%3</h3>"
            "<ul><li>Nicholas Page (<a href='https://github.com/wheat32'>wheat32</a>)</li></ul>"
            "<h3>%4</h3>"
            "<p>%5</p>"
            "<h3>%6</h3>"
            "<ul>"
            "<li>%7</li>"
            "<li>%8</li>"
            "<li>%9</li>"
            "<li>%10</li>"
            "<li>%11</li>"
            "<li>%12</li>"
            "</ul>"
            "<hr/>"
            "<p>%13</p>")
        .arg(
            tr("Disclaimer:"),
            tr("No game files are included: bbpc needs your own dump of Bloodborne with update 1.09. This project is "
               "<b>not affiliated with, endorsed by, or supported by</b> Sony Interactive Entertainment, FromSoftware, "
               "AMD, NVIDIA or the shadPS4 project. Bloodborne is a trademark of Sony Interactive Entertainment."),
            tr("Author"),
            tr("Original project"),
            tr("bbpc is a fork of <a href='https://github.com/deadinside28/bloodborne_pc'>bloodborne_pc</a> (bbport) by "
               "deadinside28 and its contributors. The port itself, from the loader and the PS4 runtime to the renderer, "
               "the upscalers and online play, is their work."),
            tr("Credits & Acknowledgments"),
            tr("Built with <a href='https://www.qt.io/'>Qt 6</a>"),
            tr("The renderer and shader translator started from <a href='https://github.com/shadps4-emu/shadPS4'>shadPS4</a> "
               "(GPL-2.0-or-later), which also provides the online module's network code"),
            tr("FSR 3.1 on Vulkan from <a href='https://github.com/FireBurn/FSR-Vulkan'>FSR-Vulkan</a> by FireBurn and AMD's "
               "FidelityFX SDK (MIT License)"),
            tr("<a href='https://github.com/Thealexbarney/LibAtrac9'>LibAtrac9</a> and "
               "<a href='https://github.com/ocornut/imgui'>Dear ImGui</a> (MIT License)"),
            tr("Seamless co-op from <a href='https://github.com/CLSAMG22/BBSC'>BBSC</a>, The Hunter's Dream from "
               "<a href='https://github.com/droogie/bbhost'>bbhost</a>"),
            tr("Game patches by Kyo, Lance McDonald, auser1337, illusion, emoose and other community members"),
            tr("Licensed under the GNU GPL v2 or later. This software is provided as-is, without warranty of any kind. "
               "Use at your own risk.")));
    layout->addWidget(browser, BROWSER_STRETCH);

    // The button is created here rather than taken from a standard QDialogButtonBox::Close: the box
    // polishes its own buttons as it creates them, so an object name set afterwards never picks up
    // the QSS rule.
    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    closeButton->setObjectName(QStringLiteral("dialogButton"));
    closeButton->setCursor(Qt::PointingHandCursor);

    QDialogButtonBox* buttons = new QDialogButtonBox(this);
    buttons->addButton(closeButton, QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(buttons);
}
