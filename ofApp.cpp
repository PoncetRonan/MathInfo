#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ofBackground(34, 34, 34);

    int bufferSize = 512;
    sampleRate = 44100;

    phase = 0;
    phaseAdder = 0.0f;
    phaseAdderTarget = 0.0f;
    targetFrequency = 261.63f; // Do par défaut

    // ===== ENVELOPPE =====
    volume = 0.0f;
    targetVolume = 0.1f;   // volume maximum
    attackSpeed = 0.002f;  // vitesse montée
    releaseSpeed = 0.002f; // vitesse descente
    noteIsOn = false;

    pan = 0.5f;

    lAudio.assign(bufferSize, 0.0);
    uAudio.assign(bufferSize, 0.0);
    fTransform.assign(bufferSize, 0.0);
    f1Transform.assign(bufferSize, 0.0);
    f2Transform.assign(bufferSize, 0.0);

    ofSoundStreamSettings settings;
    settings.setOutListener(this);
    settings.sampleRate = sampleRate;
    settings.numOutputChannels = 1;
    settings.numInputChannels = 0;
    settings.bufferSize = bufferSize;

    soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::update(){ }

//--------------------------------------------------------------
void ofApp::draw(){

    ofSetColor(225);
    ofDrawBitmapString("AUDIO OUTPUT EXAMPLE", 32, 32);

    ofNoFill();

    // ===== MONO WAVEFORM =====
    ofPushMatrix();
    ofTranslate(32, 150, 0);

    ofSetColor(225);
    ofDrawBitmapString("Mono Channel", 4, 18);
    ofDrawRectangle(0, 0, 900, 200);

    ofSetColor(245, 58, 135);
    ofSetLineWidth(3);
    ofBeginShape();
    for (unsigned int i = 0; i < uAudio.size(); i++){
        float x = ofMap(i, 0, uAudio.size(), 0, 900, true);
        ofVertex(x, 100 - uAudio[i] * 180.0f);
    }
    ofEndShape(false);
    ofPopMatrix();

    // ===== FFT =====
    ofPushMatrix();
    ofTranslate(32, 350, 0);

    ofSetColor(225);
    ofDrawBitmapString("FFT", 4, 18);
    ofDrawRectangle(0, 0, 900, 200);

    ofSetColor(245, 58, 135);
    ofSetLineWidth(3);
    ofBeginShape();
    for (unsigned int i = 0; i < fTransform.size() / 2; i++){
        float x = ofMap(i, 0, fTransform.size() / 2, 0, 900, true);
        ofVertex(x, 100 - fTransform[i] * 180.0f);
    }
    ofEndShape(false);
    ofPopMatrix();

    ofSetColor(225);
	string reportString = string("Envelope synth\n") // Wrap the first one in string()
						+ "Current volume: " + ofToString(volume, 2) + "\n"
						+ "Frequency: " + ofToString(targetFrequency, 2) + " Hz\n"
						+ "Keys: a z e r t y u";
    ofDrawBitmapString(reportString, 32, 579);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

    float frequency = 0;

    switch(key){
        case 'a': frequency = 261.63f; break;
        case 'z': frequency = 293.66f; break;
        case 'e': frequency = 329.63f; break;
        case 'r': frequency = 349.23f; break;
        case 't': frequency = 392.00f; break;
        case 'y': frequency = 440.00f; break;
        case 'u': frequency = 493.88f; break;
    }

    if(frequency > 0){
        targetFrequency = frequency;
        phaseAdderTarget = (targetFrequency / (float)sampleRate) * glm::two_pi<float>();
        noteIsOn = true; // déclenche Attack
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

    switch(key){
        case 'a': case 'z': case 'e': case 'r':
        case 't': case 'y': case 'u':
            noteIsOn = false; // déclenche Release
            break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
    pan = (float)x / (float)ofGetWidth();
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    pan = (float)x / (float)ofGetWidth();
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

        // ===== ENVELOPPE ATTACK / RELEASE =====
        if(noteIsOn){
            volume += attackSpeed;
            if(volume > targetVolume)
                volume = targetVolume;
        }else{
            volume -= releaseSpeed;
            if(volume < 0.0f)
                volume = 0.0f;
        }

        phase += phaseAdder;
        float sample = sin(phase);

        buffer[i] = sample * volume;
        lAudio[i] = buffer[i];
        uAudio[i] = buffer[i];
    }

    computeFourierTransform(buffer);
}

//--------------------------------------------------------------
void ofApp::computeFourierTransform(const ofSoundBuffer& buffer){

    size_t N = buffer.getNumFrames();

    for (size_t i = 0; i < N; i++){
        f1Transform[i] = 0;
        f2Transform[i] = 0;

        for (size_t j = 0; j < N; j++){
            float angle = 2 * PI * i * j / N;
            f1Transform[i] += uAudio[j] * cos(angle);
            f2Transform[i] += uAudio[j] * sin(angle);
        }
    }

    for (size_t i = 0; i < N; i++){
        fTransform[i] = 5 * sqrt(f1Transform[i]*f1Transform[i] + f2Transform[i]*f2Transform[i]) / N;
    }
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){ }

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ }
