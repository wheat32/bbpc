#include "choices.h"

#include <QCoreApplication>

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
        { "FSR 4", "fsr4" },
        { "FSR 4.1.1", "fsr411" },
        { "FSR 3", "fsr3" },
        { "DLSS (NVIDIA RTX)", "dlss" },
        { QT_TRANSLATE_NOOP("Choices", "TAA (native anti-aliasing)"), "taa" },
        { QT_TRANSLATE_NOOP("Choices", "Off"), "off" },
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

const QList<Choices::Choice>& Choices::memoryModels()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Auto: new on every GPU"), "auto" },
        { QT_TRANSLATE_NOOP("Choices", "New"), "new" },
        { QT_TRANSLATE_NOOP("Choices", "Old (as in 0.3)"), "old" },
    };
    return list;
}

// The game's text languages (dvdroot_ps4/msg) by the PS4 system language code.
const QList<Choices::Choice>& Choices::gameLanguages()
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

const QList<Choices::Choice>& Choices::liveResolution()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Auto (by GPU)"), "auto" },
        { QT_TRANSLATE_NOOP("Choices", "Off (faster)"), "0" },
        { QT_TRANSLATE_NOOP("Choices", "On"), "1" },
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

const QList<Choices::Choice>& Choices::readbacks()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Relaxed"), "", QT_TRANSLATE_NOOP("Choices", "The default") },
        { QT_TRANSLATE_NOOP("Choices", "Off"), "0" },
        { QT_TRANSLATE_NOOP("Choices", "Precise"), "2" },
    };
    return list;
}

// Background pre-upload of the game's GPU memory into VRAM (BB_PREUPLOAD).
const QList<Choices::Choice>& Choices::preupload()
{
    static const QList<Choice> list = {
        { QT_TRANSLATE_NOOP("Choices", "Normal"), "", QT_TRANSLATE_NOOP("Choices", "No extra VRAM") },
        { QT_TRANSLATE_NOOP("Choices", "Full"), "2", QT_TRANSLATE_NOOP("Choices", "About 3 GB more VRAM") },
        { QT_TRANSLATE_NOOP("Choices", "Off"), "0" },
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

const QList<Choices::Effect>& Choices::effects()
{
    static const QList<Effect> list = {
        { "effect_chromatic_aberration", QT_TRANSLATE_NOOP("Choices", "Chromatic aberration"), true },
        { "effect_dof", QT_TRANSLATE_NOOP("Choices", "Depth of field (DoF)"), true },
        { "effect_motion_blur", QT_TRANSLATE_NOOP("Choices", "Motion blur"), true },
        { "effect_ssao", "SSAO", true },
        { "effect_game_aa", QT_TRANSLATE_NOOP("Choices", "The game's own anti-aliasing"), true },
        { "effect_dynamic_shadows", QT_TRANSLATE_NOOP("Choices", "Dynamic light shadows"), true },
        { "effect_ssr", QT_TRANSLATE_NOOP("Choices", "SSR reflections (not in the original game)"), false },
        { "effect_bloom", QT_TRANSLATE_NOOP("Choices", "Bloom"), true },
        { "skip_intro", QT_TRANSLATE_NOOP("Choices", "Skip the intro videos"), false },
        { "debug_camera", QT_TRANSLATE_NOOP("Choices", "Free camera (Cross + L3 / F + Left Ctrl)"), false },
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
