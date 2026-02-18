#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofBackground(34, 34, 34);
	
	int bufferSize		= 512;
	sampleRate 			= 44100;
	phase 				= 0;
	phaseAdder 			= 0.0f;
	phaseAdderTarget 	= 0.0f;
	volume				= 0.1f;

	lAudio.assign(bufferSize, 0.0);
	uAudio.assign(bufferSize, 0.0);
	fTransform.assign(bufferSize, 0.0);
	f1Transform.assign(bufferSize, 0.0);
	f2Transform.assign(bufferSize, 0.0);
	
	oscillator.setup(sampleRate);

	soundStream.printDeviceList();

	ofSoundStreamSettings settings;

	auto devices = soundStream.getMatchingDevices("default");
	if(!devices.empty()){
		settings.setOutDevice(devices[0]);
	}

	settings.setOutListener(this);
	settings.sampleRate = sampleRate;
	settings.numOutputChannels = 1;
	settings.numInputChannels = 0;
	settings.bufferSize = bufferSize;
	soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){

	ofSetColor(225);
	ofDrawBitmapString("AUDIO OUTPUT EXAMPLE", 32, 32);
	
	ofNoFill();
	
	// draw the mono channel:
	ofPushStyle();
	ofPushMatrix();
	ofTranslate(32, 150, 0);
		
	ofSetColor(225);
	ofDrawBitmapString("Mono Channel", 4, 18);
	ofSetLineWidth(1);	
	ofDrawRectangle(0, 0, 900, 200);

	ofSetColor(245, 58, 135);
	ofSetLineWidth(3);
	ofBeginShape();
	for (unsigned int i = 0; i < uAudio.size(); i++){
		float x =  ofMap(i, 0, uAudio.size(), 0, 900, true);
		ofVertex(x, 100 - uAudio[i]*180.0f);
	}
	ofEndShape(false);
	ofPopMatrix();
	ofPopStyle();

	// draw the FFT:
	ofPushStyle();
	ofPushMatrix();
	ofTranslate(32, 350, 0);
		
	ofSetColor(225);
	ofDrawBitmapString("FFT", 4, 18);
	ofSetLineWidth(1);	
	ofDrawRectangle(0, 0, 900, 200);

	ofSetColor(245, 58, 135);
	ofSetLineWidth(3);
	ofBeginShape();
	for (unsigned int i = 0; i < fTransform.size() / 2; i++){
		float x =  ofMap(i, 0, fTransform.size() / 2, 0, 900, true);
		ofVertex(x, 100 - fTransform[i]*180.0f);
	}
	ofEndShape(false);
	ofPopMatrix();
	ofPopStyle();
	
	ofSetColor(225);
	string reportString = "volume: ("+ofToString(volume, 2)+")\npan: ("+ofToString(pan, 2)+") modify with mouse x\nsynthesis: sine wave (" + ofToString(targetFrequency, 2) + "hz) modify with mouse y";
	ofDrawBitmapString(reportString, 32, 579);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){ }

//--------------------------------------------------------------
void ofApp::keyReleased(int key){ }

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	int width = ofGetWidth();
	pan = (float)x / (float)width;
	float height = (float)ofGetHeight();
	float heightPct = ((height-y) / height);
	targetFrequency = 22000.0f * heightPct;
	phaseAdderTarget = (targetFrequency / (float) sampleRate) * glm::two_pi<float>();
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	int width = ofGetWidth();
	pan = (float)x / (float)width;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){ }

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){ }

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer){

	while (phase > glm::two_pi<float>()){
		phase -= glm::two_pi<float>();
	}

	phaseAdder = 0.95f * phaseAdder + 0.05f * phaseAdderTarget;
	for (size_t i = 0; i < buffer.getNumFrames(); i++){
		phase += phaseAdder;
		float sample = sin(phase);
		lAudio[i] = buffer[i * buffer.getNumChannels()] = sample * volume;
	}

	for (size_t i = 0; i < buffer.getNumFrames(); i++) {
		uAudio[i] = lAudio[i];
	}

	// calcul FFT
	computeFourierTransform(buffer);
}

//--------------------------------------------------------------
void ofApp::computeFourierTransform(const ofSoundBuffer& buffer) {

	size_t N = buffer.getNumFrames();

	for (size_t i = 0; i < N; i++) {
		f1Transform[i] = 0;
		f2Transform[i] = 0;

		for (size_t j = 0; j < N; j++) {
			float angle = 2 * PI * i * j / N;
			f1Transform[i] += uAudio[j] * cos(angle);
			f2Transform[i] += uAudio[j] * sin(angle);
		}
	}

	for (size_t i = 0; i < N; i++) {
		fTransform[i] = 5 * sqrt(f1Transform[i]*f1Transform[i] + f2Transform[i]*f2Transform[i]) / N;
	}
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){ }

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ }