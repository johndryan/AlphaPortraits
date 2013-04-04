#pragma once

#include "ofMain.h"
#include "ofxTSPSReceiver.h"
#include "ofxAutoControlPanel.h"
#include "ofxCv.h"
#include "ofMath.h"

#include "imatrix.h"
#include "ETF.h"
#include "fdog.h"
#include "myvec.h"

class testApp : public ofBaseApp{
    
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
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    ofxTSPS::Receiver tspsReceiver;
    void onPersonEntered( ofxTSPS::EventArgs & tspsEvent );
    void onPersonUpdated( ofxTSPS::EventArgs & tspsEvent );
    void onPersonWillLeave( ofxTSPS::EventArgs & tspsEvent );
    
    ofxAutoControlPanel gui;
    
    ofVideoGrabber cam;
	ofxCv::RunningBackground background;
    ofImage display, gray, graySmall, face, faceThresholded;
	ofImage backgroundSub, cld, thresholded, thinned, io, jk, nm, sourceImg, sourceImgGray, croppedBackground;
    imatrix img;
	ETF etf;
	ofxCv::ContourFinder contourFinder;
	
	cv::CascadeClassifier classifier;
	vector<cv::Rect> objects;
	float scaleFactor;
    bool backgroundSubtraction;
    
    int currentState, minCrowdSize, currentCrowdSize, sliceWidth, croppedFaceSize, backgroundThresholdValue, backgroundLearningTime;
    string currentStateTitle;
    vector<ofxTSPS::Person*> people;
    ofPoint leaderOverheadPosition, groupCenter;
    ofPolyline crowd;
    string states[5] = { "WATCHING CROWD", "CREATING PORTRAIT", "SCROLLPAPER", "DRAWING PORTRAIT", "DRAWING COMPLETE" };
    
    vector<ofPolyline> shading;
    vector<ofPoint> linePoints;
    ofTessellator tess;
};
