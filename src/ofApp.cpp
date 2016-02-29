#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	//ofSetVerticalSync(false);
	ofEnableAlphaBlending();
	ofEnableSmoothing();
	glEnable(GL_DEPTH_TEST);
	glPointSize(3);
	glLineWidth(2);

	// Setup video feed
	camWidth = 160;
	camHeight = 120;
	video.setVerbose(true);
	video.initGrabber(camWidth, camHeight);

	// Build video plane (and copy for quick initial vert lookup)
	mesh = ofMesh().plane(camWidth, camHeight, camWidth, camHeight);
	blackout(mesh);
	meshInit = ofMesh(mesh);

	// Position camera in +z, which will conveniently mirror the mesh
	camera.setAutoDistance(false);
	camera.setPosition(0.0f, 0.0f, 150.0f);
	
	ofSetFullscreen(true);
	setupGUI();
}

//--------------------------------------------------------------
void ofApp::update() {
	// Drive mesh from video feed as new frames are available
	video.update();
	if (video.isFrameNew()) {
		unsigned char* pixels = video.getPixels();
		ofVec3f *vertices = mesh.getVerticesPointer();
		ofVec3f *verticesInit = meshInit.getVerticesPointer();
		ofFloatColor *colors = mesh.getColorsPointer();

		// Scale and translate meshes to account for sparsity change, keeping
		// corner block verts centered at origin.
		float newStep = floor(sparsity->getValue() + 0.05f);
		if (newStep != step) {
			float scale = newStep / step;
			ofVec3f scaleXY = ofVec3f(scale, scale, 1.0f);
			ofVec3f translateXY = ofVec3f(
				(camWidth - camWidth * scale) * 0.5f, 
				(camHeight - camHeight * scale) * 0.5f, 
				0.0f);
			for (int i = 0; i < mesh.getNumVertices(); i++) {
				verticesInit[i] *= scaleXY;
				verticesInit[i] += translateXY;
				vertices[i] = verticesInit[i];

				// Blackout color and alpha
				colors[i] = BLACKOUT;
			}
			step = newStep;
		}

		// "Viscosity" defines a lerp amount from the previous frame's data,
		// which can create some cool effects.
		float visc = 1.0f - viscosity->getValue();

		// Iterate pixels (except for outer edges, to make a soft mesh boundry)
		int xStride = 3 * step;
		int yStride = step;
		for (int y = yStride; y < camHeight - yStride; y += yStride) {  
			for (int x = xStride; x < camWidth*3 - xStride; x += xStride) {
				int p = y * camWidth*3 + x;
				ofFloatColor rgba(pixels[p], pixels[p + 1], pixels[p + 2]);
				rgba /= 255.0f;

				// Determine ref vert index w/ sparsity:
				//   -Vert block taken from upper corner of larger mesh
				//   -Verts iterated in reverse to flip the video feed
				int i = (mesh.getNumVertices() - 1) - ((y / yStride * camWidth) + (x / xStride));
				ofVec3f vertRef = verticesInit[i];
				float distance = vertRef.length();

				// Interpolate with previous frame's color
				rgba.r = ofLerp(colors[i].r, rgba.r, visc);
				rgba.g = ofLerp(colors[i].g, rgba.g, visc);
				rgba.b = ofLerp(colors[i].b, rgba.b, visc);

				// Get input channel value from color
				float input;
				string channelName = channel->getActiveName();
				if (channelName == "HUE")
					input = rgba.getHue();
				else if (channelName == "SATURATION")
					input = rgba.getSaturation();
				else if (channelName == "BRIGHTNESS")
					input = rgba.getBrightness();
				else if (channelName == "LIGHTNESS")
					input = rgba.getLightness();
				else if (channelName == "RED")
					input = rgba.r;
				else if (channelName == "GREEN")
					input = rgba.g;
				else if (channelName == "BLUE")
					input = rgba.b;
				if (invert->getValue())
					input = 1.0f - input;
				input *= sensitivity->getValue();

				ofVec3f vertOut;

				// Displace
				float displaceCurved = curveValue(input, displaceFalloff->getValue(), displace->getValue());
				vertOut.z = displaceCurved * 2.0f;

				// Vortex
				float vortexCurved = curveValue(input, vortexFalloff->getValue(), vortex->getValue());
				float angle = atan2(vertRef.y, vertRef.x);
				float speed = ofMap(vortexCurved, vortex->getMax(), vortex->getMin(), 1.0f, -1.0f, true) * 5.0f;
				float rotatedAngle = speed + angle;
				vertOut.x = distance * cos(rotatedAngle);
				vertOut.y = distance * sin(rotatedAngle);

				// Noise
				float noiseCurved = curveValue(input, noiseFalloff->getValue(), noise->getValue()) * 0.25f;
				vertOut.x = ofRandom(vertOut.x - noiseCurved, vertOut.x + noiseCurved);
				vertOut.y = ofRandom(vertOut.y - noiseCurved, vertOut.y + noiseCurved);
				vertOut.z = ofRandom(vertOut.z - noiseCurved, vertOut.z + noiseCurved);
				
				// Alpha driven by final vert depth
				//rgba.a = ofLerp(colors[i].a, vertices[i].z / displace->getValue(), visc);

				vertices[i].interpolate(vertOut, visc);
				colors[i] = rgba;
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofClear(ofColor::black);
	ofbClearBg();

	camera.begin();
	string rendererName = renderer->getActiveName();
	if (rendererName == "POINT CLOUD") {
		glShadeModel(GL_FLAT);
		mesh.drawVertices();
	}
	if (rendererName == "WIREFRAME") {
		glShadeModel(GL_SMOOTH);
		mesh.drawWireframe();
	}
	if (rendererName == "SOLID") {
		glShadeModel(GL_FLAT);
		mesh.draw();
	}
	camera.end();

	//ofDrawBitmapString(ofToString((int)ofGetFrameRate()) + " fps", 10, 20);
}

//--------------------------------------------------------------
void ofApp::blackout(ofMesh &mesh) {
	for (int i = 0; i < mesh.getNumVertices(); i++) {
		mesh.addColor(BLACKOUT);
	}
}

//--------------------------------------------------------------
float ofApp::curveValue(float input, float gamma, float scalar) {
	return pow(input, 1.0f / (1.0f - gamma)) * scalar;
}

//--------------------------------------------------------------
void ofApp::setupGUI() {
	gui = new ofxUISuperCanvas("PRIMATE");
	gui->setTheme(OFX_UI_THEME_METALGEAR);
	gui->setWidgetFontSize(OFX_UI_FONT_SMALL);

	gui->addSpacer();
	resetButton = gui->addLabelButton("RESET", false);
	randomizeButton = gui->addLabelButton("RANDOMIZE", false);

	gui->addSpacer();
	sparsity = gui->addSlider("SPARSITY", 1.0f, 10.0f, 1.0f);
	sparsity->setStickyValue(1.0);
	sparsity->enableSticky(true);
	step = sparsity->getDefaultValue();
	sensitivity = gui->addSlider("SENSITIVITY", 0.0f, 1.0f, 0.5f);
	viscosity = gui->addSlider("VISCOSITY", 0.0f, 1.0f,  0.75f);
	displace = gui->addSlider("DISPLACE", 10.0f, 100.0f, 25.0f);
	displaceFalloff = gui->addSlider("DISPLACE FALLOFF", 0.0f, 1.0f, 0.0f);
	vortex = gui->addSlider("VORTEX", -100.0f, 100.0f, 0.0f);
	vortexFalloff = gui->addSlider("VORTEX FALLOFF", 0.0f, 1.0f, 0.0f);
	noise = gui->addSlider("NOISE", 0.0f, 100.0f, 0.0f);
	noiseFalloff = gui->addSlider("NOISE FALLOFF", 0.0f, 1.0f, 0.0f);
	invert = gui->addToggle("INVERT", false);

	gui->addSpacer();
	gui->addLabel("CHANNEL", OFX_UI_FONT_SMALL);
	channels.push_back("HUE");
	channels.push_back("SATURATION");
	channels.push_back("BRIGHTNESS");
	channels.push_back("LIGHTNESS");
	channels.push_back("RED");
	channels.push_back("GREEN");
	channels.push_back("BLUE");
	channel = gui->addRadio("CHANNEL", channels, "BRIGHTNESS", OFX_UI_ORIENTATION_VERTICAL);

	gui->addSpacer();
	gui->addLabel("RENDERER", OFX_UI_FONT_SMALL);
	renderers.push_back("POINT CLOUD");
	renderers.push_back("WIREFRAME");
	renderers.push_back("SOLID");
	renderer = gui->addRadio("RENDERER", renderers, "WIREFRAME", OFX_UI_ORIENTATION_VERTICAL);

	gui->addSpacer();
	gui->addLabel("SNAPSHOT", OFX_UI_FONT_SMALL);
	filename = gui->addTextInput("NAME", "NAME");
	saveSnapshotButton = gui->addLabelButton("SAVE", false);
	loadRandomSnapshotButton = gui->addLabelButton("LOAD RANDOM", false);

	gui->addSpacer();
	gui->autoSizeToFitWidgets();
	ofAddListener(gui->newGUIEvent, this, &ofApp::guiEvent);
}

//--------------------------------------------------------------
void ofApp::resetGUI() {
	vector<ofxUIWidget *> widgets = gui->getWidgets();
	for (int i = 0; i < widgets.size(); i++) {
		ofxUIWidget *widget = widgets[i];
		int kind = widget->getKind();

		if (kind == OFX_UI_WIDGET_SLIDER_H) {
			ofxUISlider *slider = (ofxUISlider *) widget;
			slider->resetValue();
		}
		else if (kind == OFX_UI_WIDGET_RADIO) {
			ofxUIRadio *radio = (ofxUIRadio *) widget;
			radio->resetActive();
		}
		else if (kind == OFX_UI_WIDGET_TOGGLE) {
			ofxUIToggle *toggle = (ofxUIToggle *) widget;
			ofxUIWidget *parent = toggle->getParent();
			if (parent && parent->getKind() == OFX_UI_WIDGET_RADIO) {
				continue;  // Radios already reset
			}
			toggle->resetValue();
		}
	}
}

//--------------------------------------------------------------
void ofApp::randomizeGUI() {
	vector<ofxUIWidget *> widgets = gui->getWidgets();
	for (int i = 0; i < widgets.size(); i++) {
		ofxUIWidget *widget = widgets[i];
		int kind = widget->getKind();

		if (kind == OFX_UI_WIDGET_SLIDER_H) {
			ofxUISlider *slider = (ofxUISlider *) widget;
			slider->randomizeValue();
		}
		else if (kind == OFX_UI_WIDGET_RADIO) {
			ofxUIRadio *radio = (ofxUIRadio *) widget;
			radio->randomizeActive();
		}
		else if (kind == OFX_UI_WIDGET_TOGGLE) {
			ofxUIToggle *toggle = (ofxUIToggle *) widget;
			ofxUIWidget *parent = toggle->getParent();
			if (parent && parent->getKind() == OFX_UI_WIDGET_RADIO) {
				continue;  // Radios already reset
			}
			toggle->randomizeValue();
		}
	}
}

//--------------------------------------------------------------
void ofApp::saveSnapshot() {
	// Hide GUI for snapshot
	gui->toggleVisible();
	draw();

	string name = filename->getTextString();
	// Remove extension, if any
	if (name.find('.') != string::npos) {
		vector<string> split = ofSplitString(name, ".");
		split.pop_back();
		name = ofJoinString(split, ".");
	}
	// Generate name if none entered
	if (name == filename->getDefaultTextString()) {
		name = ofGetTimestampString();
	}

	// Reset filename and save settings
	filename->resetTextString();
	gui->saveSettings(name + ".xml");

	// Save image
	ofImage snapshot;
	snapshot.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	snapshot.saveImage(name + ".png");

	gui->toggleVisible();
}

//--------------------------------------------------------------
void ofApp::loadRandomSnapshot() {
	// Get all snapshot settings files and randomly load one
	vector<string> files;
	ofDirectory dir;
	dir.listDir(ofToDataPath(""));
	for (int i = 0; i < (int)dir.size(); i++) {
		string file = dir.getName(i);
		int length = file.length();
		if (length > 3 && file.substr(length - 3, length) == "xml") {
			files.push_back(file);
		}
	}
	if (files.size()) {
		gui->loadSettings(files[ofRandom(0, files.size())]);
	}
}

//--------------------------------------------------------------
void ofApp::guiEvent(ofxUIEventArgs &e) {
	if (e.getKind() == OFX_UI_WIDGET_LABELBUTTON) {
		ofxUILabelButton *button = (ofxUILabelButton *) e.widget;
		if (button->getValue()) {
			if (button == resetButton) {
				resetGUI();
			}
			else if (button == randomizeButton) {
				randomizeGUI();
			}
			else if (button == loadRandomSnapshotButton) {
				loadRandomSnapshot();
			}
			else if (button == saveSnapshotButton) {
				saveSnapshot();
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	// Disable camera arcball during UI interaction
	ofxUIRectangle *rect = gui->getRect();
	if (x > rect->x && x <= rect->x + rect->width && 
			y > rect->y && y <= rect->y + rect->height) {
		camera.disableMouseInput();
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	// Re-enabled camera arcball after UI interaction
	if (!camera.getMouseInputEnabled())
		camera.enableMouseInput();
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) { 

}