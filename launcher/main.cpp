#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QProcess>
#include <QStyleFactory>
#include <QSysInfo>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <vector>

#include "core/appConfig.h"
#include "core/capabilities.h"
#include "core/fsr411Build.h"
#include "core/gameEnvironment.h"
#include "core/hostEnvironment.h"
#include "core/platformUtils.h"
#include "core/portPaths.h"
#include "debug.h"
#include "mainWindow.h"
#include "themeManager.h"

namespace
{
constexpr int MAIN_WINDOW_WIDTH = 1040;
constexpr int MAIN_WINDOW_HEIGHT = 780;
constexpr int GPU_QUERY_TIMEOUT_MS = 10000;
constexpr int START_TIMEOUT_MS = 10000;
constexpr int EXIT_USAGE = 2;
constexpr int MAX_FSR411_ARGUMENTS = 2;
constexpr int NO_TIMEOUT = -1;
constexpr int EXIT_EXEC_FAILED = 127;
// argv[0] is the program itself.
constexpr int FIRST_ARGUMENT = 1;
// --build-fsr411 <upscaler DLL> [loader DLL].
constexpr qsizetype UPSCALER_DLL_ARGUMENT = 0;
constexpr qsizetype LOADER_DLL_ARGUMENT = 1;
const QString PLAY = QStringLiteral("--play");
const QString BUILD_FSR411 = QStringLiteral("--build-fsr411");
const QString VERSION = QStringLiteral("--version");
const QString HELP = QStringLiteral("--help");

void setIdentity()
{
    QCoreApplication::setApplicationName(QStringLiteral("bbpc"));
    QCoreApplication::setOrganizationName(QStringLiteral("wheat32"));
    // version.json is the single source of truth, embedded as a resource.
    QString version = QStringLiteral("unknown");
    QFile file(QStringLiteral(":/version.json"));
    if (file.open(QIODevice::ReadOnly))
    {
        const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
        version = json.value(QStringLiteral("app_version")).toString(version);
    }
    QCoreApplication::setApplicationVersion(version);
}

void printUsage()
{
    std::printf("usage: bbpc [--play | --build-fsr411 <amd_fidelityfx_upscaler_dx12.dll> [loader DLL] | --version]\n"
                "  (no option)    the launcher window\n"
                "  --play         the game with the launcher's saved settings, no window (Steam's game mode)\n"
                "  --build-fsr411 FSR 4.1.1 from your own AMD DLL, as the launcher's button builds it\n");
}

GameEnvironment::Location location()
{
    GameEnvironment::Location where;
    where.dataDir = PortPaths::dataDir();
    where.packaged = PortPaths::packaged();
    where.buildDir = PortPaths::buildDir();
    where.onlineModule = QFileInfo(PortPaths::binariesDir() + QStringLiteral("/gpu/libbbnet20.so")).isFile();
    return where;
}

// --play: replaces this process with run.sh, so that Steam sees the game itself.
int play()
{
    const LauncherSettings& settings = AppConfig::instance().settings();
    if (QFileInfo(GameEnvironment::expandHome(settings.gameDir) + QStringLiteral("/eboot.bin")).isFile() == false)
    {
        std::fprintf(stderr, "bbpc: choose the game folder in the launcher first\n");
        return EXIT_FAILURE;
    }
    QList<Capabilities::Gpu> gpus;
    if (settings.gpu.isEmpty() == false)
    {
        QProcess query;
        query.start(PortPaths::capabilitiesTool(), { QStringLiteral("--gpus") });
        if (query.waitForFinished(GPU_QUERY_TIMEOUT_MS))
        {
            gpus = Capabilities::parseGpus(QString::fromUtf8(query.readAllStandardOutput()));
        }
    }
    const QStringList env = GameEnvironment::build(settings, HostEnvironment::host(), location(), gpus).toStringList();
    QDir().mkpath(PortPaths::dataDir());
    if (::chdir(QFile::encodeName(PortPaths::portDir()).constData()) != 0)
    {
        std::perror("bbpc: the port's folder");
        return EXIT_FAILURE;
    }
    std::vector<QByteArray> storage;
    storage.reserve(static_cast<size_t>(env.size()));
    std::vector<char*> envp;
    for (const QString& entry : env)
    {
        storage.push_back(entry.toLocal8Bit());
        envp.push_back(storage.back().data());
    }
    envp.push_back(nullptr);
    QByteArray runScript = QFile::encodeName(PortPaths::portDir() + QStringLiteral("/run.sh"));
    char bash[] = "bash";
    char* argv[] = { bash, runScript.data(), nullptr };
    ::execvpe(bash, argv, envp.data());
    std::perror("bbpc: bash run.sh");
    return EXIT_EXEC_FAILED;
}

// --build-fsr411 <upscaler DLL> [loader DLL]: the launcher's FSR 4.1.1 build without a window.
int buildFsr411(const QStringList& arguments)
{
    if (arguments.isEmpty() || arguments.size() > MAX_FSR411_ARGUMENTS)
    {
        std::fprintf(stderr, "usage: bbpc --build-fsr411 <amd_fidelityfx_upscaler_dx12.dll> [loader DLL]\n");
        return EXIT_USAGE;
    }
    const Fsr411Build::Command command = Fsr411Build::command(arguments.at(UPSCALER_DLL_ARGUMENT),
                                                              arguments.value(LOADER_DLL_ARGUMENT));
    QProcess build;
    build.setProcessChannelMode(QProcess::ForwardedChannels);
    build.setProcessEnvironment(command.env);
    build.setWorkingDirectory(command.workingDirectory);
    build.start(command.program, command.arguments);
    if (build.waitForStarted(START_TIMEOUT_MS) == false)
    {
        std::fprintf(stderr, "bbpc: %s\n", qPrintable(build.errorString()));
        return EXIT_EXEC_FAILED;
    }
    build.waitForFinished(NO_TIMEOUT);
    return build.exitStatus() == QProcess::NormalExit ? build.exitCode() : EXIT_FAILURE;
}

int commandLine(const QStringList& arguments)
{
    if (arguments.contains(VERSION))
    {
        std::printf("bbpc %s\n", qPrintable(QCoreApplication::applicationVersion()));
        return EXIT_SUCCESS;
    }
    if (arguments.contains(HELP))
    {
        printUsage();
        return EXIT_SUCCESS;
    }
    if (PortPaths::portDir().isEmpty())
    {
        std::fprintf(stderr, "bbpc: the port's files (run.sh, scripts/) were not found; set BBPC_PORT_DIR\n");
        return EXIT_FAILURE;
    }
    if (arguments.contains(PLAY))
    {
        return play();
    }
    const qsizetype build = arguments.indexOf(BUILD_FSR411);
    return buildFsr411(arguments.mid(build + 1));
}

bool wantsCommandLine(int argc, char* argv[])
{
    for (int i = FIRST_ARGUMENT; i < argc; ++i)
    {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (argument == PLAY || argument == BUILD_FSR411 || argument == VERSION || argument == HELP) return true;
    }
    return false;
}
} // namespace

int main(int argc, char* argv[])
{
    if (wantsCommandLine(argc, argv))
    {
        QCoreApplication app(argc, argv);
        setIdentity();
        return commandLine(QCoreApplication::arguments());
    }

    QApplication app(argc, argv);
    setIdentity();
    QApplication::setApplicationDisplayName(QStringLiteral("bbpc"));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.wheat32.bbpc"));
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/app-icon.svg")));

    //  Startup diagnostics
    DBG_APP(QStringLiteral("=== bbpc launcher starting ==="));
    DBG_APP(QStringLiteral("App version        : ") + QCoreApplication::applicationVersion());
    DBG_APP(QStringLiteral("Qt version         : ") + QString::fromLatin1(qVersion()));
    DBG_APP(QStringLiteral("Package type       : ") + packageTypeName());
    DBG_APP(QStringLiteral("OS                 : ") + QSysInfo::prettyProductName());
    DBG_APP(QStringLiteral("Kernel             : ") + QSysInfo::kernelVersion());
    DBG_APP(QStringLiteral("CPU arch           : ") + QSysInfo::currentCpuArchitecture());
    DBG_APP(QStringLiteral("Locale             : ") + QLocale::system().name());
    DBG_APP(QStringLiteral("Config file        : ") + AppConfig::configFile());
    DBG_APP(QStringLiteral("Port dir           : ") + (PortPaths::portDir().isEmpty() ? QStringLiteral("not found") : PortPaths::portDir()));
    DBG_APP(QStringLiteral("Binaries dir       : ") + PortPaths::binariesDir());
    DBG_APP(QStringLiteral("Data dir           : ") + PortPaths::dataDir());
    DBG_APP(QStringLiteral("================================="));

    ThemeManager::apply(AppConfig::instance().settings().theme);
    MainWindow window;
    window.resize(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    window.show();
    return QApplication::exec();
}
