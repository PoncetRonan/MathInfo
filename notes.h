#pragma once

#include <cmath>
#include <string>
#include <map>
#include <cstdio>

// notes.h
// Petit utilitaire header-only pour les 7 notes diatoniques (Do Ré Mi Fa Sol La Si)
// Fournit :
// - mapping clavier -> note (7 notes blanches sur QWERTY)
// - conversion note+octave -> fréquence (Hz), tempérament égal A4=440Hz
// - fonctions utilitaires pour obtenir nom et fréquence depuis une touche

static const char* DIATONIC_NAMES[7] = {"C","D","E","F","G","A","B"};

// Mapping par défaut : touches QWERTY pour les 7 notes blanches
// q = C (Do), w = D (Ré), e = E (Mi), r = F (Fa), t = G (Sol), y = A (La), u = B (Si)
static const std::map<int,int> KEY_TO_DIATONIC = {
    {'q', 0}, // C
    {'w', 1}, // D
    {'e', 2}, // E
    {'r', 3}, // F
    {'t', 4}, // G
    {'y', 5}, // A
    {'u', 6}  // B
};

// Mapping pour les touches noires (dièses) : a = C#, s = D#, d = F#, f = G#, g = A#
static const std::map<int,int> KEY_TO_ACCIDENTAL = {
    {'a', 1}, // C#
    {'s', 3}, // D#
    {'d', 6}, // F#
    {'f', 8}, // G#
    {'g', 10} // A#
};

// Retourne la fréquence (Hz) pour un numéro MIDI
inline double freqFromMidi(int midi) {
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

// Retourne l'indice de note (0=C,1=D,...6=B) si c'est une des 7 touches mappées, sinon -1
inline int diatonicIndexFromKey(int key) {
    int k = key;
    if (k >= 'A' && k <= 'Z') k = k - 'A' + 'a';
    auto it = KEY_TO_DIATONIC.find(k);
    if (it == KEY_TO_DIATONIC.end()) return -1;
    return it->second;
}

// Return semitone (0-11) for accidental key (a,s,d,f,g) or -1 if not accidental
inline int accidentalSemitoneFromKey(int key) {
    int k = key;
    if (k >= 'A' && k <= 'Z') k = k - 'A' + 'a';
    auto it = KEY_TO_ACCIDENTAL.find(k);
    if (it == KEY_TO_ACCIDENTAL.end()) return -1;
    return it->second;
}

// Retourne la fréquence pour une note diatonique (index 0..6) et une octave
// octave convention: C4 = octave 4 (midi 60)
inline double freqFromDiatonicIndex(int index, int octave) {
    // Convert diatonic index (C D E F G A B) to semitone index relative to C
    // semitone offsets for diatonic scale within an octave: C=0, D=2, E=4, F=5, G=7, A=9, B=11
    static const int diatonicToSemitone[7] = {0,2,4,5,7,9,11};
    if (index < 0 || index > 6) return 0.0;
    int semitone = diatonicToSemitone[index];
    int midi = 12 * (octave + 1) + semitone; // C4 -> 60
    return freqFromMidi(midi);
}

// Retourne le nom de la note (ex: "C4") pour la touche et octave fournis
inline std::string noteNameFromKey(int key, int octave) {
    int k = key;
    if (k >= 'A' && k <= 'Z') k = k - 'A' + 'a';
    int acc = accidentalSemitoneFromKey(k);
    if (acc >= 0) {
        // accidental
        static const char* ACC_NAMES[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%s%d", ACC_NAMES[acc], octave);
        return std::string(buf);
    }
    int idx = diatonicIndexFromKey(k);
    if (idx < 0) return std::string("");
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%s%d", DIATONIC_NAMES[idx], octave);
    return std::string(buf);
}

// Retourne la fréquence (Hz) pour la touche (q,w,e,r,t,y,u) et octave
inline double freqFromKey(int key, int octave, double a4 = 440.0) {
    int k = key;
    if (k >= 'A' && k <= 'Z') k = k - 'A' + 'a';
    // check accidental first
    int acc = accidentalSemitoneFromKey(k);
    if (acc >= 0) {
        int midi = 12 * (octave + 1) + acc;
        double f = freqFromMidi(midi);
        if (a4 == 440.0) return f;
        return f * (a4 / 440.0);
    }
    int idx = diatonicIndexFromKey(k);
    if (idx < 0) return 0.0;
    double f = freqFromDiatonicIndex(idx, octave);
    if (a4 == 440.0) return f;
    return f * (a4 / 440.0);
}

// Exemple d'utilisation (à appeler depuis ofApp::keyPressed):
// double f = freqFromKey(key, octave);
// std::string name = noteNameFromKey(key, octave);
