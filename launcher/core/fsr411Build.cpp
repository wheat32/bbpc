#include "fsr411Build.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>

#include "hostEnvironment.h"
#include "portPaths.h"

namespace
{
constexpr int EXIT_UNSUPPORTED = 2;
constexpr int EXIT_NO_PROTON = 3;
constexpr int EXIT_TOOLS_MISSING = 4;
constexpr int EXIT_NOT_STARTED = 5;
constexpr int EXIT_REPLAY_MISMATCH = 6;
constexpr int EXIT_NO_LOADER = 7;
constexpr int EXIT_NO_RECORDER = 8;
// "[ 3/20]": the recording's progress (capture_all.sh), the current step and the step count.
const QRegularExpression STEP(QStringLiteral("^\\[\\s*(\\d+)/(\\d+)\\]"));
constexpr int STEP_CURRENT_GROUP = 1;
constexpr int STEP_COUNT_GROUP = 2;

QString translate(const char* text)
{
    return QCoreApplication::translate("Fsr411Build", text);
}
} // namespace

Fsr411Build::Command Fsr411Build::command(const QString& upscalerDll, const QString& loaderDll)
{
    Command command;
    command.env = HostEnvironment::host();
    command.workingDirectory = PortPaths::portDir();
    const bool packaged = PortPaths::packaged();
    if (packaged)
    {
        command.env.insert(QStringLiteral("BB_FSR4CAP_WORK"), PortPaths::dataDir() + QStringLiteral("/fsr4cap"));
        command.env.insert(QStringLiteral("BB_FSR4CAP_CLEAN"), QStringLiteral("1"));
    }
    // A package brings both tools prebuilt; from source build_assets.sh builds them.
    const QString tools = PortPaths::fsr4capDir() + QStringLiteral("/bin");
    if (QFileInfo(tools + QStringLiteral("/dxil-spirv")).isFile() && QFileInfo(tools + QStringLiteral("/fsr4cap.exe")).isFile())
    {
        command.env.insert(QStringLiteral("BB_FSR4CAP_TOOLS"), tools);
    }
    const QString explicitOut = qEnvironmentVariable("BB_FSR411_DIR");
    command.env.insert(QStringLiteral("BB_FSR411_OUT"), explicitOut.isEmpty()
        ? (packaged ? PortPaths::dataDir() : PortPaths::portDir()) + QStringLiteral("/fsr4_411")
        : explicitOut);
    command.program = QStringLiteral("bash");
    command.arguments = { PortPaths::fsr4capDir() + QStringLiteral("/build_assets.sh"), upscalerDll };
    if (loaderDll.isEmpty() == false)
    {
        command.arguments.append(loaderDll);
    }
    return command;
}

QString Fsr411Build::errorMessage(int exitCode)
{
    switch (exitCode)
    {
        case EXIT_UNSUPPORTED:
            return translate("The DLL does not fit: details in the log");
        case EXIT_NO_PROTON:
            return translate("No suitable Proton found: install GE-Proton 10 or newer (ProtonUp-Qt), or Proton "
                             "Experimental / Proton-CachyOS in Steam. If you have them, start any game with that "
                             "Proton in Steam so that Steam installs its runtime");
        case EXIT_TOOLS_MISSING:
            return translate("Tools for the build are missing: the list is in the log");
        case EXIT_NOT_STARTED:
            return translate("No Proton started this DLL's FSR 4.1. Either they are too old (GE-Proton 10+, Proton "
                             "Experimental and Proton-CachyOS 11 work), or the DLL does not enable FSR 4.1 on this GPU "
                             "(Steam Deck?): then build on a PC with a Radeon RX 7000/9000 and copy the fsr4_411 "
                             "folder. Details in the log");
        case EXIT_REPLAY_MISMATCH:
            return translate("The recorded passes do not match what bbpc replays (another DLL version, or a new "
                             "GPU?): details in the log");
        case EXIT_NO_LOADER:
            return translate("No FidelityFX loader 2.x found");
        case EXIT_NO_RECORDER:
            return translate("The recording could start neither through umu-launcher nor through your systemd user "
                             "session: install umu-launcher (Arch: sudo pacman -S umu-launcher). Details in the log");
        default:
            return translate("The build failed (exit code %1): details in the log").arg(exitCode);
    }
}

QString Fsr411Build::progress(const QString& line)
{
    const QRegularExpressionMatch match = STEP.match(line);
    if (match.hasMatch())
    {
        return translate("Recording the DLL's passes under Proton: %1 of %2")
            .arg(match.captured(STEP_CURRENT_GROUP), match.captured(STEP_COUNT_GROUP));
    }
    if (line.startsWith(QStringLiteral("Translating")))
    {
        return translate("Translating the passes to SPIR-V...");
    }
    return {};
}
