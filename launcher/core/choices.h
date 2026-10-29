#pragma once

#include <QList>
#include <QString>

// Choices - the values the launcher offers for the port's settings. Labels and notes are marked
// for translation (context "Choices") and translated when shown: label() and note().
namespace Choices
{
    // One entry of a drop-down: its label, the value stored, and an optional note shown under the
    // row's title while it is selected (the labels stay short, so the selected one is explained).
    struct Choice
    {
        const char* label;
        const char* value;
        const char* note = nullptr;
    };

    QString label(const Choice& choice);
    QString note(const Choice& choice);

    // The index of value in choices, or 0 (the first entry is the default).
    int indexOf(const QList<Choice>& choices, const QString& value);

    const QList<Choice>& upscalers();
    const QList<Choice>& presets();
    // Frame generation as the launcher offers it (GraphicsSettings maps it to bbport.ini's
    // frame_generation and frame_generation_multiplier); the DLSS entries start with "dlss".
    const QList<Choice>& frameGenerations();
    const QList<Choice>& lowLatency();
    const QList<Choice>& antiAliasing();
    const QList<Choice>& textureQuality();
    // The PS4 system language the game sees (BB_LANGUAGE), by its number.
    const QList<Choice>& systemLanguages();
    // bbport.ini game_text_language / game_voice_language: the game's own codes (patches.py).
    const QList<Choice>& gameTextLanguages();
    const QList<Choice>& gameVoiceLanguages();
    const QList<Choice>& presentModes();
    const QList<Choice>& modelDetail();
    const QList<Choice>& fpsModes();
    const QList<Choice>& drawPipe();
    const QList<Choice>& buttonPrompts();
    const QList<Choice>& onlinePools();
    const QList<Choice>& themes();

    // A game effect: a patch applied at start, switched in bbport.ini.
    struct Effect
    {
        const char* key;
        const char* title;
        bool defaultOn;
        const char* note = nullptr;
    };

    const QList<Effect>& effects();
    QString title(const Effect& effect);
    QString note(const Effect& effect);

    // One input of the game (runtime_pad.c): bbport.ini key.<name>= / pad.<name>= replace the
    // default; no line keeps it. Keyboard defaults are Dark Souls III-like ("Left Shift+Mouse
    // Left" is a chord); gamepad buttons use SDL's names. padDefault nullptr: no gamepad binding.
    // A binding lists up to MAX_KEYS_PER_ACTION keys or buttons, separated by commas.
    struct Control
    {
        const char* name;
        const char* label;
        const char* keyDefault;
        const char* padDefault;
    };

    const QList<Control>& controls();
    QString label(const Control& control);

    // The keys or buttons one action takes at most (runtime_pad/bindings.c MAX_BIND).
    inline constexpr int MAX_KEYS_PER_ACTION = 4;

    // The binding current ("E, Mouse X2") with input added: kept once, the oldest dropped beyond
    // MAX_KEYS_PER_ACTION.
    QString addBinding(const QString& current, const QString& input);
}
