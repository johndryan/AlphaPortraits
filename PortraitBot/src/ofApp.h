#pragma once

#include "ofMain.h"
#include "ofxAutoControlPanel.h"
#include "ofxCv.h"

#include "imatrix.h"
#include "ETF.h"
#include "fdog.h"
#include "myvec.h"

#include "arduinoThread.h"

#include "ofxSimpleSerial.h"
#include "Instruction.h"

class ofApp : public ofBaseApp{
public:
    void setup();
	void update();
	void draw();
	void drawPaths();
    void pathsToInstructions();
    void setTarget ();
	void keyPressed(int key);
    void exit();
    
    int getPoints(int steps);
    
    imatrix img;
	ETF etf;
    ofxAutoControlPanel gui;
	ofVideoGrabber cam;
	ofImage gray, cld, thresholded, thinned, io, jk, nm, sourceImg, sourceImgGray;
    ofImage graySmall, cropped;
    cv::CascadeClassifier classifier;
    arduinoThread AT;
    
    vector<cv::Rect> objects;
	vector<ofPolyline> paths;
    ofPolyline ln;

    float timer, timeout;
    
	float faceTrackingScaleFactor;
    int croppedSize;
    int camWidth, camHeight;
	bool needToUpdate;
    
    float minGapLength;
    int minPathLength;
    
    ofxSimpleSerial	serial;
    int nInstructions, currentInstruction;
    float startX, startY, scaleFactor, max_X, min_X, max_Y, min_Y;
    bool currentlyPlotting, plotterReady, runOnTimer, firstLastDraw, randomLoc;
    string      message, serialID;
    void		onNewMessage(string & message);
    
    ofTrueTypeFont font;
};
