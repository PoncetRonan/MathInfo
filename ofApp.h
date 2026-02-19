#pragma once

#include "ofMain.h"
#include "Oscillator.h"
#include "def.h"

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

		// SINE WAVE
		float targetFrequency;
		float phase;
		float phaseAdder;
		float phaseAdderTarget;
		float sample;


		// ===== ENVELOPPE =====
		float volume;        // volume courant
		float targetVolume;  // volume max
		float attackSpeed;   // vitesse montée
		float releaseSpeed;  // vitesse descente
		bool  noteIsOn;      // état de la note

		Oscillator oscillator;

		// filter
		float f0; 
		float Fs; 
		float wo; 
		float Q;  
		float a;  
		float b0; 
		float b1; 
		float b2; 
		float a0; 
		float a1; 
		float a2; 

		float xn;
		float xn_1;
		float xn_2;
		float yn;
		float yn_1;
		float yn_2;
		
};