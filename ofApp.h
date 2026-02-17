#pragma once

#include "ofMain.h"

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
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
		void audioOut(ofSoundBuffer & buffer);
		void computeFourierTransform(const ofSoundBuffer& buffer);
		
		ofSoundStream soundStream;

		float 	pan;
		int		sampleRate;
		float 	volume;

		vector <float> lAudio;
		vector <float> uAudio;
		vector <float> fTransform;
		vector <float> f1Transform;
		vector <float> f2Transform;

		//------------------- for the simple sine wave synthesis
		float 	targetFrequency;
		float 	phase;
		float 	phaseAdder;
		float 	phaseAdderTarget;
};