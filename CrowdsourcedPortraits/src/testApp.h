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

//#include "ofxSimpleSerial.h"
#include "Instruction.h"

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
    void pathsToInstructions();
	void drawPaths();
    
    ofxTSPS::Receiver tspsReceiver;
    void onPersonEntered( ofxTSPS::EventArgs & tspsEvent );
    void onPersonUpdated( ofxTSPS::EventArgs & tspsEvent );
    void onPersonWillLeave( ofxTSPS::EventArgs & tspsEvent );
    
    ofxAutoControlPanel gui;
    ofTrueTypeFont font;
    
    ofVideoGrabber cam;
	ofxCv::RunningBackground background;
    ofImage display, gray, graySmall, face, faceThresholded, tspsGrab, backgroundSub, cld, thresholded, thinned, io, jk, nm, sourceImg, sourceImgGray, croppedBackground;
    imatrix img;
	ETF etf;
	ofxCv::ContourFinder contourFinder, backgroundContourFinder;
	
	cv::CascadeClassifier classifier;
	vector<cv::Rect> objects;
	float scaleFactor;
    bool backgroundSubtraction, flipVideo;
    
    int currentState, drawingState, minCrowdSize, currentCrowdSize, sliceWidth, croppedFaceSize, backgroundThresholdValue, backgroundLearningTime, roiSize, leftCamCal, rightCamCal;
    float leaderPosAdjusted, frontScaleFactor, backScaleFactor, distScale;
    string currentStateTitle;
    vector<ofxTSPS::Person*> people;
    ofPoint leaderOverheadPosition, groupCenter;
    ofPolyline crowd;
    string states[6] = { "WATCHING CROWD", "CREATING PORTRAIT", "CONFIRM PORTRAIT", "SCROLLPAPER", "DRAWING PORTRAIT", "DRAWING COMPLETE" };
    
    vector<ofPolyline> shading, paths;
    vector<ofPoint> linePoints;
    ofTessellator tess;
    
    //ofxSimpleSerial	serial;
    int nInstructions, currentInstruction, plotMinX, plotMaxX, plotMinY, plotMaxY, plotScaleFactor;
    bool currentlyPlotting, plotterReady, firstLastDraw, shadingOn, drawingOn;
    string      message, serialID;
    void		onNewMessage(string & message);
    vector<Instruction> instructions;
};
