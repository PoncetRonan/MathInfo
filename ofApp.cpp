#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ofBackground(10, 10, 20);
    ofSetCircleResolution(60);

    int bufferSize = 512;
    sampleRate = 44100;

    phase = 0;
    phaseAdder = 0.0f;
    phaseAdderTarget = 0.0f;
    targetFrequency = 261.63f;
    sample = 0;

    // ===== ENVELOPPE =====
    volume = 0.0f;
    attackSpeed = 0.01f;  
    releaseSpeed = 0.01f; 
    noteIsOn = false;
    activeKey = 0;

    pan = 0.5f;

    // ===== FILTRE IIR =====
    lowpass = false;
    xn = xn_1 = xn_2 = 0;
    yn = yn_1 = yn_2 = 0;
    f0 = 1000.0f;

    // ===== GUI =====
    gui.setup("Parametres Synth");
    gui.add(volumeMax.setup("Volume Max", 0.2f, 0.0f, 0.5f));
    gui.add(brightnessSlider.setup("Brillance", 2.0f, 1.0f, 10.0f));
    gui.add(cutoffSlider.setup("Filtre Freq (Hz)", 1000.0f, 50.0f, 5000.0f));
    gui.add(lowpassToggle.setup("Activer Filtre", false));
    
    uAudio.assign(bufferSize, 0.0);
    fTransform.assign(bufferSize, 0.0);
    f1Transform.assign(bufferSize, 0.0);
    f2Transform.assign(bufferSize, 0.0);

    oscillator.setup(sampleRate);

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
    oscillator.setBrightness(brightnessSlider);
    f0 = cutoffSlider;
    lowpass = lowpassToggle;
}

//--------------------------------------------------------------
void ofApp::drawBackground(){
    float t = ofGetElapsedTimef();

    // Fond noir bleuté
    ofBackground(10, 10, 20);

    // ---- Grille animée style oscilloscope ----
    ofSetLineWidth(1);
    int gridStep = 40;
    for(int x = 0; x < ofGetWidth(); x += gridStep){
        float alpha = ofMap(sin(t * 0.4f + x * 0.015f), -1, 1, 15, 55);
        ofSetColor(0, 90, 140, alpha);
        ofDrawLine(x, 0, x, ofGetHeight());
    }
    for(int y = 0; y < ofGetHeight(); y += gridStep){
        float alpha = ofMap(sin(t * 0.4f + y * 0.015f), -1, 1, 15, 55);
        ofSetColor(0, 90, 140, alpha);
        ofDrawLine(0, y, ofGetWidth(), y);
    }

    // ---- Lignes de scan CRT ----
    for(int y = 0; y < ofGetHeight(); y += 4){
        ofSetColor(0, 0, 0, 18);
        ofDrawLine(0, y, ofGetWidth(), y);
    }

    // ---- Lueur centrale pulsante ----
    float pulse   = volume * 320.0f;
    float breathe = ofMap(sin(t * 1.2f), -1, 1, 0.85f, 1.15f);
    float baseW   = 420.0f * breathe;
    float baseH   = 220.0f * breathe;

    ofSetColor(245, 58, 135, 8  + volume * 25);
    ofDrawEllipse(ofGetWidth()/2, ofGetHeight()/2, baseW + pulse * 1.4f, baseH + pulse * 0.7f);

    ofSetColor(180, 30, 100, 12 + volume * 35);
    ofDrawEllipse(ofGetWidth()/2, ofGetHeight()/2, baseW + pulse, baseH + pulse * 0.5f);

    ofSetColor(245, 58, 135, 18 + volume * 50);
    ofDrawEllipse(ofGetWidth()/2, ofGetHeight()/2, baseW * 0.55f + pulse * 0.4f, baseH * 0.55f + pulse * 0.2f);

    // ---- Particules flottantes néon ----
    ofSeedRandom(42);
    int numParticles = 45 * static_cast<int>(brightnessSlider);
    for(int i = 0; i < numParticles; i++){
        float px    = ofRandom(ofGetWidth());
        float py    = ofRandom(ofGetHeight());
        float speed = ofRandom(0.15f*(oscillator.octave+1), 0.6f*(oscillator.octave+1));az
        float drift = ofRandom(0.05f*(oscillator.octave+1), 0.3f*(oscillator.octave+1));
        float fx    = px + sin(t * speed + i * 1.3f) * 18.0f;
        float fy    = py + cos(t * drift  + i * 2.1f) * 12.0f;
        float sz    = ofRandom(1.5f+(noteIsOn * ofRandom(3)), 4.0f+(noteIsOn * ofRandom(3)));
        float alpha = ofClamp(ofRandom(30, 90) + volume * 80, 0, 255);

        if(i % 2 == 0 && lowpassToggle == false) ofSetColor(0, 200, 255, alpha);
        else            ofSetColor(245, 58, 135, alpha);
        ofFill();
        ofDrawCircle(fx, fy, sz);
    }

    // ---- Grande sinusoïde décorative de fond ----
    ofSetLineWidth(1.5f);
    ofSetColor(0, 191, 255, 25 + volume * 40);
    ofBeginShape();
    for(int x = 0; x <= ofGetWidth(); x += 3){
        float y = ofGetHeight() * 0.5f
                  + sin(x * 0.012f + t * 1.5f) * (30.0f + volume * 60.0f)
                  + sin(x * 0.025f + t * 0.8f) * (15.0f + volume * 30.0f);
        ofVertex(x, y);
    }
    ofEndShape(false);

    // ---- Vignette (bords sombres) ----
    int vSize = 120;
    for(int i = 0; i < vSize; i++){
        float a = ofMap(i, 0, vSize, 80, 0);
        ofSetColor(0, 0, 0, a);
        ofDrawRectangle(i,                   i,                    ofGetWidth()  - i*2, 1);
        ofDrawRectangle(i,                   ofGetHeight() - i - 1, ofGetWidth() - i*2, 1);
        ofDrawRectangle(i,                   i,                    1, ofGetHeight() - i*2);
        ofDrawRectangle(ofGetWidth() - i - 1, i,                    1, ofGetHeight() - i*2);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){

    // ===== FOND ANIMÉ =====
    drawBackground();

    // ===== TITRE =====
    ofSetColor(200, 200, 220);
    ofDrawBitmapString("SYNTHETISEUR INTERACTIF", 32, 32);
    
    // ===== Signal Temporel =====
    ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 100, 0);

    ofSetColor(0, 0, 0, 110);
    ofFill();
    ofDrawRectangle(0, 0, 900, 150);

    ofSetColor(200, 200, 220);
    ofDrawBitmapString("Signal Temporel", 4, 18);
    ofNoFill();
    ofSetColor(80, 80, 100, 180);
    ofDrawRectangle(0, 0, 900, 150);

    ofSetColor(245, 58, 135);
    ofSetLineWidth(2);
    ofBeginShape();
    for (unsigned int i = 0; i < uAudio.size(); i++){
        float x = ofMap(i, 0, uAudio.size(), 0, 900, true);
        ofVertex(x, 75 - uAudio[i] * 120.0f);
    }
    ofEndShape(false);
    ofPopMatrix();
    ofPopStyle();

    // ===== FFT =====
    ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 280, 0);

    ofSetColor(0, 0, 0, 110);
    ofFill();
    ofDrawRectangle(0, 0, 900, 150);

    ofSetColor(200, 200, 220);
    ofDrawBitmapString("Analyse Frequentielle", 4, 18);
    ofNoFill();
    ofSetColor(80, 80, 100, 180);
    ofDrawRectangle(0, 0, 900, 150);

    ofSetColor(0, 191, 255);
    ofBeginShape();
    for (unsigned int i = 0; i < fTransform.size() / 2; i++){
        float x = ofMap(i, 0, fTransform.size() / 2, 0, 900, true);
        ofVertex(x, 140 - fTransform[i] * 150.0f);
    }
    ofEndShape(false);
    ofPopMatrix();
    ofPopStyle();

    // ===== GUI =====
    gui.draw();

    // ===== Info forme d'onde =====
    ofSetColor(180, 180, 200);
    string info = "Forme d'onde (w): " + ofToString(oscillator.waveform);
    ofDrawBitmapString(info, 32, 460);

    // ===== CLAVIER VISUEL =====
    ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 480);

    struct KeyNote { int key; string line1; string line2; bool isBlack; int xPos; };
    vector<KeyNote> keys = {
        {'a', "a",  "Do",   false, 0},
        {'s', "s",  "Do#",  true,  25},
        {'z', "z",  "Re",   false, 50},
        {'d', "d",  "Re#",  true,  75},
        {'e', "e",  "Mi",   false, 100},
        {'r', "r",  "Fa",   false, 150},
        {'f', "f",  "Fa#",  true,  175},
        {'t', "t",  "Sol",  false, 200},
        {'g', "g",  "Sol#", true,  225},
        {'y', "y",  "La",   false, 250},
        {'h', "h",  "La#",  true,  275},
        {'u', "u",  "Si",   false, 300},
    };

    // Touches blanches
    for (auto& k : keys) {
        if (!k.isBlack) {
            bool isActive = (activeKey == k.key);
            if (isActive) ofSetColor(245, 58, 135);
            else          ofSetColor(220, 220, 230);
            ofFill();
            ofDrawRectangle(k.xPos, 0, 48, 120);
            ofSetColor(60, 60, 80);
            ofNoFill();
            ofDrawRectangle(k.xPos, 0, 48, 120);
            if (isActive) ofSetColor(255, 255, 255);
            else          ofSetColor(40, 40, 60);
            ofDrawBitmapString(k.line1, k.xPos + 17, 85);
            ofDrawBitmapString(k.line2, k.xPos + (k.line2.size() == 2 ? 14 : 8), 100);
        }
    }

    // Touches noires
    for (auto& k : keys) {
        if (k.isBlack) {
            bool isActive = (activeKey == k.key);
            if (isActive) ofSetColor(245, 58, 135);
            else          ofSetColor(20, 20, 35);
            ofFill();
            ofDrawRectangle(k.xPos, 0, 32, 75);
            if (isActive) ofSetColor(255, 255, 255);
            else          ofSetColor(180, 180, 200);
            ofDrawBitmapString(k.line1, k.xPos + 11, 50);
            ofDrawBitmapString(k.line2, k.xPos + (k.line2.size() == 3 ? 4 : 8), 63);
        }
    }

    ofSetColor(160, 160, 190);
    ofDrawBitmapString("Octave: " + ofToString(oscillator.octave) + "   (Up / Down pour changer)", 0, 145);

    ofPopMatrix();
    ofPopStyle();
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
        case 's': frequency = 277.18f; break; 
        case 'd': frequency = 311.13f; break; 
        case 'f': frequency = 369.99f; break; 
        case 'g': frequency = 415.30f; break; 
        case 'h': frequency = 466.16f; break; 
    }

    if(key == 'w'){
        if(oscillator.waveform == SINE)          oscillator.setWaveform(SQUARE);
        else if(oscillator.waveform == SQUARE)   oscillator.setWaveform(SAW);
        else if(oscillator.waveform == SAW)      oscillator.setWaveform(TRIANGLE);
        else if(oscillator.waveform == TRIANGLE) oscillator.setWaveform(SINE);
    }

    if(key == OF_KEY_UP   && oscillator.octave < 5) oscillator.octave++;
    if(key == OF_KEY_DOWN && oscillator.octave > 0) oscillator.octave--;

    if(frequency > 0){
        targetFrequency = frequency * std::pow(2, oscillator.octave);
        oscillator.setFrequency(targetFrequency);
        noteIsOn = true;
        activeKey = key;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    noteIsOn = false;
    activeKey = 0;
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer){

    for (size_t i = 0; i < buffer.getNumFrames(); i++){

        if(noteIsOn){
            volume += attackSpeed;
            if(volume > volumeMax) volume = (float)volumeMax;
        } else {
            volume -= releaseSpeed;
            if(volume < 0.0f) volume = 0.0f;
        }

        sample = oscillator.getNextSample();
        
        xn_2 = xn_1;
        xn_1 = xn;
        xn = sample;

        yn_2 = yn_1;
        yn_1 = yn;
        yn = computeIIRFilter();

        float finalSample = lowpass ? yn : sample;
        buffer[i] = finalSample * volume;
        
        uAudio[i] = buffer[i]; 
    }
    computeFourierTransform(buffer);
}

//--------------------------------------------------------------
float ofApp::computeIIRFilter() {
    float Fs = sampleRate;
    float wo = glm::two_pi<float>() * (f0 / Fs);
    float Q  = 1.0f;
    float alpha = sin(wo) / (2.0f * Q);
    
    float b0 = alpha;
    float b1 = 0;
    float b2 = -alpha;
    float a0 = 1 + alpha;
    float a1 = -2 * cos(wo);
    float a2 = 1 - alpha;

    return (b0/a0) * xn + (b1/a0) * xn_1 + (b2/a0) * xn_2 - (a1/a0) * yn_1 - (a2/a0) * yn_2;
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
void ofApp::mouseMoved(int x, int y ){ pan = (float)x / (float)ofGetWidth(); }
void ofApp::mouseDragged(int x, int y, int button){ pan = (float)x / (float)ofGetWidth(); }
void ofApp::mousePressed(int x, int y, int button){}
void ofApp::mouseReleased(int x, int y, int button){}
void ofApp::dragEvent(ofDragInfo dragInfo){}
void ofApp::gotMessage(ofMessage msg){}