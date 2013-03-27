#define WATCHING_CROWD 0
#define CREATING_PORTRAIT 1
#define SCROLLPAPER 2
#define DRAWING_PORTRAIT 3
#define DRAWING_COMPLETE 4

#include "testApp.h"

using namespace cv;
using namespace ofxCv;

//--------------------------------------------------------------
void testApp::setup(){
    // VARIABLES
    currentState = WATCHING_CROWD;
    
    // CONNECTIONS
    tspsReceiver.connect( 12000 );
    ofxAddTSPSListeners(this);
    
    // DISPLAY
    ofBackground(0);
}

//--------------------------------------------------------------
void testApp::update(){
    switch (currentState) {
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case WATCHING_CROWD:
        {
            // Values come in from TSPS
            vector<ofxTSPS::Person*> people = tspsReceiver.getPeople();
            for ( int i=0; i<people.size(); i++){
                ofLog(OF_LOG_NOTICE, "PERSON");
            // do stuff for each person!
            //people[i]->contour...
            }
            // Get directions / clustering
            
            // Individual selected?
            // currentState++;
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case CREATING_PORTRAIT:
        {
            // Using webcam, look for face nearest location of Individual
                
            // Is there a usable face? If not:
            // currentState = WATCHING_CROWD;
            // break;
                
            // Process image
            // Do threshoding & look for min percentage black to white?
                
            // B&W Image -> Vector graphics (using outlines or fill?)
            // currentState++;
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
    switch (currentState) {
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case WATCHING_CROWD:
        {
            tspsReceiver.draw(ofGetWidth(), ofGetHeight());
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case CREATING_PORTRAIT:
        {
            //
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
    ofLog(OF_LOG_NOTICE, "New person!");
    // you can access the person like this:
    // tspsEvent.person
}

//--------------------------------------------------------------
void testApp::onPersonUpdated( ofxTSPS::EventArgs & tspsEvent ){
    ofLog(OF_LOG_NOTICE, "Person updated!");
    // you can access the person like this:
    // tspsEvent.person
    
}

//--------------------------------------------------------------
void testApp::onPersonWillLeave( ofxTSPS::EventArgs & tspsEvent ){
    ofLog(OF_LOG_NOTICE, "Person left!");
    // you can access the person like this:
    // tspsEvent.person
    
}