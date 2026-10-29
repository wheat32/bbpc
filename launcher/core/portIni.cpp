#include "portIni.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>

#include "choices.h"

namespace
{
const QString HEADER = QStringLiteral("# bbport settings (in-game menu: Insert / hold L3+R3)");

// The key of a "key=value" line, or an empty string for comments and other lines.
QString lineKey(const QString& line)
{
    if (line.contains(u'=') == false || line.trimmed().startsWith(u'#'))
    {
        return {};
    }
    return line.section(u'=', 0, 0).trimmed();
}
} // namespace

const QList<QPair<QString, QString>>& PortIni::defaults()
{
    static const QList<QPair<QString, QString>> list = []()
    {
        // live_resolution is gone: live changes are always on (run.sh), an old key changes nothing.
        // output_res=auto: the monitor's own resolution.
        QList<QPair<QString, QString>> keys = {
            { QStringLiteral("upscaler"), QStringLiteral("fsr4") },
            { QStringLiteral("preset"), QStringLiteral("4") },
            { QStringLiteral("sharpen"), QStringLiteral("1") },
            { QStringLiteral("sharpness"), QStringLiteral("0.50") },
            { QStringLiteral("object_motion"), QStringLiteral("1") },
            { QStringLiteral("show_fps"), QStringLiteral("1") },
            { QStringLiteral("vbao"), QStringLiteral("0") },
            { QStringLiteral("anti_aliasing"), QStringLiteral("auto") },
            { QStringLiteral("texture_quality"), QStringLiteral("auto") },
            { QStringLiteral("output_res"), QStringLiteral("auto") },
            { QStringLiteral("model_lod"), QStringLiteral("0") },
            { QStringLiteral("frame_generation"), QStringLiteral("off") },
            { QStringLiteral("frame_generation_multiplier"), QStringLiteral("2") },
            { QStringLiteral("low_latency"), QStringLiteral("on") },
            { QStringLiteral("mouse_look"), QStringLiteral("1") },
            { QStringLiteral("mouse_sensitivity"), QStringLiteral("1.00") },
            { QStringLiteral("mouse_invert_y"), QStringLiteral("0") },
            { QStringLiteral("mouse_invert_x"), QStringLiteral("0") },
            { QStringLiteral("camera_speed_x"), QStringLiteral("1.00") },
            { QStringLiteral("camera_speed_y"), QStringLiteral("1.00") },
            { QStringLiteral("button_prompts"), QStringLiteral("auto") },
            { QStringLiteral("game_text_language"), QStringLiteral("auto") },
            { QStringLiteral("game_voice_language"), QStringLiteral("auto") },
        };
        for (const Choices::Effect& effect : Choices::effects())
        {
            keys.append({ QString::fromLatin1(effect.key), effect.defaultOn ? QStringLiteral("1") : QStringLiteral("0") });
        }
        return keys;
    }();
    return list;
}

void PortIni::load(const QString& path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) == false)
    {
        parse({});
        return;
    }
    parse(QString::fromUtf8(file.readAll()));
}

void PortIni::parse(const QString& text)
{
    m_order.clear();
    m_values.clear();
    m_lines.clear();
    m_fileKeys.clear();
    for (const auto& [key, value] : defaults())
    {
        setValue(key, value);
    }
    if (text.isEmpty())
    {
        return;
    }
    m_lines = text.split(u'\n');
    // The file's final newline gives no line of its own.
    if (m_lines.isEmpty() == false && m_lines.last().isEmpty())
    {
        m_lines.removeLast();
    }
    for (const QString& line : m_lines)
    {
        const QString key = lineKey(line);
        if (key.isEmpty())
        {
            continue;
        }
        setValue(key, line.section(u'=', 1).trimmed());
        m_fileKeys.insert(key);
    }
}

QString PortIni::serialize() const
{
    QStringList out;
    QSet<QString> written;
    for (const QString& line : m_lines)
    {
        const QString key = lineKey(line);
        if (key.isEmpty() == false && m_values.contains(key))
        {
            const std::optional<QString>& value = m_values.value(key);
            if (value.has_value())
            {
                out.append(key + u'=' + value.value());
            }
            written.insert(key);
            continue;
        }
        out.append(line);
    }
    if (m_lines.isEmpty())
    {
        out.append(HEADER);
    }
    for (const QString& key : m_order)
    {
        const std::optional<QString>& value = m_values.value(key);
        if (written.contains(key) == false && value.has_value())
        {
            out.append(key + u'=' + value.value());
        }
    }
    return out.join(u'\n') + u'\n';
}

bool PortIni::save(const QString& path) const
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text) == false) return false;
    file.write(serialize().toUtf8());
    return file.commit();
}

QString PortIni::value(const QString& key, const QString& fallback) const
{
    const std::optional<QString> value = m_values.value(key);
    return value.has_value() ? value.value() : fallback;
}

bool PortIni::contains(const QString& key) const
{
    return m_values.value(key).has_value();
}

void PortIni::setValue(const QString& key, const QString& value)
{
    if (m_values.contains(key) == false)
    {
        m_order.append(key);
    }
    m_values.insert(key, value);
}

void PortIni::remove(const QString& key)
{
    if (m_values.contains(key) == false)
    {
        m_order.append(key);
    }
    m_values.insert(key, std::nullopt);
}

std::optional<QString> PortIni::raw(const QString& key) const
{
    return m_values.value(key);
}

bool PortIni::inFile(const QString& key) const
{
    return m_fileKeys.contains(key);
}

bool PortIni::saveChanges(const QString& path, const PortIni& previous, const PortIni& current)
{
    PortIni file;
    file.load(path);
    for (const QString& key : current.keys())
    {
        const std::optional<QString> value = current.raw(key);
        const bool changed = value != previous.raw(key);
        if (changed == false && (file.inFile(key) || value.has_value() == false)) continue;
        if (value.has_value())
        {
            file.setValue(key, value.value());
        }
        else
        {
            file.remove(key);
        }
    }
    return file.save(path);
}
