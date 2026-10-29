#include "choices.h"

#include <QCoreApplication>
#include <QStringList>

namespace
{
constexpr const char* CONTEXT = "Choices";

QString translate(const char* text)
{
    return text == nullptr ? QString() : QCoreApplication::translate(CONTEXT, text);
}
} // namespace

QString Choices::label(const Choice& choice)
{
    return translate(choice.label);
}

QString Choices::note(const Choice& choice)
{
    return translate(choice.note);
}

int Choices::indexOf(const QList<Choice>& choices, const QString& value)
{
    for (int i = 0; i < choices.size(); ++i)
    {
        if (QString::fromLatin1(choices.at(i).value) == value)
        {
            return i;
        }
    }
    return 0;
}

const QList<Choices::Choice>& Choices::upscalers()
{
    static const QList<Choice> list = {
        { "FSR 4", "fsr4", QT_TRANSLATE_NOOP("Choices", "The best quality on AMD RDNA 3/4; slower on other GPUs") },
        { "FSR 4.1.1", "fsr411", QT_TRANSLATE_NOOP("Choices", "AMD's newer model (needs your own AMD DLL, see below)") },
        { "FSR 3.1", "fsr3", QT_TRANSLATE_NOOP("Choices", "A temporal upscaler for any GPU") },
        { QT_TRANSLATE_NOOP("Choices", "FSR 1 (weak GPUs)"), "fsr1",
          QT_TRANSLATE_NOOP("Choices", "Spatial: the cheapest (about 1 ms), no frame history; the game's post-processing "
                                       "runs at the scene size. For laptops and the Steam Deck") },
        { "DLSS (NVIDIA RTX)", "dlss", QT_TRANSLATE_NOOP("Choices", "Needs NVIDIA's library (see below)") },
        { QT_TRANSLATE_NOOP("Choices", "TAA (native anti-aliasing)"), "taa",
          QT_TRANSLATE_NOOP("Choices", "Anti-aliasing without a smaller resolution") },
        { QT_TRANSLATE_NOOP("Choices", "Off"), "off", QT_TRANSLATE_NOOP("Choices", "The scene at the output resolution") },
    };
    return list;
}

const QList<Choices::Choice>& Choices::presets()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Native AA"), "0" },
        { QT_TRANSLATE_NOOP("Choices", "Quality (x1.5)"), "1" },
        { QT_TRANSLATE_NOOP("Choices", "Balanced (x1.7)"), "2" },
        { QT_TRANSLATE_NOOP("Choices", "Performance (x2)"), "3" },
        { QT_TRANSLATE_NOOP("Choices", "Ultra Performance (x3)"), "4" },
    };
    return list;
}

const QList<Choices::Choice>& Choices::frameGenerations()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Off"), "off" },
        { QT_TRANSLATE_NOOP("Choices", "FSR 3.1 (experimental)"), "fsr3",
          QT_TRANSLATE_NOOP("Choices", "Any GPU; artifacts at the edges are possible") },
        { "DLSS 2x (NVIDIA RTX 40/50)", "dlss2" },
        { "DLSS 3x (NVIDIA RTX 50)", "dlss3" },
        { "DLSS 4x (NVIDIA RTX 50)", "dlss4" },
    };
    return list;
}

// NVIDIA Reflex (VK_NV_low_latency2) or AMD Anti-Lag 2 (VK_AMD_anti_lag): render/vk_low_latency.
const QList<Choices::Choice>& Choices::lowLatency()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "On"), "on" },
        { QT_TRANSLATE_NOOP("Choices", "On + Boost"), "boost" },
        { QT_TRANSLATE_NOOP("Choices", "Off"), "off" },
    };
    return list;
}

// bbport.ini anti_aliasing (BbSettings::AntiAliasing); the game's own AA switch follows it.
const QList<Choices::Choice>& Choices::antiAliasing()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Auto"), "auto",
          QT_TRANSLATE_NOOP("Choices", "The game's AA without an upscaler; off with one (it only blurs its input)") },
        { QT_TRANSLATE_NOOP("Choices", "The game's own"), "game" },
        { "FXAA2_GS", "fxaa", QT_TRANSLATE_NOOP("Choices", "The game's AA step running FXAA2_GS; applied at once") },
        { QT_TRANSLATE_NOOP("Choices", "SMAA (no upscaler)"), "smaa",
          QT_TRANSLATE_NOOP("Choices", "SMAA 1x on the finished scene; not with an upscaler") },
        { QT_TRANSLATE_NOOP("Choices", "Off"), "off" },
    };
    return list;
}

// bbport.ini texture_quality (BbSettings::TextureQuality): large textures in VRAM.
const QList<Choices::Choice>& Choices::textureQuality()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Auto"), "auto", QT_TRANSLATE_NOOP("Choices", "Half with 3 GB of VRAM or less, else full") },
        { QT_TRANSLATE_NOOP("Choices", "Full"), "full" },
        { QT_TRANSLATE_NOOP("Choices", "Half"), "half",
          QT_TRANSLATE_NOOP("Choices", "Large textures half as big per side: 25-35% less image memory") },
        { QT_TRANSLATE_NOOP("Choices", "Quarter"), "quarter",
          QT_TRANSLATE_NOOP("Choices", "A quarter per side: even less memory, visibly blurrier up close") },
    };
    return list;
}

// The PS4 system language codes the game knows (its text in dvdroot_ps4/msg).
const QList<Choices::Choice>& Choices::systemLanguages()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "English"), "1" },
        { QT_TRANSLATE_NOOP("Choices", "English (UK)"), "18" },
        { QT_TRANSLATE_NOOP("Choices", "Russian"), "8" },
        { QT_TRANSLATE_NOOP("Choices", "Japanese"), "0" },
        { QT_TRANSLATE_NOOP("Choices", "French"), "2" },
        { QT_TRANSLATE_NOOP("Choices", "Spanish"), "3" },
        { QT_TRANSLATE_NOOP("Choices", "Spanish (Latin America)"), "20" },
        { QT_TRANSLATE_NOOP("Choices", "German"), "4" },
        { QT_TRANSLATE_NOOP("Choices", "Italian"), "5" },
        { QT_TRANSLATE_NOOP("Choices", "Dutch"), "6" },
        { QT_TRANSLATE_NOOP("Choices", "Portuguese"), "7" },
        { QT_TRANSLATE_NOOP("Choices", "Portuguese (Brazil)"), "17" },
        { QT_TRANSLATE_NOOP("Choices", "Polish"), "16" },
        { QT_TRANSLATE_NOOP("Choices", "Turkish"), "19" },
        { QT_TRANSLATE_NOOP("Choices", "Korean"), "9" },
        { QT_TRANSLATE_NOOP("Choices", "Chinese (Simplified)"), "11" },
        { QT_TRANSLATE_NOOP("Choices", "Chinese (Traditional)"), "10" },
        { QT_TRANSLATE_NOOP("Choices", "Finnish"), "12" },
        { QT_TRANSLATE_NOOP("Choices", "Swedish"), "13" },
        { QT_TRANSLATE_NOOP("Choices", "Danish"), "14" },
        { QT_TRANSLATE_NOOP("Choices", "Norwegian"), "15" },
    };
    return list;
}

// Any language the game's files have, whatever the copy's sell region and the save chose
// (patches.py language_writes, issue #269).
const QList<Choices::Choice>& Choices::gameTextLanguages()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "As the game chooses"), "auto" },
        { QT_TRANSLATE_NOOP("Choices", "English (US)"), "enus" },
        { QT_TRANSLATE_NOOP("Choices", "English (UK)"), "engb" },
        { QT_TRANSLATE_NOOP("Choices", "Russian"), "ruru" },
        { QT_TRANSLATE_NOOP("Choices", "Japanese"), "jajp" },
        { QT_TRANSLATE_NOOP("Choices", "French"), "frfr" },
        { QT_TRANSLATE_NOOP("Choices", "Italian"), "itit" },
        { QT_TRANSLATE_NOOP("Choices", "German"), "dede" },
        { QT_TRANSLATE_NOOP("Choices", "Spanish"), "eses" },
        { QT_TRANSLATE_NOOP("Choices", "Spanish (Latin America)"), "esar" },
        { QT_TRANSLATE_NOOP("Choices", "Portuguese (Brazil)"), "ptbr" },
        { QT_TRANSLATE_NOOP("Choices", "Portuguese"), "ptpt" },
        { QT_TRANSLATE_NOOP("Choices", "Dutch"), "nlnl" },
        { QT_TRANSLATE_NOOP("Choices", "Polish"), "plpl" },
        { QT_TRANSLATE_NOOP("Choices", "Danish"), "dadk" },
        { QT_TRANSLATE_NOOP("Choices", "Norwegian"), "nono" },
        { QT_TRANSLATE_NOOP("Choices", "Finnish"), "fifi" },
        { QT_TRANSLATE_NOOP("Choices", "Swedish"), "svse" },
        { QT_TRANSLATE_NOOP("Choices", "Turkish"), "trtr" },
        { QT_TRANSLATE_NOOP("Choices", "Chinese (Simplified)"), "zhcn" },
        { QT_TRANSLATE_NOOP("Choices", "Chinese (Traditional)"), "zhtw" },
        { QT_TRANSLATE_NOOP("Choices", "Korean"), "kokr" },
    };
    return list;
}

// The languages with voice files (sound/*.fsb): the first eight of patches.py GAME_LANGUAGES.
const QList<Choices::Choice>& Choices::gameVoiceLanguages()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "As the game chooses"), "auto" },
        { QT_TRANSLATE_NOOP("Choices", "English"), "engb" },
        { QT_TRANSLATE_NOOP("Choices", "Japanese"), "jajp" },
        { QT_TRANSLATE_NOOP("Choices", "French"), "frfr" },
        { QT_TRANSLATE_NOOP("Choices", "Italian"), "itit" },
        { QT_TRANSLATE_NOOP("Choices", "German"), "dede" },
        { QT_TRANSLATE_NOOP("Choices", "Spanish"), "eses" },
        { QT_TRANSLATE_NOOP("Choices", "Spanish (Latin America)"), "esar" },
        { QT_TRANSLATE_NOOP("Choices", "Portuguese (Brazil)"), "ptbr" },
    };
    return list;
}

const QList<Choices::Choice>& Choices::presentModes()
{
    static const QList<Choice> list = {
        { "Mailbox", "Mailbox" },
        { "FIFO (VSync)", "Fifo" },
        { "FIFO Relaxed", "FifoRelaxed" },
        { "Immediate", "Immediate" },
    };
    return list;
}

const QList<Choices::Choice>& Choices::modelDetail()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "As in the game"), "0" },
        { QT_TRANSLATE_NOOP("Choices", "Highest (-2)"), "-2" },
        { QT_TRANSLATE_NOOP("Choices", "Lower (1)"), "1" },
        { QT_TRANSLATE_NOOP("Choices", "Lowest (2)"), "2" },
    };
    return list;
}

const QList<Choices::Choice>& Choices::fpsModes()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Unlocked"), "uncap" },
        { "60", "60" },
        { "90", "90" },
        { QT_TRANSLATE_NOOP("Choices", "30 (as on PS4)"), "30" },
    };
    return list;
}

const QList<Choices::Choice>& Choices::drawPipe()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Auto"), "", QT_TRANSLATE_NOOP("Choices", "On with 8 or more CPU threads") },
        { QT_TRANSLATE_NOOP("Choices", "On"), "1" },
        { QT_TRANSLATE_NOOP("Choices", "Off"), "0", QT_TRANSLATE_NOOP("Choices", "More stable, but slower") },
    };
    return list;
}

// Issue #299 (runtime_pad/bindings.c button_prompts): the button icons of one device, whatever
// is used.
const QList<Choices::Choice>& Choices::buttonPrompts()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Auto (the last device used)"), "auto" },
        { "PlayStation", "playstation" },
        { "Xbox", "xbox" },
        { QT_TRANSLATE_NOOP("Choices", "Keyboard and mouse"), "keyboard" },
    };
    return list;
}

const QList<Choices::Choice>& Choices::onlinePools()
{
    static const QList<Choice> list = {
        { "Seamless Hunt (seamlesshunt.com)", "seamless",
          QT_TRANSLATE_NOOP("Choices", "BBSC server: seamless co-op, bells and summons; the account is made on the website") },
        { "The Hunter's Dream (thehuntersdream.com)", "huntersdream",
          QT_TRANSLATE_NOOP("Choices", "Co-op with bbhost players through The Hunter's Dream; sign in with Discord") },
    };
    return list;
}

const QList<Choices::Choice>& Choices::themes()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "System"), "system" },
        { QT_TRANSLATE_NOOP("Choices", "Dark"), "dark" },
        { QT_TRANSLATE_NOOP("Choices", "Light"), "light" },
    };
    return list;
}

// The game's own anti-aliasing (effect_game_aa) is no longer one of them: anti_aliasing sets it.
const QList<Choices::Effect>& Choices::effects()
{
    static const QList<Effect> list = {
        { "effect_chromatic_aberration", QT_TRANSLATE_NOOP("Choices", "Chromatic aberration"), true,
          QT_TRANSLATE_NOOP("Choices", "Color fringes at the frame's edges") },
        { "effect_dof", QT_TRANSLATE_NOOP("Choices", "Depth of field (DoF)"), true,
          QT_TRANSLATE_NOOP("Choices", "Blur of distant and near objects") },
        { "effect_motion_blur", QT_TRANSLATE_NOOP("Choices", "Motion blur"), false,
          QT_TRANSLATE_NOOP("Choices", "Blur when the camera turns fast") },
        { "effect_ssao", QT_TRANSLATE_NOOP("Choices", "Ambient occlusion (SSAO)"), true,
          QT_TRANSLATE_NOOP("Choices", "Shading in corners and creases") },
        { "effect_dynamic_shadows", QT_TRANSLATE_NOOP("Choices", "Dynamic light shadows"), true,
          QT_TRANSLATE_NOOP("Choices", "Shadows of torches and lanterns. Turning them off helps weak GPUs noticeably") },
        { "effect_ssr", QT_TRANSLATE_NOOP("Choices", "SSR reflections (not in the original game)"), false,
          QT_TRANSLATE_NOOP("Choices", "Screen-space reflections") },
        { "effect_bloom", QT_TRANSLATE_NOOP("Choices", "Bloom"), true, QT_TRANSLATE_NOOP("Choices", "A glow around bright lights") },
        { "skip_intro", QT_TRANSLATE_NOOP("Choices", "Skip the intro videos"), false,
          QT_TRANSLATE_NOOP("Choices", "Straight to the main menu") },
        { "debug_camera", QT_TRANSLATE_NOOP("Choices", "Free camera"), false,
          QT_TRANSLATE_NOOP("Choices", "Cross + L3 (F + Left Ctrl on the keyboard)") },
        { "debug_menu", QT_TRANSLATE_NOOP("Choices", "Debug menu (left touchpad / Tab; needs fonts)"), false,
          QT_TRANSLATE_NOOP("Choices", "Needs the adhoc folder from Nexus mod #253 (fonts in adhoc/font), in the game's "
                                       "dvdroot_ps4 or as a mod; without it the patch is not applied") },
    };
    return list;
}

QString Choices::title(const Effect& effect)
{
    return translate(effect.title);
}

QString Choices::note(const Effect& effect)
{
    return translate(effect.note);
}

const QList<Choices::Control>& Choices::controls()
{
    static const QList<Control> list = {
        { "cross", QT_TRANSLATE_NOOP("Choices", "Cross (interact)"), "F, Return", "a" },
        { "circle", QT_TRANSLATE_NOOP("Choices", "Circle (dodge and run; on the keyboard: Space and Left Shift below)"), "", "b" },
        { "dodge", QT_TRANSLATE_NOOP("Choices", "Dodge (a short press of Circle; Shift+Space jumps)"), "Space", nullptr },
        { "sprint", QT_TRANSLATE_NOOP("Choices", "Run (Circle held)"), "Left Shift", nullptr },
        { "walk", QT_TRANSLATE_NOOP("Choices", "Slow walk (held)"), "Left Alt", nullptr },
        { "square", QT_TRANSLATE_NOOP("Choices", "Square (quick item)"), "Tab, R", "x" },
        { "triangle", QT_TRANSLATE_NOOP("Choices", "Triangle (blood vial)"), "Q", "y" },
        { "l1", QT_TRANSLATE_NOOP("Choices", "L1 (transform the weapon)"), "E, Mouse X2", "leftshoulder" },
        { "r1", QT_TRANSLATE_NOOP("Choices", "R1 (attack)"), "Mouse Left", "rightshoulder" },
        { "l2", QT_TRANSLATE_NOOP("Choices", "L2 (shoot)"), "Mouse Right", "lefttrigger" },
        { "r2", QT_TRANSLATE_NOOP("Choices", "R2 (strong attack)"), "Left Shift+Mouse Left, Mouse X1", "righttrigger" },
        { "l3", "L3", "Left Ctrl", "leftstick" },
        { "r3", QT_TRANSLATE_NOOP("Choices", "R3 (lock on)"), "C, Mouse Middle", "rightstick" },
        { "options", QT_TRANSLATE_NOOP("Choices", "Options (menu; in the game's menus Escape goes back, as Circle)"), "Escape", "start" },
        { "touchpad", QT_TRANSLATE_NOOP("Choices", "Touchpad, left half (gestures)"), "G", "back, touchpad" },
        { "touchpad_right", QT_TRANSLATE_NOOP("Choices", "Touchpad, right half (key items)"), "Backspace", "" },
        { "up", QT_TRANSLATE_NOOP("Choices", "D-pad up"), "3, Up, Wheel Up", "dpup" },
        { "down", QT_TRANSLATE_NOOP("Choices", "D-pad down"), "4, Down, Wheel Down", "dpdown" },
        { "left", QT_TRANSLATE_NOOP("Choices", "D-pad left (left-hand weapon)"), "1, Left", "dpleft" },
        { "right", QT_TRANSLATE_NOOP("Choices", "D-pad right (right-hand weapon)"), "2, Right", "dpright" },
        { "move_up", QT_TRANSLATE_NOOP("Choices", "Move forward"), "W", nullptr },
        { "move_down", QT_TRANSLATE_NOOP("Choices", "Move back"), "S", nullptr },
        { "move_left", QT_TRANSLATE_NOOP("Choices", "Move left"), "A", nullptr },
        { "move_right", QT_TRANSLATE_NOOP("Choices", "Move right"), "D", nullptr },
        { "look_up", QT_TRANSLATE_NOOP("Choices", "Camera up"), "I", nullptr },
        { "look_down", QT_TRANSLATE_NOOP("Choices", "Camera down"), "K", nullptr },
        { "look_left", QT_TRANSLATE_NOOP("Choices", "Camera left"), "J", nullptr },
        { "look_right", QT_TRANSLATE_NOOP("Choices", "Camera right"), "L", nullptr },
    };
    return list;
}

QString Choices::label(const Control& control)
{
    return translate(control.label);
}

QString Choices::addBinding(const QString& current, const QString& input)
{
    QStringList keys;
    for (const QString& key : current.split(u','))
    {
        const QString trimmed = key.trimmed();
        if (trimmed.isEmpty() == false)
        {
            keys.append(trimmed);
        }
    }
    const QString added = input.trimmed();
    if (added.isEmpty() == false && keys.contains(added) == false)
    {
        keys.append(added);
    }
    while (keys.size() > MAX_KEYS_PER_ACTION)
    {
        keys.removeFirst();
    }
    return keys.join(QStringLiteral(", "));
}
