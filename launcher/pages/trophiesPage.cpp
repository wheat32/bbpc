#include "trophiesPage.h"

#include <QCoreApplication>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPixmap>
#include <QProgressBar>
#include <QStyle>

#include "widgets/icons.h"
#include "widgets/settingRows.h"

namespace
{
constexpr int ICON_SIZE = 48;
constexpr int PROGRESS_WIDTH = 160;
constexpr int PROGRESS_MIN = 0;
constexpr const char* INVALID_PROPERTY = "invalid";
constexpr const char* LOCKED_PROPERTY = "locked";

struct Grade
{
    QChar grade;
    const char* name;
};

// The grades best first, as the summary lists them.
const QList<Grade>& grades()
{
    static const QList<Grade> list = {
        { Trophies::PLATINUM, QT_TRANSLATE_NOOP("TrophiesPage", "Platinum") },
        { Trophies::GOLD, QT_TRANSLATE_NOOP("TrophiesPage", "Gold") },
        { Trophies::SILVER, QT_TRANSLATE_NOOP("TrophiesPage", "Silver") },
        { Trophies::BRONZE, QT_TRANSLATE_NOOP("TrophiesPage", "Bronze") },
    };
    return list;
}

QString gradeName(QChar grade)
{
    for (const Grade& entry : grades())
    {
        if (entry.grade == grade)
        {
            return QCoreApplication::translate("TrophiesPage", entry.name);
        }
    }
    return {};
}

void repolish(QWidget* widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}
} // namespace

TrophiesPage::TrophiesPage(const LauncherSettings& settings, const PortIni&, QWidget* parent)
    : LauncherPage(parent), m_language(settings.language)
{
    SettingsGroup* intro = new SettingsGroup({}, tr("Trophies earned in this saves folder. The game takes the names, "
                                                    "descriptions and icons from its own trophy pack: that needs the "
                                                    "trophy key, the same one shadPS4 asks for. The key is yours and "
                                                    "stays on this computer with the launcher settings"));
    addGroup(intro);

    SettingsGroup* key = new SettingsGroup(tr("Trophy key"));
    m_keyRow = new EntryRow(tr("Key (32 hexadecimal digits)"), settings.trophyKey, true);
    connect(m_keyRow, &EntryRow::textEdited, this, [this]()
    {
        showKeyState();
        emit changed();
    });
    key->addRow(m_keyRow);
    addGroup(key);

    m_summaryGroup = new SettingsGroup(tr("Progress"));
    addGroup(m_summaryGroup);
    m_listGroup = new SettingsGroup(tr("Trophies"));
    addGroup(m_listGroup);
    finish();
    showKeyState();
}

void TrophiesPage::store(LauncherSettings& settings, PortIni&) const
{
    settings.trophyKey = m_keyRow->text().trimmed();
}

void TrophiesPage::setUserDir(const QString& userDir)
{
    m_userDir = userDir;
    refresh();
}

void TrophiesPage::setLanguage(const QString& language)
{
    m_language = language;
}

void TrophiesPage::showKeyState()
{
    const QString text = m_keyRow->text().trimmed();
    const bool invalid = text.isEmpty() == false && Trophies::validKey(text) == false;
    m_keyRow->lineEdit()->setProperty(INVALID_PROPERTY, invalid);
    repolish(m_keyRow->lineEdit());
    m_keyRow->setSubtitle(invalid ? tr("Not a trophy key: it is 32 digits 0-9 and a-f. The game starts without it")
                                  : QString());
}

void TrophiesPage::clearRows()
{
    for (QWidget* row : m_summaryRows)
    {
        m_summaryGroup->removeRow(row);
    }
    m_summaryRows.clear();
    for (QWidget* row : m_trophyRows)
    {
        m_listGroup->removeRow(row);
    }
    m_trophyRows.clear();
}

void TrophiesPage::refresh()
{
    clearRows();
    if (m_userDir.isEmpty()) return;
    const Trophies::Content content = Trophies::load(m_userDir, m_language);
    if (content.total == 0)
    {
        SettingRow* none = new SettingRow(tr("No trophies yet"), tr("They appear here after the first trophy earned in the game"));
        m_summaryGroup->addRow(none);
        m_summaryRows.append(none);
        m_listGroup->hide();
        return;
    }

    SettingRow* earned = new SettingRow(content.named ? tr("Earned %1 of %2").arg(content.unlocked).arg(content.total)
                                                      : tr("Earned: %1").arg(content.unlocked));
    if (content.named)
    {
        QProgressBar* bar = new QProgressBar(earned);
        bar->setObjectName(QStringLiteral("trophyProgress"));
        bar->setRange(PROGRESS_MIN, content.total);
        bar->setValue(content.unlocked);
        bar->setTextVisible(false);
        bar->setFixedWidth(PROGRESS_WIDTH);
        earned->addSuffix(bar);
        QStringList counts;
        for (const Grade& grade : grades())
        {
            if (content.grades.contains(grade.grade) == false) continue;
            const Trophies::GradeCount count = content.grades.value(grade.grade);
            counts.append(QStringLiteral("%1: %2/%3").arg(gradeName(grade.grade)).arg(count.unlocked).arg(count.total));
        }
        earned->setSubtitle(counts.join(QStringLiteral(", ")));
    }
    m_summaryGroup->addRow(earned);
    m_summaryRows.append(earned);
    if (content.named == false)
    {
        SettingRow* unnamed = new SettingRow(tr("Names unknown"),
                                             tr("Enter the trophy key above and start the game: it decrypts the names "
                                                "and the icons"));
        m_summaryGroup->addRow(unnamed);
        m_summaryRows.append(unnamed);
    }

    for (const Trophies::Trophy& trophy : content.trophies)
    {
        SettingRow* row = trophyRow(trophy, content.named);
        m_listGroup->addRow(row);
        m_trophyRows.append(row);
    }
    m_listGroup->setVisible(content.trophies.isEmpty() == false);
}

SettingRow* TrophiesPage::trophyRow(const Trophies::Trophy& trophy, bool named)
{
    const bool secret = trophy.hidden && trophy.unlocked == false;
    const QString number = tr("Trophy #%1").arg(trophy.id);
    QString title = number;
    if (named && secret)
    {
        title = tr("Hidden trophy");
    }
    else if (named && trophy.name.isEmpty() == false)
    {
        title = trophy.name;
    }
    SettingRow* row = new SettingRow(title, secret ? tr("Shown once it is earned") : trophy.detail);
    row->setProperty(LOCKED_PROPERTY, trophy.unlocked == false);

    QLabel* icon = new QLabel(row);
    icon->setFixedSize(ICON_SIZE, ICON_SIZE);
    icon->setAlignment(Qt::AlignCenter);
    QPixmap picture;
    if (trophy.icon.isEmpty() == false && secret == false)
    {
        picture.load(trophy.icon);
    }
    if (picture.isNull())
    {
        picture = Icons::get(Icons::Kind::Trophy).pixmap(ICON_SIZE, ICON_SIZE);
    }
    icon->setPixmap(picture.scaled(ICON_SIZE, ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    row->addPrefix(icon);

    QStringList info;
    const QString grade = gradeName(trophy.grade);
    if (grade.isEmpty() == false)
    {
        info.append(grade);
    }
    if (trophy.unlocked)
    {
        const QDateTime moment = Trophies::unlockTime(trophy.time);
        info.append(moment.isValid() ? QLocale().toString(moment, QLocale::ShortFormat) : tr("Earned"));
    }
    QLabel* details = new QLabel(info.join(u'\n'), row);
    details->setObjectName(QStringLiteral("trophyInfo"));
    details->setTextFormat(Qt::PlainText);
    details->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addSuffix(details);
    return row;
}
