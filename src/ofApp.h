#pragma once

#include "ofMain.h"
#include "ofxEasyCam.h"
#include "ofxUI.h"

const ofFloatColor BLACKOUT = ofFloatColor(0.0f, 0.0f);

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseDragged(int x, int y, int button);
		void mouseMoved(int x, int y );
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);

		void blackout(ofMesh &mesh);
		float curveValue(float input, float gamma, float scalar);
		void guiEvent(ofxUIEventArgs &e);
		void randomizeGUI();
		void resetGUI();
		void setupGUI();
		void saveSnapshot();
		void loadRandomSnapshot();
		
		ofVideoGrabber video;
		ofMesh mesh;
		ofMesh meshInit;
		int camWidth;
		int camHeight;
		ofxEasyCam camera;
		
		float step;
		vector<string> channels;
		vector<string> renderers;

		// GUI
		ofxUISuperCanvas *gui;
		ofxUISlider *sparsity;
		ofxUISlider *sensitivity;
		ofxUISlider *viscosity;
		ofxUISlider *displace;
		ofxUISlider *displaceFalloff;
		ofxUISlider *vortex;
		ofxUISlider *vortexFalloff;
		ofxUISlider *noise;
		ofxUISlider *noiseFalloff;
		ofxUIToggle *invert;
		ofxUIRadio *channel;
		ofxUIRadio *renderer;
		ofxUILabelButton *resetButton;
		ofxUILabelButton *randomizeButton;
		ofxUILabelButton *loadRandomSnapshotButton;
		ofxUITextInput *filename;
		ofxUILabelButton *saveSnapshotButton;

};
