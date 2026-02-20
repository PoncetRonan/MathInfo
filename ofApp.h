#pragma once

#include "ofMain.h"
#include "Oscillator.h"
#include "ofxGui.h" // Nécessaire pour l'interface graphique

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
		void audioOut(ofSoundBuffer & buffer);
		void computeFourierTransform(const ofSoundBuffer& buffer);
		float computeIIRFilter();
		
		ofSoundStream soundStream;

		float pan;
		int sampleRate;

		// AUDIO BUFFERS
		vector<float> lAudio;
		vector<float> uAudio;
		vector<float> fTransform;
		vector<float> f1Transform;
		vector<float> f2Transform;

		// SYNTH & PHASE
		float targetFrequency;
		float phase;
		float phaseAdder;
		float phaseAdderTarget;
		float sample;

		// ===== ENVELOPPE =====
		float volume;        // volume courant (pour le lissage)
		float attackSpeed;   // vitesse montée
		float releaseSpeed;  // vitesse descente
		bool  noteIsOn;      // état de la note

		Oscillator oscillator;

		// ===== FILTRE IIR =====
		bool lowpass;
		float f0, Fs, wo, Q, a;  
		float b0, b1, b2, a0, a1, a2; 
		float xn, xn_1, xn_2;
		float yn, yn_1, yn_2;

		// ==========================================
		// INTERFACE GRAPHIQUE (GUI)
		// ==========================================
		ofxPanel gui;                    // Panneau principal
		ofxFloatSlider volumeMax;        // Slider pour le volume max
		ofxFloatSlider brightnessSlider; // Slider pour la brillance de l'oscillateur
		ofxFloatSlider cutoffSlider;     // Slider pour la fréquence du filtre
		ofxToggle lowpassToggle;         // Interrupteur pour activer/désactiver le filtre
		// ==========================================
		
};