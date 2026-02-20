#pragma once

#include "ofMain.h"
#include "Oscillator.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void drawBackground(); // Fond procédural animé

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
		float volume;
		float attackSpeed;
		float releaseSpeed;
		bool  noteIsOn;
		int   activeKey;

		Oscillator oscillator;

		// ===== FILTRE IIR =====
		bool lowpass;
		float f0, Fs, wo, Q, a;  
		float b0, b1, b2, a0, a1, a2; 
		float xn, xn_1, xn_2;
		float yn, yn_1, yn_2;

		// ===== GUI =====
		ofxPanel gui;
		ofxFloatSlider volumeMax;
		ofxFloatSlider brightnessSlider;
		ofxFloatSlider cutoffSlider;
		ofxToggle lowpassToggle;
};