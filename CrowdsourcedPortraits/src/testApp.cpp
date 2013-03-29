#define WATCHING_CROWD 0
#define CREATING_PORTRAIT 1
#define SCROLLPAPER 2
#define DRAWING_PORTRAIT 3
#define DRAWING_COMPLETE 4

#define PADDING 10
#define OUTPUT_LARGE_WIDTH 492
#define OUTPUT_LARGE_HEIGHT 364

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGTH 1080

#include "testApp.h"

using namespace cv;
using namespace ofxCv;

//--------------------------------------------------------------
void testApp::setup(){
    // VARIABLES
    currentState = WATCHING_CROWD;
    currentStateTitle = states[currentState];
    minCrowdSize = 3;
    sliceWidth = 200;
    
    // CONNECTIONS
    tspsReceiver.connect( 12000 );
    ofxAddTSPSListeners(this);
    people = tspsReceiver.getPeople();
    currentCrowdSize = people.size();
    
    // CAMERA & CV
    //ofSetVerticalSync(true);
	cam.initGrabber(VIDEO_WIDTH, VIDEO_HEIGTH);
    classifier.load(ofToDataPath("haarcascade_frontalface_alt2.xml"));
    scaleFactor = .25;
    graySmall.allocate(cam.getWidth() * scaleFactor, cam.getHeight() * scaleFactor, OF_IMAGE_GRAYSCALE);
    
    // CONTROL PANEL
    gui.setup();
    
	gui.addPanel("Crowd Settings");
    vector <guiVariablePointer> vars;
	vars.push_back( guiVariablePointer("Num People", &currentCrowdSize, GUI_VAR_INT) );
	vars.push_back( guiVariablePointer("State", &currentStateTitle, GUI_VAR_STRING) );
    gui.addVariableLister("Crowd Variables", vars);
    gui.addSlider("minCrowdSize", minCrowdSize, 3, 10, true);
	gui.loadSettings("crowdsettings.xml");

	gui.addPanel("Face Detect Settings");
    gui.addSlider("sliceWidth", sliceWidth, 100, 400, true);
	gui.loadSettings("facesettings.xml");
    
    // DISPLAY
    ofBackground(0);
}

//--------------------------------------------------------------
void testApp::update(){
    // Get control panel values ( TODO: everytime? )
    minCrowdSize = gui.getValueI("minCrowdSize");
    currentCrowdSize = people.size();
    currentStateTitle = states[currentState];
    
    cam.update();
    copy(cam, display);
    display.resize(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
    
    switch (currentState) {
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case WATCHING_CROWD:
        {
            // Values come in from TSPS
            people = tspsReceiver.getPeople();
            crowd.clear();
            
            // If min number present
            if (people.size() >= minCrowdSize) {
                
                for ( int i=0; i<people.size(); i++){
                    crowd.addVertex(people[i]->centroid.x*OUTPUT_LARGE_WIDTH, people[i]->centroid.y*OUTPUT_LARGE_HEIGHT);
                }
                crowd.close();
                
                // TODO: improve clustering and selection of leader
                // Get center and find nearest vertex
                groupCenter = crowd.getCentroid2D();
                
                int leaderPoint = 0;
                float minDistance = OUTPUT_LARGE_WIDTH;
                
                for ( int i=0; i<crowd.size(); i++){
                    float tempDist = ofDist(groupCenter.x, groupCenter.y, crowd[i].x, crowd[i].y);
                    if (tempDist < minDistance) {
                        minDistance = tempDist;
                        leaderPoint = i;
                    }
                }

                leaderOverheadPosition = crowd[leaderPoint];
                currentState++;
            }
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case CREATING_PORTRAIT:
        {
            if(cam.isFrameNew()) {
                convertColor(cam, gray, CV_RGB2GRAY);
                resize(gray, graySmall);
                Mat graySmallMat = toCv(graySmall);
                if(ofGetMousePressed()) {
                    equalizeHist(graySmallMat, graySmallMat);
                }
                graySmall.update();
                
                classifier.detectMultiScale(graySmallMat, objects, 1.06, 1,
                                            //CascadeClassifier::DO_CANNY_PRUNING |
                                            //CascadeClassifier::FIND_BIGGEST_OBJECT |
                                            //CascadeClassifier::DO_ROUGH_SEARCH |
                                            0);
                
                // Using webcam, look for face nearest location of Individual
                    
                // Is there a usable face? If not:
                // currentState = WATCHING_CROWD;
                // break;
                    
                // Process image
                // Do threshoding & look for min percentage black to white?
                    
                // B&W Image -> Vector graphics (using outlines or fill?)
                // currentState++;
            }
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case SCROLLPAPER:
        {
            // Scroll the paper into place
            // Delay until complete
            // currentState++;
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case DRAWING_PORTRAIT:
        {
            // Do the drawing
            // When complete
            // currentState++;
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case DRAWING_COMPLETE:
        {
            // Wait
            // currentState++;
        }
        break;
    }
    // COMPLETE? THEN START OVER
    if (currentState > DRAWING_COMPLETE) currentState = WATCHING_CROWD;
}

//--------------------------------------------------------------
void testApp::draw(){
    // DRAW BASIC LAYOUT
    ofSetColor(255);
    ofNoFill();
    ofRect(0,0,OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
    
    // DRAW CAM ALL THE TIME?
    //display.draw(OUTPUT_LARGE_WIDTH+PADDING, 0);
    tspsReceiver.draw(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
    cam.draw(0,0);
    
    switch (currentState) {
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case WATCHING_CROWD:
        {
            //tspsReceiver.draw(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
            if (crowd.size() > 0) {
                ofSetColor(0, 0, 255);
                crowd.draw();
                ofSetColor(yellowPrint);
                ofCircle(groupCenter, 15);
                ofSetColor(255);
                ofCircle(leaderOverheadPosition, 30);
                ofSetColor(255);
                ofCircle(crowd.getClosestPoint(groupCenter), 5);
            }
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case CREATING_PORTRAIT:
        {
            
            ofNoFill();
            
            //ofPushMatrix();
            ofScale(1 / scaleFactor, 1 / scaleFactor);
            //ofTranslate()
            for(int i = 0; i < objects.size(); i++) {
                ofRect(toOf(objects[i]));
            }
            //ofPopMatrix()
            ofDrawBitmapString(ofToString(objects.size()), 10, 20);
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case SCROLLPAPER:
        {
            //
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case DRAWING_PORTRAIT:
        {
            //
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case DRAWING_COMPLETE:
        {
            //
        }
        break;
    }
    // Debug / diagnostics
    // tspsReceiver.status
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
    
}

//--------------------------------------------------------------
void testApp::onPersonEntered( ofxTSPS::EventArgs & tspsEvent ){
    //ofLog(OF_LOG_NOTICE, "New person!");
    // you can access the person like this:
    // tspsEvent.person
}

//--------------------------------------------------------------
void testApp::onPersonUpdated( ofxTSPS::EventArgs & tspsEvent ){
    //ofLog(OF_LOG_NOTICE, "Person updated!");
    // you can access the person like this:
    // tspsEvent.person
    
}

//--------------------------------------------------------------
void testApp::onPersonWillLeave( ofxTSPS::EventArgs & tspsEvent ){
    //ofLog(OF_LOG_NOTICE, "Person left!");
    // you can access the person like this:
    // tspsEvent.person
    
}