#include "ofApp.h"
#include <cmath>
#include <algorithm>
#include "notes.h"
#include <string>
#include <cstdio>
#include <numeric>
#include <cassert>

//--------------------------------------------------------------
void ofApp::setup(){

	ofBackground(34, 34, 34);
	
	int bufferSize		= 512;
	sampleRate 			= 44100;
	phase 				= 0;
	phaseAdder 			= 0.0f;
	phaseAdderTarget 	= 0.0f;
	volume				= 0.1f;
	freq				= 440.0f;
	t					= 0.0f;
	formeOndeFloat		= 0.0f;	// Sinusoïde par défaut (interpolation 0-2)
	brillance			= 5.0f;	// 5 harmoniques par défaut
	bNoise 				= false;
	pan					= 0.5f;
	
	// Initialisation du clavier musical
	keyboardMode		= false;
	baseFreq			= 261.63f;  // C4 (Do)
	octave				= 4;

	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);
	
	// Initialisation de la FFT
	fftSize = bufferSize;
	spectrum.assign(fftSize / 2, 0.0);
	
	fftBuffer.assign(fftSize, std::complex<float>(0.0f, 0.0f));
	
	// Fenêtre de Hann pour la FFT
	window.assign(fftSize, 0.0);
	for (int i = 0; i < fftSize; i++) {
		window[i] = 0.5f * (1.0f - cos(2.0f * glm::pi<float>() * i / (fftSize - 1)));
	}
	
	// Initialisation du spectrogramme
	spectrogramWidth = 256;  // Nombre de trames (colonnes)
	spectrogramHeight = fftSize / 2;  // Nombre de fréquences (lignes)
	spectrogram.assign(spectrogramWidth, std::vector<float>(spectrogramHeight, 0.0f));
	showSpectrogram = true;
	
	soundStream.printDeviceList();

	ofSoundStreamSettings settings;

	auto devices = soundStream.getMatchingDevices("default");
	if(!devices.empty()){
		settings.setOutDevice(devices[0]);
	}

	settings.setOutListener(this);
	settings.sampleRate = sampleRate;
	settings.numOutputChannels = 2;
	settings.numInputChannels = 0;
	settings.bufferSize = bufferSize;
	soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::update(){
	// Calculer le spectre du signal temporel
	std::lock_guard<std::mutex> lock(audioMutex);
	computeSpectrum(lAudio, spectrum);
	
	// Ajouter le spectre actuel au spectrogramme
	if (showSpectrogram) {
		// Décaler les colonnes vers la gauche et ajouter la nouvelle à droite
		for (int i = 0; i < spectrogramWidth - 1; i++) {
			spectrogram[i] = spectrogram[i + 1];
		}
		// Copier le spectre actuel dans la dernière colonne
		if (spectrum.size() == spectrogramHeight) {
			spectrogram[spectrogramWidth - 1] = spectrum;
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofSetColor(225);
	ofDrawBitmapString("AUDIO OUTPUT EXAMPLE", 32, 32);
	ofDrawBitmapString("press 's' to unpause the audio\npress 'e' to pause the audio", 31, 92);
	
	ofNoFill();
	
	// Dessiner le signal temporel (canal gauche)
	ofPushStyle();
		ofPushMatrix();
		ofTranslate(32, 150, 0);
			
		ofSetColor(225);
		ofDrawBitmapString("Signal Temporel (Domaine Temporel)", 4, 18);
		
		ofSetLineWidth(2);	
		ofDrawRectangle(0, 0, 900, 200);

		ofSetColor(245, 58, 135);
		ofSetLineWidth(2);
					
		ofBeginShape();
		for (unsigned int i = 0; i < lAudio.size(); i++){
			float x =  ofMap(i, 0, lAudio.size(), 0, 900, true);
			ofVertex(x, 100 -lAudio[i]*180.0f);
		}
		ofEndShape(false);
			
		ofPopMatrix();
	ofPopStyle();

	// Dessiner le spectre fréquentiel (FFT)

	ofPushStyle();
		ofPushMatrix();
		//ofTranslate(32, 400, 0);
		ofTranslate(32, 184, 0);
		ofSetColor(225);
		ofDrawBitmapString("Spectre Frequentiel (Domaine Frequentiel)", 4, 18);
		
		ofSetLineWidth(2);	
		ofDrawRectangle(0, 0, 900, 200);

		ofSetColor(58, 245, 135);
		ofSetLineWidth(2);
		
		std::lock_guard<std::mutex> lock(audioMutex);
			double detectedFreq = 0.0;
			std::string detectedNote = "---";
			if (spectrum.size() > 0) {
			ofBeginShape();
			for (unsigned int i = 0; i < spectrum.size(); i++){
				float x = ofMap(i, 0, spectrum.size(), 0, 900, true);
				float magnitude = spectrum[i];
				float y = ofMap(magnitude, 0, 1.0, 200, 0, true);
				ofVertex(x, y);
			}
			ofEndShape(false);

				// Estimer la fréquence dominante à partir du spectre
				detectedFreq = findDominantFrequency(spectrum);
				// Essayer YIN sur le signal temporel (meilleur pour monophonie) si possible
				// Note: YIN est limité par la longueur du buffer (fftSize), basses fréquences peuvent être inaccessibles
				double yinFreq = 0.0;
				if (lAudio.size() >= 32) {
					// lAudio est protégé par le même mutex
					yinFreq = estimatePitchYIN(lAudio, (double)sampleRate, 0.12);
				}
				// Préférer YIN si valide
				if (yinFreq > 0.0) {
					detectedFreq = yinFreq;
				}
				detectedNote = freqToNoteName(detectedFreq);
		}
			
		ofPopMatrix();
	ofPopStyle();
	
	// Dessiner le spectrogramme (Temps/Fréquence)
	if (showSpectrogram) {
		ofPushStyle();
			ofPushMatrix();
			ofTranslate(950, 150, 0);
			ofSetColor(225);
			ofDrawBitmapString("Spectrogramme (Temps/Frequence)", 4, 18);
			
			ofSetLineWidth(1);
			ofDrawRectangle(0, 0, 256, 200);
			
			// Dessiner le spectrogramme
			for (int x = 0; x < spectrogramWidth; x++) {
				for (int y = 0; y < spectrogramHeight; y++) {
					if (x < spectrogram.size() && y < spectrogram[x].size()) {
						// Convertir la magnitude en couleur (HSV -> RGB)
						float magnitude = spectrogram[x][y];
						magnitude = std::min(1.0f, magnitude * 2.0f);  // Amplifier légèrement
						
						// Échelle de couleur : bleu (faible) -> cyan -> vert -> jaune -> rouge (fort)
						float hue = (1.0f - magnitude) * 240.0f;  // 240 (bleu) à 0 (rouge)
						ofColor col = ofColor::fromHsb(hue / 360.0f * 255.0f, 200, 200);
						
						ofSetColor(col);
						ofDrawRectangle(x, y * 200 / spectrogramHeight, 1, 200 / spectrogramHeight);
					}
				}
			}
			
			ofPopMatrix();
		ofPopStyle();
	}
	
	ofSetColor(225);
	std::string reportString = "volume: ("+ofToString(volume, 2)+") modify with mouse y\npan: ("+ofToString(pan, 2)+") modify with mouse x\n";
	
	if( !bNoise ){
		std::string waveType = "sine";
		if (formeOndeFloat < 0.5f) {
			waveType = "sine->square";
		} else if (formeOndeFloat < 1.0f) {
			waveType = "square";
		} else if (formeOndeFloat < 1.5f) {
			waveType = "square->sawtooth";
		} else {
			waveType = "sawtooth";
		}
		
		reportString += "synthesis: " + waveType + " (" + ofToString(freq, 2) + "hz - range: 20Hz to 20kHz)\n";
		reportString += "waveform: " + ofToString(formeOndeFloat, 2) + " (0=sin, 0.5=sq, 1=sq, 1.5=saw, 2=saw) use 1,2,3 or </>\n";
		reportString += "brillance: " + ofToString(brillance, 2) + " (1-20) with smooth interpolation modify with arrow keys\n";
		reportString += "spectrogram: (press 'v' to toggle) ";
		if (showSpectrogram) reportString += "ON";
		else reportString += "OFF";
		reportString += "\n";

		// Afficher la note détectée (si disponible) avec l'écart en cents
		if (detectedFreq > 0.0) {
			// calculer note la plus proche et l'écart en cents
			auto [midiNearest, cents] = nearestNoteAndCents(detectedFreq);
			std::string nearestName = "---";
			if (midiNearest >= 0) {
				int nameIndex = (midiNearest % 12 + 12) % 12;
				int octaveNum = midiNearest / 12 - 1;
				static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
				char notebuf[64];
				std::snprintf(notebuf, sizeof(notebuf), "%s%d", names[nameIndex], octaveNum);
				nearestName = std::string(notebuf);
			}
			char buf[128];
			std::snprintf(buf, sizeof(buf), "Detected: %s (%.2f Hz)  Nearest: %s %+0.0f cents\n", detectedNote.c_str(), detectedFreq, nearestName.c_str(), cents);
			reportString += std::string(buf);
		} else {
			reportString += "Detected: ---\n";
		}
		reportString += "\nKEYBOARD MODE: ";
		if (keyboardMode) {
			reportString += "ON (press 'k' to toggle)\nOctave: " + ofToString(octave) + " (use [ and ] to change)\n";
			reportString += "QWERTY = white keys, ASDEG = black keys";
		} else {
			reportString += "OFF (press 'k' to enable)";
		}
	}else{
		reportString += "synthesis: noise";	
	}
	ofDrawBitmapString(reportString, 32, 650);

	// Affichage plus visible de la note détectée (couleur et position proche du spectrogramme)
	{
		ofPushStyle();
			ofSetColor(255, 100, 100);
			std::string bigNote = detectedNote;
			if (detectedFreq > 0.0) {
				char buf2[64];
				std::snprintf(buf2, sizeof(buf2), "%s (%.1f Hz)", detectedNote.c_str(), detectedFreq);
				bigNote = std::string(buf2);
			}
			// Positionner le texte au-dessus du spectrogramme
			ofDrawBitmapString(bigNote, 950, 130);
		ofPopStyle();
	}

	// Dessiner un petit clavier visuel (q-w-e-r-t-y-u blancs et a-s-d-f-g noirs)
	{
		ofPushStyle();
		int kbX = 32;
		int kbY = 720; // position verticale
		int keyW = 48;
		int keyH = 120;
		int blackW = (int)(keyW * 0.6f);
		int blackH = 72;

		// white keys order and labels
		const char whiteKeys[7] = {'q','w','e','r','t','y','u'};
		for (int i = 0; i < 7; ++i) {
			int x = kbX + i * keyW;
			int y = kbY;
			int k = whiteKeys[i];
			bool pressed = (activeKeys.find(k) != activeKeys.end());
			if (pressed) ofSetColor(200, 220, 255); else ofSetColor(255);
			ofDrawRectangle(x, y, keyW, keyH);
			ofSetColor(0);
			ofNoFill();
			ofDrawRectangle(x, y, keyW, keyH);
			// draw label
			ofFill();
			ofSetColor(0);
			char lab[4] = {(char)whiteKeys[i], 0, 0, 0};
			ofDrawBitmapString(std::string(lab), x + 6, y + keyH - 6);
		}

		// black keys mapping and positions (between white keys 0-1,1-2,3-4,4-5,5-6)
		const int blackIndices[5] = {0,1,3,4,5};
		const char blackKeys[5] = {'a','s','d','f','g'};
		for (int i = 0; i < 5; ++i) {
			int idx = blackIndices[i];
			int x = kbX + (int)((idx + 1) * keyW - blackW/2.0f);
			int y = kbY;
			int k = blackKeys[i];
			bool pressed = (activeKeys.find(k) != activeKeys.end());
			if (pressed) ofSetColor(120, 160, 255); else ofSetColor(0);
			ofDrawRectangle(x, y, blackW, blackH);
			// label on black key (small, white)
			ofSetColor(255);
			char lab[4] = {(char)blackKeys[i], 0, 0, 0};
			ofDrawBitmapString(std::string(lab), x + 6, y + 14);
		}

		ofPopStyle();
	}

}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){
	if (key == '-' || key == '_' ){
		volume -= 0.05;
		volume = std::max(volume, 0.f);
	} else if (key == '+' || key == '=' ){
		volume += 0.05;
		volume = std::min(volume, 1.f);
	}
	
	// Activation/désactivation du spectrogramme
	if (key == 'v' || key == 'V') {
		showSpectrogram = !showSpectrogram;
	}
	
	// Sélection de la forme d'onde
	if (key == '1') {
		formeOndeFloat = 0.0f;  // Sinusoïde
	} else if (key == '2') {
		formeOndeFloat = 1.0f;  // Carré
	} else if (key == '3') {
		formeOndeFloat = 2.0f;  // Dent de scie
	}
	
	// Interpolation fluide de la forme d'onde
	if (key == '<' || key == ',') {
		formeOndeFloat = std::max(formeOndeFloat - 0.1f, 0.0f);
	} else if (key == '>' || key == '.') {
		formeOndeFloat = std::min(formeOndeFloat + 0.1f, 2.0f);
	}
	
	// Contrôle de la brillance avec les flèches
	if (key == OF_KEY_UP) {
		brillance = std::min(brillance + 1.0f, 20.0f);
	} else if (key == OF_KEY_DOWN) {
		brillance = std::max(brillance - 1.0f, 1.0f);
	}
	
	// Activation/désactivation du mode clavier musical
	if (key == 'k' || key == 'K') {
		keyboardMode = !keyboardMode;
		if (!keyboardMode) {
			activeKeys.clear();  // Vider les touches actives
		}
	}
	
	// Changement d'octave
	if (keyboardMode) {
		if (key == '[') {
			octave = std::max(octave - 1, 0);
		} else if (key == ']') {
			octave = std::min(octave + 1, 8);
		}
		
		// Use notes.h mapping (handles both diatonic q..u and accidentals a/s/d/f/g)
		double noteFreq = freqFromKey(key, octave);
		if (noteFreq > 0.0) {
			activeKeys[key] = (float)noteFreq;
		}
	}
	
	if( key == 's' ){
		soundStream.start();
	}
	
	if( key == 'e' ){
		soundStream.stop();
	}

	// Play scale: press 'p'
	if (key == 'p' || key == 'P') {
		startPlayScale(octave, 0.5f); // default 0.5s per note
	}
	
}

//--------------------------------------------------------------
void ofApp::keyReleased  (int key){
	// Libérer la touche du clavier musical
	if (keyboardMode && activeKeys.find(key) != activeKeys.end()) {
		activeKeys.erase(key);
	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	int width = ofGetWidth();
	float height = (float)ofGetHeight();
	
	// Déplacement en abscisse (x) contrôle la fréquence (20 Hz à 20000 Hz)
	float widthPct = (float)x / (float)width;
	freq = 20.0f + (20000.0f - 20.0f) * widthPct;
	targetFrequency = freq;
	phaseAdderTarget = (targetFrequency / (float) sampleRate) * glm::two_pi<float>();
	
	// Déplacement en ordonnée (y) contrôle le volume (0 à 1)
	float heightPct = ((height - y) / height);
	volume = heightPct;
	volume = std::max(0.0f, std::min(volume, 1.0f));
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	int width = ofGetWidth();
	float height = (float)ofGetHeight();
	
	// Déplacement horizontal contrôle le pan
	pan = (float)x / (float)width;
	
	// Déplacement vertical contrôle la brillance
	float heightPct = ((height - y) / height);
	brillance = 1.0f + heightPct * 19.0f;  // Brillance de 1 à 20
	brillance = std::max(1.0f, std::min(brillance, 20.0f));
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	bNoise = true;
}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	bNoise = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer){
	float leftScale = 1 - pan;
	float rightScale = pan;

	while (phase > glm::two_pi<float>()){
		phase -= glm::two_pi<float>();
	}

	if ( bNoise == true){
		for (size_t i = 0; i < buffer.getNumFrames(); i++){
			lAudio[i] = buffer[i*buffer.getNumChannels()    ] = ofRandom(0, 1) * volume * leftScale;
			rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = ofRandom(0, 1) * volume * rightScale;
		}
	} else if (keyboardMode && !activeKeys.empty()) {
		// Mode clavier musical : mixer toutes les notes actives
		for (size_t i = 0; i < buffer.getNumFrames(); i++){
			float sample = 0.0f;
			
			// Mélanger toutes les notes actives
			for (const auto& keyFreq : activeKeys) {
				float noteFreq = keyFreq.second;
				
				// Utiliser le mixage fluide des formes d'onde
				float noteSample = calcul_mixage(1.0f, noteFreq, t, brillance, formeOndeFloat);
				
				// Ajouter cette note au mélange (divisé par le nombre de notes)
				sample += noteSample / (float)activeKeys.size();
			}
			
			lAudio[i] = buffer[i*buffer.getNumChannels()    ] = sample * volume * leftScale;
			rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = sample * volume * rightScale;
			
			// Incrémenter le temps par la durée d'un échantillon
			t += 1.0f / (float)sampleRate;
		}
	} else {
		// Mode souris : synthèse normale
		phaseAdder = 0.95f * phaseAdder + 0.05f * phaseAdderTarget;
		for (size_t i = 0; i < buffer.getNumFrames(); i++){
			phase += phaseAdder;

			float currentFreq = freq;
			// If a sequence is playing, override current freq with the sequence note
			if (isPlayingSequence && !sequenceFreqs.empty()) {
				// samplesPerNote must be > 0
				if (samplesPerNote <= 0) samplesPerNote = (int)(0.5f * sampleRate);
				currentFreq = sequenceFreqs[seqIndex];

				// advance counter and index when needed
				seqSampleCounter++;
				if (seqSampleCounter >= samplesPerNote) {
					seqSampleCounter = 0;
					seqIndex++;
					if (seqIndex >= (int)sequenceFreqs.size()) {
						// sequence finished
						isPlayingSequence = false;
						seqIndex = 0;
						// fall back to current mouse freq
						currentFreq = freq;
					}
				}
			}

			// Utiliser le mixage fluide des formes d'onde
			float sample = calcul_mixage(1.0f, currentFreq, t, brillance, formeOndeFloat);

			lAudio[i] = buffer[i*buffer.getNumChannels()    ] = sample * volume * leftScale;
			rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = sample * volume * rightScale;

			// Incrémenter le temps par la durée d'un échantillon
			t += 1.0f / (float)sampleRate;
		}
	}
}

//--------------------------------------------------------------
float ofApp::calc_sin(float A, float f, float t){
	// Calcule un échantillon de sinusoïde
	// A : amplitude
	// f : fréquence (Hz)
	// t : temps (en secondes)
	return A * sin(2.0f * glm::pi<float>() * f * t);
}

//--------------------------------------------------------------
float ofApp::calcul_carre(float A, float f, float t, float brillance){
	// Synthèse additive : signal carré
	// Somme de sinusoïdes impaires avec amplitudes décroissantes
	// Brillance : nombre d'harmoniques à ajouter
	float signal = 0.0f;
	int numHarmonics = (int)brillance;
	float fractionalHarmonics = brillance - numHarmonics;
	
	// Ajouter les harmoniques impaires
	for (int n = 1; n <= numHarmonics; n += 2) {
		float amplitude = 4.0f / (glm::pi<float>() * n);  // Amplitude de l'harmonique
		signal += amplitude * sin(2.0f * glm::pi<float>() * f * n * t);
	}
	
	// Ajouter partiellement la dernière harmonique pour interpolation douce
	if (fractionalHarmonics > 0.0f && numHarmonics % 2 == 0) {
		int nextOdd = numHarmonics + 1;
		float amplitude = 4.0f / (glm::pi<float>() * nextOdd) * fractionalHarmonics;
		signal += amplitude * sin(2.0f * glm::pi<float>() * f * nextOdd * t);
	}
	
	return A * signal;
}

//--------------------------------------------------------------
float ofApp::calcul_scie(float A, float f, float t, float brillance){
	// Synthèse additive : signal dent de scie
	// Somme de toutes les harmoniques avec amplitudes décroissantes
	// Brillance : nombre d'harmoniques à ajouter
	float signal = 0.0f;
	int numHarmonics = (int)brillance;
	float fractionalHarmonics = brillance - numHarmonics;
	
	// Ajouter toutes les harmoniques
	for (int n = 1; n <= numHarmonics; n++) {
		float amplitude = 2.0f / (glm::pi<float>() * n);  // Amplitude de l'harmonique
		signal += amplitude * sin(2.0f * glm::pi<float>() * f * n * t);
	}
	
	// Ajouter partiellement la dernière harmonique pour interpolation douce
	if (fractionalHarmonics > 0.0f) {
		int next = numHarmonics + 1;
		float amplitude = 2.0f / (glm::pi<float>() * next) * fractionalHarmonics;
		signal += amplitude * sin(2.0f * glm::pi<float>() * f * next * t);
	}
	
	return A * signal;
}

//--------------------------------------------------------------
float ofApp::calcul_mixage(float A, float f, float t, float brillance, float formeFloat){
	// Mixage lisse entre les différentes formes d'onde
	// formeFloat: 0 = sin, 0.5 = transition sin/carré, 1 = carré, 1.5 = transition carré/scie, 2 = scie
	
	float sample = 0.0f;
	
	if (formeFloat <= 1.0f) {
		// Transition entre sinusoïde (0) et carré (1)
		float alpha = formeFloat;  // 0 à 1
		
		if (alpha == 0.0f) {
			sample = calc_sin(A, f, t);
		} else if (alpha == 1.0f) {
			sample = calcul_carre(A, f, t, brillance);
		} else {
			// Interpolation linéaire entre sin et carré
			float sinSample = calc_sin(A, f, t);
			float carreSample = calcul_carre(A, f, t, brillance);
			sample = (1.0f - alpha) * sinSample + alpha * carreSample;
		}
	} else {
		// Transition entre carré (1) et dent de scie (2)
		float alpha = formeFloat - 1.0f;  // 0 à 1
		
		if (alpha == 0.0f) {
			sample = calcul_carre(A, f, t, brillance);
		} else if (alpha == 1.0f) {
			sample = calcul_scie(A, f, t, brillance);
		} else {
			// Interpolation linéaire entre carré et scie
			float carreSample = calcul_carre(A, f, t, brillance);
			float scieSample = calcul_scie(A, f, t, brillance);
			sample = (1.0f - alpha) * carreSample + alpha * scieSample;
		}
	}
	
	return sample;
}

//--------------------------------------------------------------
float ofApp::synthSample(){
	if (bNoise) {
		return ofRandom(-1.0f, 1.0f);
	} else {
		return sin(phase); // Signal sinusoïdal
	}
}

//--------------------------------------------------------------
void ofApp::computeSpectrum(const std::vector<float>& timeSignal, std::vector<float>& magnitude){
	if (timeSignal.size() != fftSize) return;
	
	// Appliquer la fenêtre et préparer le buffer FFT
	for (int i = 0; i < fftSize; i++) {
		fftBuffer[i] = std::complex<float>(timeSignal[i] * window[i], 0.0f);
	}
	
	// Calculer la FFT
	fft(fftBuffer);
	
	// Calculer la magnitude pour chaque bin de fréquence
	for (int i = 0; i < fftSize / 2; i++) {
		float real = fftBuffer[i].real();
		float imag = fftBuffer[i].imag();
		magnitude[i] = std::sqrt(real * real + imag * imag) / (fftSize / 2.0f);
	}
}

//--------------------------------------------------------------
void ofApp::fft(std::vector<std::complex<float>>& data){
	int n = data.size();
	if (n <= 1) return;
	
	// Diviser
	std::vector<std::complex<float>> even(n/2);
	std::vector<std::complex<float>> odd(n/2);
	
	for (int i = 0; i < n/2; i++) {
		even[i] = data[i * 2];
		odd[i] = data[i * 2 + 1];
	}
	
	// Conquérir
	fft(even);
	fft(odd);
	
	// Combiner
	for (int k = 0; k < n/2; k++) {
		std::complex<float> t = std::polar(1.0f, -2.0f * glm::pi<float>() * k / n) * odd[k];
		data[k] = even[k] + t;
		data[k + n/2] = even[k] - t;
	}
}

// Estimate the dominant frequency from a magnitude spectrum using parabolic interpolation around the peak
double ofApp::findDominantFrequency(const std::vector<float>& magnitude) {
	if (magnitude.empty()) return 0.0;
	int N = fftSize; // full FFT size
	int M = (int)magnitude.size(); // should be N/2

	// find peak bin
	int peak = 0;
	float maxVal = magnitude[0];
	for (int i = 1; i < M; ++i) {
		if (magnitude[i] > maxVal) {
			maxVal = magnitude[i];
			peak = i;
		}
	}

	if (peak <= 0 || peak >= M-1) {
		// peak at edge, return basic bin frequency
		double freq = (double)peak * (double)sampleRate / (double)N;
		return freq;
	}

	// parabolic interpolation to refine peak position
	double alpha = magnitude[peak-1];
	double beta  = magnitude[peak];
	double gamma = magnitude[peak+1];
	double p = 0.5 * (alpha - gamma) / (alpha - 2.0*beta + gamma); // interpolation offset
	double peakBin = peak + p;

	double frequency = peakBin * (double)sampleRate / (double)N;
	return frequency;
}

// Convert frequency to a note name (e.g. "C4", "D#3") using equal temperament A4=440Hz
std::string ofApp::freqToNoteName(double freq) {
	if (freq <= 0.0) return std::string("---");
	double midi = 69.0 + 12.0 * std::log2(freq / 440.0);
	int m = (int)std::round(midi);
	static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
	int nameIndex = (m % 12 + 12) % 12;
	int octave = m / 12 - 1;
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%s%d", names[nameIndex], octave);
	return std::string(buf);
}

std::pair<int,double> ofApp::nearestNoteAndCents(double freq) {
	if (freq <= 0.0) return std::make_pair(-1, 0.0);
	double midiExact = 69.0 + 12.0 * std::log2(freq / 440.0);
	int midiRound = (int)std::lround(midiExact);
	double cents = (midiExact - (double)midiRound) * 100.0; // positive if freq above nearest midi
	return std::make_pair(midiRound, cents);
}

// YIN algorithm implementation (simplified)
double ofApp::estimatePitchYIN(const std::vector<float>& x, double sr, double threshold) {
	int N = (int)x.size();
	if (N < 32) return 0.0; // too short

	int maxTau = N / 2;
	std::vector<double> d(maxTau + 1, 0.0);

	// 1) difference function
	for (int tau = 1; tau <= maxTau; ++tau) {
		double sum = 0.0;
		for (int j = 0; j < N - tau; ++j) {
			double diff = (double)x[j] - (double)x[j + tau];
			sum += diff * diff;
		}
		d[tau] = sum;
	}

	// 2) cumulative mean normalized difference
	std::vector<double> cmnd(maxTau + 1, 0.0);
	cmnd[0] = 1.0;
	double runningSum = 0.0;
	for (int tau = 1; tau <= maxTau; ++tau) {
		runningSum += d[tau];
		cmnd[tau] = d[tau] * tau / (runningSum + 1e-12);
	}

	// 3) absolute threshold: find first tau where cmnd < threshold
	int tauEstimate = -1;
	for (int tau = 2; tau <= maxTau; ++tau) {
		if (cmnd[tau] < threshold) {
			// choose local minimum
			while (tau + 1 <= maxTau && cmnd[tau + 1] < cmnd[tau]) tau++;
			tauEstimate = tau;
			break;
		}
	}

	if (tauEstimate == -1) return 0.0; // no pitch found

	// 4) parabolic interpolation using cmnd values around the estimate
	double betterTau = (double)tauEstimate;
	if (tauEstimate > 1 && tauEstimate < maxTau) {
		double x1 = cmnd[tauEstimate - 1];
		double x2 = cmnd[tauEstimate];
		double x3 = cmnd[tauEstimate + 1];
		double denom = (x1 + x3 - 2.0 * x2);
		if (std::abs(denom) > 1e-12) {
			double delta = 0.5 * (x1 - x3) / denom;
			betterTau = tauEstimate + delta;
		}
	}

	double pitch = sr / betterTau;
	return pitch;
}

void ofApp::startPlayScale(int octave, float noteDurationSeconds) {
	// Build sequence C D E F G for the requested octave
	static const int indices[] = {0, 2, 4, 5, 7}; // C D E F G
	sequenceFreqs.clear();
	for (int idx : indices) {
		int midi = 12 * (octave + 1) + idx;
		double f = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
		sequenceFreqs.push_back((float)f);
	}
	seqIndex = 0;
	seqSampleCounter = 0;
	samplesPerNote = std::max(1, (int)std::round(noteDurationSeconds * sampleRate));
	isPlayingSequence = true;
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}