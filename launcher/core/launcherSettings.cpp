#include "launcherSettings.h"

namespace
{
// One JSON key bound to one member, for the reading and the writing in one place.
template <typename Visitor>
void visitFields(LauncherSettings& s, Visitor&& visit)
{
    visit(QStringLiteral("theme"), s.theme);
    visit(QStringLiteral("game_dir"), s.gameDir);
    visit(QStringLiteral("user_dir"), s.userDir);
    visit(QStringLiteral("language"), s.language);
    visit(QStringLiteral("memory_model"), s.memoryModel);
    visit(QStringLiteral("gpu"), s.gpu);
    visit(QStringLiteral("mods_dir"), s.modsDir);
    visit(QStringLiteral("mods_enabled"), s.modsEnabled);
    visit(QStringLiteral("patches_dir"), s.patchesDir);
    visit(QStringLiteral("fullscreen"), s.fullscreen);
    visit(QStringLiteral("hdr"), s.hdr);
    visit(QStringLiteral("present_mode"), s.presentMode);
    visit(QStringLiteral("display"), s.display);
    visit(QStringLiteral("gamepad"), s.gamepad);
    visit(QStringLiteral("gamepad_name"), s.gamepadName);
    visit(QStringLiteral("skip_network_choice"), s.skipNetworkChoice);
    visit(QStringLiteral("fps_mode"), s.fpsMode);
    visit(QStringLiteral("fps_limit"), s.fpsLimit);
    visit(QStringLiteral("draw_pipe"), s.drawPipe);
    visit(QStringLiteral("readbacks"), s.readbacks);
    visit(QStringLiteral("preupload"), s.preupload);
    visit(QStringLiteral("mangohud"), s.mangohud);
    visit(QStringLiteral("frame_stats"), s.frameStats);
    visit(QStringLiteral("save_log"), s.saveLog);
    visit(QStringLiteral("crash_diag"), s.crashDiag);
    visit(QStringLiteral("as_0_3"), s.as03);
    visit(QStringLiteral("gpu_profile"), s.gpuProfile);
    visit(QStringLiteral("vk_validation"), s.vkValidation);
    visit(QStringLiteral("extra_env"), s.extraEnv);
    visit(QStringLiteral("online"), s.online);
    visit(QStringLiteral("online_bbsc"), s.onlineBbsc);
    visit(QStringLiteral("online_community"), s.onlineCommunity);
    visit(QStringLiteral("online_server"), s.onlineServer);
    visit(QStringLiteral("online_webapi"), s.onlineWebapi);
    visit(QStringLiteral("online_npid"), s.onlineNpid);
    visit(QStringLiteral("online_password"), s.onlinePassword);
    visit(QStringLiteral("online_upnp"), s.onlineUpnp);
    visit(QStringLiteral("online_pool"), s.onlinePool);
    visit(QStringLiteral("thd_server"), s.thdServer);
    visit(QStringLiteral("thd_online_id"), s.thdOnlineId);
    visit(QStringLiteral("thd_token"), s.thdToken);
    visit(QStringLiteral("fsr411_dll_dir"), s.fsr411DllDir);
}

void readField(const QJsonObject& json, const QString& key, QString& field)
{
    const QJsonValue value = json.value(key);
    if (value.isString())
    {
        field = value.toString();
    }
}

void readField(const QJsonObject& json, const QString& key, bool& field)
{
    const QJsonValue value = json.value(key);
    if (value.isBool())
    {
        field = value.toBool();
    }
}

void readField(const QJsonObject& json, const QString& key, int& field)
{
    const QJsonValue value = json.value(key);
    if (value.isDouble())
    {
        field = value.toInt();
    }
}
} // namespace

LauncherSettings LauncherSettings::fromJson(const QJsonObject& json)
{
    return fromJson(json, LauncherSettings());
}

LauncherSettings LauncherSettings::fromJson(const QJsonObject& json, const LauncherSettings& defaults)
{
    LauncherSettings settings = defaults;
    QJsonObject migrated = json;
    OnlineAddresses::migrateOldDefaults(migrated);
    visitFields(settings, [&migrated](const QString& key, auto& field)
    {
        readField(migrated, key, field);
    });
    settings.saved = migrated;
    return settings;
}

QJsonObject LauncherSettings::toJson() const
{
    QJsonObject json = saved;
    LauncherSettings copy = *this;
    visitFields(copy, [&json](const QString& key, const auto& field)
    {
        json.insert(key, field);
    });
    return json;
}

QString LauncherSettings::pool() const
{
    return onlinePool == QStringLiteral("huntersdream") ? onlinePool : QStringLiteral("seamless");
}
