#pragma once

#include <QDir>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

// The environment for programs run on the host (run.sh and its Python scripts, Proton, the file
// manager). Inside the AppImage, the runtime and AppRun may point search paths at the bundle's
// libraries and Qt plugins; a host program that inherits them loads the bundled libraries instead
// of its own. Every entry under $APPDIR is removed, and a variable left empty is unset. Outside an
// AppImage the environment is returned unchanged.
namespace HostEnvironment
{
    inline const QStringList& searchPathVariables()
    {
        static const QStringList variables = {
            QStringLiteral("LD_LIBRARY_PATH"),
            QStringLiteral("QT_PLUGIN_PATH"),
            QStringLiteral("QT_QPA_PLATFORM_PLUGIN_PATH"),
            QStringLiteral("PYTHONPATH"),
            QStringLiteral("PYTHONHOME"),
            QStringLiteral("XDG_DATA_DIRS"),
        };
        return variables;
    }

    inline QProcessEnvironment strip(QProcessEnvironment env, const QString& appDir)
    {
        if (appDir.isEmpty())
        {
            return env;
        }
        const QString bundle = QDir::cleanPath(appDir);
        for (const QString& variable : searchPathVariables())
        {
            if (env.contains(variable) == false) continue;
            QStringList kept;
            for (const QString& entry : env.value(variable).split(u':', Qt::SkipEmptyParts))
            {
                const bool inBundle = entry == bundle || entry.startsWith(bundle + u'/');
                if (inBundle == false)
                {
                    kept.append(entry);
                }
            }
            if (kept.isEmpty())
            {
                env.remove(variable);
            }
            else
            {
                env.insert(variable, kept.join(u':'));
            }
        }
        return env;
    }

    inline QProcessEnvironment host()
    {
        const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        if (qEnvironmentVariableIsSet("APPIMAGE") == false)
        {
            return env;
        }
        return strip(env, qEnvironmentVariable("APPDIR"));
    }
}
