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

#define SCALE_FACTOR 0.1
#define CROPPED_FACE_SIZE 512
#define CONTOUR_MIN_AREA 5

#include "testApp.h"

using namespace cv;
using namespace ofxCv;

//--------------------------------------------------------------
void removeIslands(ofPixels& img) {
	int w = img.getWidth(), h = img.getHeight();
	int ia1=-w-1,ia2=-w-0,ia3=-w+1,ib1=-0-1,ib3=-0+1,ic1=+w-1,ic2=+w-0,ic3=+w+1;
	unsigned char* p = img.getPixels();
	for(int y = 1; y + 1 < h; y++) {
		for(int x = 1; x + 1 < w; x++) {
			int i = y * w + x;
			if(p[i]) {
				if(!p[i+ia1]&&!p[i+ia2]&&!p[i+ia3]&&!p[i+ib1]&&!p[i+ib3]&&!p[i+ic1]&&!p[i+ic2]&&!p[i+ic3]) {
					p[i] = 0;
				}
			}
		}
	}
}

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
    scaleFactor = SCALE_FACTOR;
	croppedFaceSize = CROPPED_FACE_SIZE;
    // TODO: Make face non-square?
    
    //ofSetVerticalSync(true);
	cam.initGrabber(VIDEO_WIDTH, VIDEO_HEIGTH);
    classifier.load(ofToDataPath("haarcascade_frontalface_alt2.xml"));
    graySmall.allocate(cam.getWidth() * scaleFactor, cam.getHeight() * scaleFactor, OF_IMAGE_GRAYSCALE);
	face.allocate(croppedFaceSize, croppedFaceSize, OF_IMAGE_GRAYSCALE);
	img.init(croppedFaceSize, croppedFaceSize);
	imitate(gray, face);
	imitate(cld, face);
	imitate(thresholded, face);
	imitate(thinned, face);
    
	contourFinder.setMinAreaRadius(CONTOUR_MIN_AREA);
	contourFinder.setMaxAreaRadius(croppedFaceSize*3/4);
    
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
	gui.addSlider("black", 42, -255, 255, true);
	gui.addSlider("sigma1", 0.99, 0.01, 2.0, false);
	gui.addSlider("sigma2", 4.45, 0.01, 10.0, false);
	gui.addSlider("tau", 0.97, 0.8, 1.0, false);
	gui.addSlider("halfw", 4, 1, 8, true);
	gui.addSlider("smoothPasses", 4, 1, 4, true);
	gui.addSlider("thresh", 121.8, 0, 255, false);
	//gui.addSlider("minGapLength", 5.5, 2, 12, false);
	//gui.addSlider("minPathLength", 40, 0, 50, true);
	gui.addSlider("facePadding", 1.5, 0, 2, false); 
    gui.addSlider("verticalOffset", int(-croppedFaceSize/12), int(-croppedFaceSize/2), int(croppedFaceSize/2), false);
    
    gui.addSlider("contourThreshold", 0, 0, 255, true);
    gui.addSlider("contourMinAreaRadius", 5, 1, 50, true);
    gui.addSlider("contourMaxAreaRadius", croppedFaceSize*3/4, croppedFaceSize/2, croppedFaceSize, true);
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
                ofLog(OF_LOG_NOTICE, "== STATE: CREATING PORTRAIT ==");
            }
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case CREATING_PORTRAIT:
        {
            // Most of this OpenCV stuff is taken from: http://github.com/kylemcdonald/BaristaBot
            
            cam.update();
            copy(cam, display);
            display.resize(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
            
            if(cam.isFrameNew()) {
                int black = gui.getValueI("black");
                float sigma1 = gui.getValueF("sigma1");
                float sigma2 = gui.getValueF("sigma2");
                float tau = gui.getValueF("tau");
                float thresh = gui.getValueF("thresh");
                int halfw = gui.getValueI("halfw");
                int smoothPasses = gui.getValueI("smoothPasses");
                //minGapLength = gui.getValueF("minGapLength");
                //minPathLength = gui.getValueI("minPathLength");
                float facePadding = gui.getValueF("facePadding");
                int verticalOffset = gui.getValueI("verticalOffset");
                
                contourFinder.setMinAreaRadius(gui.getValueI("contourMinAreaRadius"));
                contourFinder.setMaxAreaRadius(gui.getValueI("contourMaxAreaRadius"));
                
                // TODO: Set OpenCV ROI by information from overhead cam
                
                // TODO: Background subtraction to clean up?
                
                convertColor(cam, gray, CV_RGB2GRAY);
                resize(gray, graySmall);
                Mat graySmallMat = toCv(graySmall);
                equalizeHist(graySmallMat, graySmallMat);
                graySmall.update(); // TODO: Needed ??
                
                classifier.detectMultiScale(graySmallMat, objects, 1.06, 1,
                                            CASCADE_DO_CANNY_PRUNING |
                                            CASCADE_FIND_BIGGEST_OBJECT |
                                            //CASCADE_DO_ROUGH_SEARCH |
                                            0);
                
                ofRectangle faceRect;
                if(objects.empty()) {
                    // No Face. Start over;
                    currentState = 10;
                    break;
                } else {
                    ofLog(OF_LOG_NOTICE, "---- Face Found ----");
                    faceRect = toOf(objects[0]);
                    faceRect.getPositionRef() /= scaleFactor;
                    faceRect.scale(1 / scaleFactor);
                    faceRect.scaleFromCenter(facePadding);
                    faceRect.translateY(verticalOffset);
                }
                
                // Process Captured Face Image
                
                ofRectangle camBoundingBox(0, 0, cam.getWidth(), cam.getHeight());
                faceRect = faceRect.getIntersection(camBoundingBox);
                float whDiff = fabsf(faceRect.width - faceRect.height);
                if(faceRect.width < faceRect.height) {
                    faceRect.height = faceRect.width;
                    faceRect.y += whDiff / 2;
                } else {
                    faceRect.width = faceRect.height;
                    faceRect.x += whDiff / 2;
                }
                
                cv::Rect roi = toCv(faceRect);
                Mat grayMat = toCv(gray);
                Mat croppedGrayMat(grayMat, roi);
                resize(croppedGrayMat, face);
                face.update();
                
                int j = 0;
                unsigned char* grayPixels = face.getPixels();
                for(int y = 0; y < croppedFaceSize; y++) {
                    for(int x = 0; x < croppedFaceSize; x++) {
                        img[x][y] = grayPixels[j++] - black;
                    }
                }
                etf.init(croppedFaceSize, croppedFaceSize);
                etf.set(img);
                etf.Smooth(halfw, smoothPasses);
                GetFDoG(img, etf, sigma1, sigma2, tau);
                j = 0;
                unsigned char* cldPixels = cld.getPixels();
                for(int y = 0; y < croppedFaceSize; y++) {
                    for(int x = 0; x < croppedFaceSize; x++) {
                        cldPixels[j++] = img[x][y];
                    }
                }
                threshold(cld, thresholded, thresh, true);
                copy(thresholded, thinned);
                thin(thinned);
                removeIslands(thinned.getPixelsRef());
                
                gray.update();
                cld.update();
                thresholded.update();
                thinned.update();
                    
                contourFinder.setThreshold(ofMap(mouseX, 0, ofGetWidth(), 0, 255));
                contourFinder.findContours(thresholded);
                
                // Do threshoding & look for min percentage black to white?
                    
                // B&W Image -> Vector graphics (using outlines or fill?)
                currentState++;
                ofLog(OF_LOG_NOTICE, "== STATE: SCROLLING PAPER ==");
            }
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case SCROLLPAPER:
        {
            // Scroll the paper into place
            // Delay until complete
            // currentState++;
            // ofLog(OF_LOG_NOTICE, "== STATE: DRAWING PORTRAIT ==");
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case DRAWING_PORTRAIT:
        {
            // Do the drawing
            // When complete
            // currentState++;
            // ofLog(OF_LOG_NOTICE, "== STATE: DRAWING COMPLETE ==");
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
    if (currentState > DRAWING_COMPLETE) {
        currentState = WATCHING_CROWD;
        ofLog(OF_LOG_NOTICE, "== STATE: WATCHING CROWD ==");
    }
}

//--------------------------------------------------------------
void testApp::draw(){
    // DRAW BASIC LAYOUT
    ofSetColor(255);
    ofNoFill();
    ofRect(1,1,OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
    ofRect(OUTPUT_LARGE_WIDTH+PADDING,1,OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
    
    // DRAW ALL THE TIME
    display.draw(OUTPUT_LARGE_WIDTH+PADDING, 0);
    face.draw(0, OUTPUT_LARGE_HEIGHT+PADDING);
    int x = 0;
    cld.draw((x+=face.getHeight()), OUTPUT_LARGE_HEIGHT+PADDING);
    thresholded.draw((x+=cld.getHeight()), OUTPUT_LARGE_HEIGHT+PADDING);
    thinned.draw((x+=thresholded.getHeight()), OUTPUT_LARGE_HEIGHT+PADDING);
    
	contourFinder.draw();
    
    //tspsReceiver.draw(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
    //cam.draw(0,0);
    
    switch (currentState) {
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case WATCHING_CROWD:
        {
            tspsReceiver.draw(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
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
            //cam.draw(0,0);
            ofNoFill();
            
            //ofPushMatrix();
            //ofScale(1 / scaleFactor, 1 / scaleFactor);
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
    switch (key) {
        case '1':
            currentState = WATCHING_CROWD;
            break;
        case '2':
            currentState = CREATING_PORTRAIT;
            break;
        case '3':
            currentState = SCROLLPAPER;
            break;
        case '4':
            currentState = DRAWING_PORTRAIT;
            break;
        case '5':
            currentState = DRAWING_COMPLETE;
            break;
    }
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