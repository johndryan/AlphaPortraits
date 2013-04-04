#define WATCHING_CROWD 0
#define CREATING_PORTRAIT 1
#define CONFIRM_PORTRAIT 2
#define SCROLLPAPER 3
#define DRAWING_PORTRAIT 4
#define DRAWING_COMPLETE 5

#define PADDING 10
#define OUTPUT_LARGE_WIDTH 492
#define OUTPUT_LARGE_HEIGHT 364

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGTH 1080

#define CV_SCALE_FACTOR 0.3
#define CROPPED_FACE_SIZE 512
#define CONTOUR_MIN_AREA 3
#define SHADING_RES 10

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

bool sortByX( ofPoint a, ofPoint b){
    return a.x < b.x;
}

//--------------------------------------------------------------
void testApp::setup(){
    // VARIABLES
    currentState = WATCHING_CROWD;
    currentStateTitle = states[currentState];
    minCrowdSize = 3;
    sliceWidth = 200;
    backgroundLearningTime = 900;
    backgroundThresholdValue = 10;
    
    // CONNECTIONS
    tspsReceiver.connect( 12000 );
    ofxAddTSPSListeners(this);
    people = tspsReceiver.getPeople();
    currentCrowdSize = people.size();
    
    // CAMERA & CV
    scaleFactor = CV_SCALE_FACTOR;
	croppedFaceSize = CROPPED_FACE_SIZE;
    // TODO: Make face non-square?
    
    //ofSetVerticalSync(true);
	cam.initGrabber(VIDEO_WIDTH, VIDEO_HEIGTH);
	background.setLearningTime(backgroundLearningTime);
	background.setThresholdValue(backgroundThresholdValue);
    classifier.load(ofToDataPath("haarcascade_frontalface_alt2.xml"));
    display.allocate(VIDEO_WIDTH, VIDEO_HEIGTH, OF_IMAGE_COLOR);
    backgroundSub.allocate(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT, OF_IMAGE_COLOR);
    graySmall.allocate(cam.getWidth() * scaleFactor, cam.getHeight() * scaleFactor, OF_IMAGE_GRAYSCALE);
	face.allocate(croppedFaceSize, croppedFaceSize, OF_IMAGE_GRAYSCALE);
	img.init(croppedFaceSize, croppedFaceSize);
	imitate(gray, face);
	imitate(cld, face);
	imitate(thresholded, face);
	imitate(thinned, face);
	imitate(croppedBackground, face);
    imitate(faceThresholded, face);
    
	contourFinder.setMinAreaRadius(CONTOUR_MIN_AREA);
	contourFinder.setMaxAreaRadius(croppedFaceSize*3/4);
    contourFinder.setFindHoles(true);
    contourFinder.setSimplify(true);
    backgroundContourFinder.setMinAreaRadius(100);
    backgroundContourFinder.setMaxAreaRadius(croppedFaceSize);
    backgroundContourFinder.setSimplify(true);
    backgroundContourFinder.setInvert(true);
    
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
	gui.addSlider("facePadding", 1.4, 0, 2, false);
    gui.addSlider("verticalOffset", int(-croppedFaceSize/24), int(-croppedFaceSize/2), int(croppedFaceSize/2), false);
	gui.loadSettings("facesettings.xml");
    
    gui.addPanel("Background Subtraction");
    gui.addToggle("backgroundSubtraction", false);
    gui.addSlider("backgroundThresholdValue", backgroundThresholdValue, 0, 255, true);
    gui.addSlider("backgroundLearningTime", backgroundLearningTime, 100, 1500, true);
    gui.loadSettings("contoursettings.xml");
    
    gui.addPanel("Shading Settings");
	gui.addSlider("shadingThresh", 5, 0, 255, false);
    gui.addSlider("contourThreshold", 0, 0, 255, true);
    gui.addSlider("contourMinAreaRadius", CONTOUR_MIN_AREA, 1, 50, true);
    gui.addSlider("contourMaxAreaRadius", croppedFaceSize*3/4, croppedFaceSize/2, croppedFaceSize, true);
    gui.addToggle("contourFindHoles", true);
    gui.addToggle("contourSimplify", true);
	gui.loadSettings("contoursettings.xml");
    
    // DISPLAY
    ofBackground(0);
}

//--------------------------------------------------------------
void testApp::update(){
    // Get control panel values ( TODO: everytime? )
    minCrowdSize = gui.getValueI("minCrowdSize");
    currentCrowdSize = people.size();
    currentStateTitle = states[currentState];
    backgroundSubtraction = gui.getValueB("backgroundSubtraction");
	background.setLearningTime(gui.getValueI("backgroundLearningTime"));
	background.setThresholdValue(gui.getValueI("backgroundThresholdValue"));
    if (backgroundSubtraction) {
        cam.update();
        copy(cam, display);
        display.resize(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
        
        background.update(display, backgroundSub);
        backgroundSub.update();
    }
    
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
            
            
            if (!backgroundSubtraction) {
                cam.update();
                copy(cam, display);
                display.resize(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
            }
            
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
                int shadeThres = gui.getValueF("shadingThresh");
                
                contourFinder.setMinAreaRadius(gui.getValueI("contourMinAreaRadius"));
                contourFinder.setMaxAreaRadius(gui.getValueI("contourMaxAreaRadius"));
                contourFinder.setFindHoles(gui.getValueB("contourFindHoles"));
                contourFinder.setSimplify(gui.getValueB("contourSimplify"));
                
                convertColor(cam, gray, CV_RGB2GRAY);
                resize(gray, graySmall);
                
                // TODO: Set OpenCV ROI by information from overhead cam
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
                    ofLog(OF_LOG_NOTICE, "  ---- Face Found ----");
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
                
                if (backgroundSubtraction) {
                    ofRectangle backRect;
                    backRect.x = faceRect.x*OUTPUT_LARGE_WIDTH/VIDEO_WIDTH;
                    backRect.y = faceRect.y*OUTPUT_LARGE_HEIGHT/VIDEO_HEIGTH;
                    backRect.width = faceRect.width*OUTPUT_LARGE_WIDTH/VIDEO_WIDTH;
                    backRect.height = faceRect.height*OUTPUT_LARGE_HEIGHT/VIDEO_HEIGTH;
                    
                    cv::Rect roiScaled = toCv(backRect);
                    Mat backgroundMat = toCv(backgroundSub);
                    Mat croppedBackMat(backgroundMat, roiScaled);
                    resize(croppedBackMat, croppedBackground);
                    croppedBackground.update();
                }
                
                face.update();
                
                int j = 0;
                unsigned char* grayPixels = face.getPixels();
                unsigned char* backgroundPixels = croppedBackground.getPixels();
                for(int y = 0; y < croppedFaceSize; y++) {
                    for(int x = 0; x < croppedFaceSize; x++) {
//                        img[x][y] = grayPixels[j++] - black;
                        int colorVal = grayPixels[j] - black;
                        if (backgroundSubtraction && backgroundPixels[j] < colorVal) {
                            img[x][y] = backgroundPixels[j];
                        } else {
                            img[x][y] = colorVal;
                        }
                        j++;
                    }
                }
                
                // Use a blob of background subtraction to delete elements around the edge
//                for(int i = 0; i < backgroundContourFinder.size(); i++) {
//                    ofPolyline blob = backgroundContourFinder.getPolyline(i);
//                    for(int y = 0; y < croppedFaceSize; y++) {
//                        for(int x = 0; x < croppedFaceSize; x++) {
//                            if (backgroundSubtraction && img[x][y] < 0 && blob.inside(x, y)) {
//                                img[x][y] = 255;
//                                ofLog(OF_LOG_NOTICE) << "Changing color of " << x << "," << y;
//                            }
//                        }
//                    }
//                }
                
                etf.init(croppedFaceSize, croppedFaceSize);
                etf.set(img);
                etf.Smooth(halfw, smoothPasses);
                GetFDoG(img, etf, sigma1, sigma2, tau);
                j = 0;
                unsigned char* cldPixels = cld.getPixels();
                for(int y = 0; y < croppedFaceSize; y++) {
                    for(int x = 0; x < croppedFaceSize; x++) {
                        cldPixels[j++] = img[x][y];
//                        int colorVal = img[x][y];
//                        if (backgroundSubtraction && backgroundPixels[j] < colorVal) {
//                            cldPixels[j] = 255 - backgroundPixels[j];
//                        } else {
//                            cldPixels[j] = colorVal;
//                        }
//                        j++;
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
                
                threshold(face, faceThresholded, shadeThres, true);
                faceThresholded.update();
                
                backgroundContourFinder.setAutoThreshold(true);
                backgroundContourFinder.findContours(croppedBackground);
                
                contourFinder.setAutoThreshold(true);
                contourFinder.findContours(faceThresholded);
                int n = contourFinder.size();
                
                shading.clear();
                
                // Create Shading
                ofLog(OF_LOG_NOTICE, "  ---- Create Shading ----");
                for (int i = 0; i < croppedFaceSize*2/SHADING_RES; i++) {
                    ofPoint lineStart = ofPoint(0, i * SHADING_RES);
                    ofPoint lineEnd = ofPoint(i * SHADING_RES, 0);

                    for(int i = 0; i < n; i++) {
                        linePoints.clear();
                        
                        ofPolyline blob = contourFinder.getPolyline(i).getSmoothed(n/10);
                        for(int i = 1; i < blob.size(); i++) {
                            ofPoint intersection;
                            ofPoint line2start = ofPoint(blob[i-1].x,blob[i-1].y);
                            ofPoint line2end = ofPoint(blob[i].x,blob[i].y);
                            if (ofLineSegmentIntersection(lineStart, lineEnd, line2start, line2end, intersection)) {
                                linePoints.push_back(intersection);
                            }
                        }
                        
                        ofSort(linePoints, sortByX);
                        
                        for(int i = 0; i < linePoints.size(); i+=2) {
                            if (i+1 < linePoints.size()) {
                                ofPolyline line;
                                line.addVertex(linePoints[i]);
                                line.addVertex(linePoints[i+1]);
                                shading.push_back(line);
                            }
                        }
                    }
                }
                
                // Do threshoding & look for min percentage black to white?
                    
                // B&W Image -> Vector graphics (using outlines or fill?)
                currentState++;
                ofLog(OF_LOG_NOTICE, "== STATE: AWAITING PORTRAIT CONFIRMATION ==");
            }
        }
        break;
            
            // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case CONFIRM_PORTRAIT:
        {
            // Check the portrait
            // currentState++;
            //ofLog(OF_LOG_NOTICE, "== STATE: SCROLLING PAPER ==");
        }
            
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
    if (backgroundSubtraction) backgroundSub.draw((OUTPUT_LARGE_WIDTH+PADDING)*2, 0);
    face.draw(0, OUTPUT_LARGE_HEIGHT+PADDING, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
    faceThresholded.draw(0, OUTPUT_LARGE_HEIGHT+PADDING*2+CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
    cld.draw(CROPPED_FACE_SIZE/2 + PADDING, OUTPUT_LARGE_HEIGHT+PADDING, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
    thresholded.draw(CROPPED_FACE_SIZE/2 + PADDING, OUTPUT_LARGE_HEIGHT+PADDING*2+CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
    croppedBackground.draw(CROPPED_FACE_SIZE + PADDING, OUTPUT_LARGE_HEIGHT+PADDING, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
    //thinned.draw(CROPPED_FACE_SIZE/2 + PADDING, OUTPUT_LARGE_HEIGHT+PADDING*2+CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
    
    //int n = contourFinder.size();
    ofTranslate(CROPPED_FACE_SIZE*1.5 + PADDING*3, OUTPUT_LARGE_HEIGHT+PADDING);
    thinned.draw(0,0);
    
//    for(int i = 0; i < n; i++) {
//        ofPolyline blob = contourFinder.getPolyline(i);
//        ofSetColor(255,0,0);
//        blob.draw();
//        ofSetColor(255,255,255);
//        blob.getSmoothed(n/10).draw();
//        //ofxCvBlob blob = contourFinder.blobs.at(i);
//        // do something fun with blob
//    }
    ofSetColor(255,0,0);
    for(int i = 0; i < shading.size(); i++) {
        shading[i].draw();
    }
    ofSetColor(0, 255, 0);
    for(int i = 0; i < backgroundContourFinder.size(); i++) {
        ofPolyline blob = backgroundContourFinder.getPolyline(i);
        blob.draw();
    }
    //backgroundContourFinder.draw();
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
            currentState = CONFIRM_PORTRAIT;
            break;
        case '4':
            currentState = SCROLLPAPER;
            break;
        case '5':
            currentState = DRAWING_PORTRAIT;
            break;
        case '6':
            currentState = DRAWING_COMPLETE;
            break;
        case 'b':
            background.reset();
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