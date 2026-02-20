#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ofBackground(34, 34, 34);
    ofSetCircleResolution(60);

    int bufferSize = 512;
    sampleRate = 44100;

    // Initialisation des variables de phase et fréquence
    phase = 0;
    phaseAdder = 0.0f;
    phaseAdderTarget = 0.0f;
    targetFrequency = 261.63f; // Do par défaut
    sample = 0;

    // ===== ENVELOPPE =====
    volume = 0.0f;
    attackSpeed = 0.01f;  
    releaseSpeed = 0.01f; 
    noteIsOn = false;

    pan = 0.5f;

    // ===== FILTRE IIR =====
    lowpass = false;
    xn = xn_1 = xn_2 = 0;
    yn = yn_1 = yn_2 = 0;
    f0 = 1000.0f;

    // ===== INTERFACE GRAPHIQUE (GUI) =====
    gui.setup("Parametres Synth");
    gui.add(volumeMax.setup("Volume Max", 0.2f, 0.0f, 0.5f));
    gui.add(brightnessSlider.setup("Brillance", 10.0f, 1.0f, 40.0f));
    gui.add(cutoffSlider.setup("Filtre Freq (Hz)", 1000.0f, 50.0f, 5000.0f));
    gui.add(lowpassToggle.setup("Activer Filtre", false));
    
    // Initialisation des buffers pour la visualisation
    uAudio.assign(bufferSize, 0.0);
    fTransform.assign(bufferSize, 0.0);
    f1Transform.assign(bufferSize, 0.0);
    f2Transform.assign(bufferSize, 0.0);

    oscillator.setup(sampleRate);

    // Configuration du flux audio
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
    // Mise à jour des paramètres de l'oscillateur et du filtre via le GUI
    oscillator.setBrightness(brightnessSlider);
    f0 = cutoffSlider;
    lowpass = lowpassToggle;
}

//--------------------------------------------------------------
void ofApp::draw(){

    ofSetColor(225);
    ofDrawBitmapString("SYNTHETISEUR INTERACTIF", 32, 32);
    
    // Dessin de la forme d'onde (Mono Channel)
    ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 100, 0);
    ofSetColor(225);
    ofDrawBitmapString("Signal Temporel", 4, 18);
    ofNoFill();
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

    // Dessin de la FFT
    ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 280, 0);
    ofSetColor(225);
    ofDrawBitmapString("Analyse Frequentielle (FFT)", 4, 18);
    ofNoFill();
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

    // Affichage de l'interface GUI
    gui.draw();

    // Instructions clavier
    ofSetColor(200);
    string info = "Clavier: a,z,e,r,t,y,u (Notes) | Up/Down (Octave: " + ofToString(oscillator.octave) + ")\n";
    info += "Forme d'onde (w): " + ofToString(oscillator.waveform);
    ofDrawBitmapString(info, 32, 480);
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
        if(oscillator.waveform == SINE) oscillator.setWaveform(SQUARE);
        else if(oscillator.waveform == SQUARE) oscillator.setWaveform(SAW);
        else if(oscillator.waveform == SAW) oscillator.setWaveform(TRIANGLE);
        else if(oscillator.waveform == TRIANGLE) oscillator.setWaveform(SINE);
    }

    if(key == OF_KEY_UP && oscillator.octave < 8) oscillator.octave++;
    if(key == OF_KEY_DOWN && oscillator.octave > 0) oscillator.octave--;

    if(frequency > 0){
        targetFrequency = frequency * std::pow(2, oscillator.octave);
        oscillator.setFrequency(targetFrequency);
        noteIsOn = true; 
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    noteIsOn = false;
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer){

    for (size_t i = 0; i < buffer.getNumFrames(); i++){

        // Gestion de l'enveloppe
        if(noteIsOn){
            volume += attackSpeed;
            if(volume > volumeMax) volume = (float)volumeMax;
        } else {
            volume -= releaseSpeed;
            if(volume < 0.0f) volume = 0.0f;
        }

        // Génération du signal
        sample = oscillator.getNextSample();
        
        // Filtrage IIR
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
void ofApp::mouseMoved(int x, int y ){
    // Optionnel : mettre à jour le pan ici si vous le souhaitez
    pan = (float)x / (float)ofGetWidth();
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    pan = (float)x / (float)ofGetWidth();
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    // Logique au clic si nécessaire
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    // Logique au relâchement si nécessaire
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
    // Gestion du glisser-déposer de fichiers
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    // Gestion des messages système
}