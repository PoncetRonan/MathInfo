#pragma once

#include "ofMain.h"
#include <complex>
#include <mutex>
#include <vector>
#include <map>

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
		void audioOut(ofSoundBuffer & buffer);
		
		// Fonctions pour la synthèse et la FFT
		float calc_sin(float A, float f, float t);
		float calcul_carre(float A, float f, float t, float brillance);
		float calcul_scie(float A, float f, float t, float brillance);
		float calcul_mixage(float A, float f, float t, float brillance, float formeFloat);
		float synthSample();
		void computeSpectrum(const std::vector<float>& timeSignal, std::vector<float>& magnitude);
		static void fft(std::vector<std::complex<float>>& data);
		
		ofSoundStream soundStream;

		float 	pan;
		int		sampleRate;
		bool 	bNoise;
		float 	volume;

		vector <float> lAudio;
		vector <float> rAudio;
		
		// Variables pour la synthèse sinusoïdale
		float 	freq;
		float 	targetFrequency;
		float 	phase;
		float 	phaseAdder;
		float 	phaseAdderTarget;
		float 	t;
		float 	formeOndeFloat;	// 0-1: sin/carré, 1-2: carré/scie (interpolation)
		float 	brillance;		// nombre d'harmoniques avec interpolation (0-20.0)
		
		// Variables pour clavier musical
		bool 	keyboardMode;	// Mode clavier musical activé
		std::map<int, float> activeKeys;  // Clés pressées et leurs fréquences
		float 	baseFreq;		// Fréquence de base (C4 = 261.63 Hz)
		int 	octave;			// Octave courante (0-8)
		
		// Variables pour la FFT et l'affichage du spectre
		std::vector<float> spectrum;
		std::vector<float> window;
		std::vector<std::complex<float>> fftBuffer;
		int fftSize;
		std::mutex audioMutex;
		
		// Variables pour le spectrogramme
		std::vector<std::vector<float>> spectrogram;  // Historique du spectre (temps/fréquence)
		int spectrogramWidth;   // Nombre de trames (colonnes)
		int spectrogramHeight;  // Nombre de fréquences (lignes)
		bool showSpectrogram;   // Affichage du spectrogramme activé

		// Helpers: détection de la fréquence dominante et conversion en nom de note
		double findDominantFrequency(const std::vector<float>& magnitude);
		std::string freqToNoteName(double freq);

		// YIN pitch detection (time-domain). Returns 0.0 if no pitch found.
		double estimatePitchYIN(const std::vector<float>& signal, double sampleRate, double threshold = 0.15);

		// Return nearest MIDI note and cents deviation for a frequency
		std::pair<int,double> nearestNoteAndCents(double freq);

		// Simple sequencer to play a scale (no separate thread) — used by pressing 'p'
		bool isPlayingSequence = false;
		std::vector<float> sequenceFreqs;
		int seqIndex = 0;
		int samplesPerNote = 0;
		int seqSampleCounter = 0;
		void startPlayScale(int octave = 4, float noteDurationSeconds = 0.5f);
};