#define WATCHING_CROWD 0
#define CREATING_PORTRAIT 1
#define CONFIRM_PORTRAIT 2
#define SCROLLPAPER 3
#define DRAWING_PORTRAIT 4
#define DRAWING_COMPLETE 5

#define PADDING 10
#define OUTPUT_LARGE_WIDTH 720
#define OUTPUT_LARGE_HEIGHT 405

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGTH 1080

#define CV_SCALE_FACTOR 0.3
#define CROPPED_FACE_SIZE 512
#define CONTOUR_MIN_AREA 3
#define SHADING_RES 10

#define MOVE_ABS 0
#define MOVE_REL 1
#define LINE_ABS 2
#define LINE_REL 3

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
typedef std::pair<int, int> intPair;
vector<ofPolyline> getPaths(ofPixels& img, float minGapLength = 2, int minPathLength = 0) {
	float minGapSquared = minGapLength * minGapLength;
	
	list<intPair> remaining;
	int w = img.getWidth(), h = img.getHeight();
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			if(img.getColor(x, y).getBrightness() > 128) {
				remaining.push_back(intPair(x, y));
			}
		}
	}
	
	vector<ofPolyline> paths;
	if(!remaining.empty()) {
		int x = remaining.back().first, y = remaining.back().second;
		while(!remaining.empty()) {
			int nearDistance = 0;
			list<intPair>::iterator nearIt, it;
			for(it = remaining.begin(); it != remaining.end(); it++) {
				intPair& cur = *it;
				int xd = x - cur.first, yd = y - cur.second;
				int distance = xd * xd + yd * yd;
				if(it == remaining.begin() || distance < nearDistance) {
					nearIt = it, nearDistance = distance;
					// break for immediate neighbors
					if(nearDistance < 4) {
						break;
					}
				}
			}
			intPair& next = *nearIt;
			x = next.first, y = next.second;
			if(paths.empty()) {
				paths.push_back(ofPolyline());
			} else if(nearDistance >= minGapSquared) {
				if(paths.back().size() < minPathLength) {
					paths.back().clear();
				} else {
					paths.push_back(ofPolyline());
				}
			}
			paths.back().addVertex(ofVec2f(x, y));
			remaining.erase(nearIt);
		}
	}
	
	return paths;
}

//--------------------------------------------------------------

bool sortByX( ofPoint a, ofPoint b){
    return a.x < b.x;
}

//--------------------------------------------------------------
void testApp::setup(){
    ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofEnableSmoothing();
    
    plotMinX = 1500;
    plotMaxX = 11350;
    plotMinY = 2500;
    plotMaxY = 11350;
    plotScaleFactor = 10;
    currentlyPlotting = false;
    plotterReady = false;
    firstLastDraw = false;
    currentInstruction = 0;
    
    // VARIABLES
    currentState = WATCHING_CROWD;
    currentStateTitle = states[currentState];
    minCrowdSize = 4;
    sliceWidth = 200;
    backgroundLearningTime = 900;
    backgroundThresholdValue = 10;
    
    // CONNECTIONS
    tspsReceiver.connect( 12000 );
    ofxAddTSPSListeners(this);
    people = tspsReceiver.getPeople();
    currentCrowdSize = people.size();
    
//    serial.listDevices();
//	vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();
//    string deviceLine, serialID;
//    for(int i=0; i<deviceList.size();i++){
//        deviceLine = deviceList[i].getDeviceName().c_str();
//        if(deviceLine.substr(0,12) == "tty.usbmodem"){
//            serialID = "/dev/" +deviceLine;
//            cout<<"arduino serial = "<<serialID<<endl;
//        }
//    }
//	serial.setup(serialID,57600);
//	serial.startContinuesRead(false);
//	ofAddListener(serial.NEW_MESSAGE,this,&testApp::onNewMessage);
    
    // CAMERA & CV
    scaleFactor = CV_SCALE_FACTOR;
	croppedFaceSize = CROPPED_FACE_SIZE;
    // TODO: Make face non-square?
    
    //cam.setDeviceID(0);
	cam.initGrabber(VIDEO_WIDTH, VIDEO_HEIGTH);
	background.setLearningTime(backgroundLearningTime);
	background.setThresholdValue(backgroundThresholdValue);
    classifier.load(ofToDataPath("haarcascade_frontalface_alt2.xml"));
    display.allocate(VIDEO_WIDTH, VIDEO_HEIGTH, OF_IMAGE_COLOR);
    backgroundSub.allocate(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT, OF_IMAGE_COLOR);
    imitate(backgroundSub, tspsGrab);
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
    gui.addSlider("minCrowdSize", minCrowdSize, 2, 10, true);
    gui.addSlider("roiSize", 100, 25, 400, true);
    gui.addSlider("roiHeightDec", 0, 120, OUTPUT_LARGE_HEIGHT/2, true);
    gui.addSlider("leftCamCal", -9, -OUTPUT_LARGE_WIDTH/2, OUTPUT_LARGE_WIDTH/2, true);
    gui.addSlider("rightCamCal", 768, OUTPUT_LARGE_WIDTH/2, OUTPUT_LARGE_WIDTH + OUTPUT_LARGE_WIDTH/2, true);
    gui.addSlider("frontScaleFactor", 1.6, 1, 2.5, false);
    gui.addSlider("backScaleFactor", 0.75, 0.5, 1, false);
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
	gui.addSlider("minGapLength", 5.5, 2, 12, false);
	gui.addSlider("minPathLength", 40, 0, 50, true);
	gui.addSlider("facePadding", 1.4, 0, 2, false);
    gui.addSlider("verticalOffset", int(-croppedFaceSize/24), int(-croppedFaceSize/2), int(croppedFaceSize/2), false);
	gui.loadSettings("facesettings.xml");
    
    gui.addPanel("Camera Settings");
    gui.addToggle("flipVideo", true);
    gui.addToggle("backgroundSubtraction", false);
    gui.addSlider("backgroundThresholdValue", backgroundThresholdValue, 0, 255, true);
    gui.addSlider("backgroundLearningTime", backgroundLearningTime, 100, 1500, true);
    gui.loadSettings("contoursettings.xml");
    
    gui.addPanel("Shading Settings");
    gui.addToggle("shadingOn", true);
	gui.addSlider("shadingThresh", 40, 0, 255, false);
    gui.addSlider("contourThreshold", 0, 0, 255, true);
    gui.addSlider("contourMinAreaRadius", CONTOUR_MIN_AREA, 1, 50, true);
    gui.addSlider("contourMaxAreaRadius", croppedFaceSize*3/4, croppedFaceSize/2, croppedFaceSize, true);
    gui.addSlider("minBlobNumber", 2, 1, 10, true);
    gui.addSlider("minBlobArea", 10000, 5000, 20000, true);
    gui.addToggle("contourFindHoles", true);
    gui.addToggle("contourSimplify", true);
	gui.loadSettings("contoursettings.xml");
    
    gui.addPanel("Drawing");
    gui.addToggle("drawingOn", true);
    gui.addToggle("confirmFirst", true);
    gui.addSlider("SCALE", 10, 1, 20, true);
    gui.addSlider("home_x", 6425, plotMinX, plotMaxX, true);
    gui.addSlider("home_y", 6425, plotMinY, plotMaxY, true);
    gui.addSlider("startX", 6000, plotMinX, plotMaxX, true);
    gui.addSlider("startY", 6000, plotMinY, plotMaxY, true);
    gui.addToggle("firstLastDraw", false);
    gui.loadSettings("calibration.xml");
    
    // DISPLAY
    ofBackground(0);
    ofTrueTypeFont::setGlobalDpi(72);
    font.loadFont("verdana.ttf", 15, true, true);
}

//--------------------------------------------------------------
void testApp::update(){
    drawingState = currentState;
    
    // Get control panel values ( TODO: everytime? )
    minCrowdSize = gui.getValueI("minCrowdSize");
    flipVideo = gui.getValueI("flipVideo");
    currentCrowdSize = people.size();
    currentStateTitle = states[currentState];
    
    backgroundSubtraction = gui.getValueB("backgroundSubtraction");
    if (backgroundSubtraction && currentState < SCROLLPAPER) {
        cam.update();
        if(cam.isFrameNew()) {
            copy(cam, display);
            if (flipVideo) display.mirror(true, false);
            display.resize(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
            background.update(display, backgroundSub);
            backgroundSub.update();
        }
    }
    
    // Position Calibration
    if (currentState > WATCHING_CROWD && currentState < SCROLLPAPER) {
        roiSize = gui.getValueI("roiSize");
        leftCamCal = gui.getValueI("leftCamCal");
        rightCamCal = gui.getValueI("rightCamCal");
        frontScaleFactor = gui.getValueF("frontScaleFactor");
        backScaleFactor = gui.getValueF("backScaleFactor");
        roiHeightDec = gui.getValueI("roiHeightDec");
        leaderPosAdjusted = ofMap(OUTPUT_LARGE_WIDTH - leaderOverheadPosition.x, 0, OUTPUT_LARGE_WIDTH, leftCamCal, rightCamCal);
        leaderPosAdjusted -= OUTPUT_LARGE_WIDTH/2;
        // Account for camera scue
        if (leaderOverheadPosition.y > OUTPUT_LARGE_HEIGHT/2) {
            distScale = ofMap(leaderOverheadPosition.y, OUTPUT_LARGE_HEIGHT/2, OUTPUT_LARGE_HEIGHT, 1, frontScaleFactor);
            //distScale = frontScaleFactor;
        } else {
            distScale = ofMap(leaderOverheadPosition.y, 0, OUTPUT_LARGE_HEIGHT/2, backScaleFactor, 1);
            //distScale = backScaleFactor;
        }
        roiY = ofMap(leaderOverheadPosition.y, 0, OUTPUT_LARGE_HEIGHT, roiHeightDec, 0);
        roiHeight = ofMap(leaderOverheadPosition.y, 0, OUTPUT_LARGE_HEIGHT, OUTPUT_LARGE_HEIGHT - 150, OUTPUT_LARGE_HEIGHT);
        roiHeight -= roiY;
        leaderPosAdjusted *= distScale;
        roiSize *= distScale;
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
                // TODO: timer on crowd?
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
            
            if (!backgroundSubtraction) cam.update();

            if(cam.isFrameNew()) {
                int black = gui.getValueI("black");
                float sigma1 = gui.getValueF("sigma1");
                float sigma2 = gui.getValueF("sigma2");
                float tau = gui.getValueF("tau");
                float thresh = gui.getValueF("thresh");
                int halfw = gui.getValueI("halfw");
                int smoothPasses = gui.getValueI("smoothPasses");
                float minGapLength = gui.getValueF("minGapLength");
                int minPathLength = gui.getValueI("minPathLength");
                float facePadding = gui.getValueF("facePadding");
                int verticalOffset = gui.getValueI("verticalOffset");
                int shadeThres = gui.getValueF("shadingThresh");
                backgroundSubtraction = gui.getValueB("backgroundSubtraction");
                background.setLearningTime(gui.getValueI("backgroundLearningTime"));
                background.setThresholdValue(gui.getValueI("backgroundThresholdValue"));
                
                minBlobNumber = gui.getValueI("minBlobNumber");
                minBlobArea = gui.getValueI("minBlobArea");
                shadingOn = gui.getValueB("shadingOn");
                drawingOn = gui.getValueB("drawingOn");
                
                contourFinder.setMinAreaRadius(gui.getValueI("contourMinAreaRadius"));
                contourFinder.setMaxAreaRadius(gui.getValueI("contourMaxAreaRadius"));
                contourFinder.setFindHoles(gui.getValueB("contourFindHoles"));
                contourFinder.setSimplify(gui.getValueB("contourSimplify"));
                
                
                if (!backgroundSubtraction) {
                    copy(cam, display);
                    if (flipVideo) display.mirror(true, false);
                    display.resize(OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
                }
                
                convertColor(cam, gray, CV_RGB2GRAY);
                if (flipVideo) gray.mirror(true, false);
                resize(gray, graySmall);
                
                ofRectangle cvBoundingBox(0, 0, graySmall.getWidth(), graySmall.getHeight());
                roiRect.set(OUTPUT_LARGE_WIDTH/2 + leaderPosAdjusted - roiSize/2, roiY, roiSize, roiHeight);
                roiScale = OUTPUT_LARGE_WIDTH/VIDEO_WIDTH/scaleFactor; // 720/1920/0.3 = 1.25
                roiRect.getPositionRef() /= 1.25; // roiScale; TODO: Why do I need the actual number? C++ int / float stuff - sort it out
                roiRect.scale(1 / 1.25);  // (1/roiScale); Why do I need the actual number?
                roiRect = roiRect.getIntersection(cvBoundingBox);
                
                cv::Rect searchROI = toCv(roiRect);
                Mat graySmallMat = toCv(graySmall);
                Mat croppedGraySmallMat(graySmallMat, searchROI);
                equalizeHist(croppedGraySmallMat, croppedGraySmallMat);
                graySmall.update(); // TODO: Needed ??
                
                classifier.detectMultiScale(croppedGraySmallMat, objects, 1.06, 1,
                                            CASCADE_DO_CANNY_PRUNING |
                                            CASCADE_FIND_BIGGEST_OBJECT |
                                            //CASCADE_DO_ROUGH_SEARCH |
                                            0);
                
                ofRectangle camBoundingBox(0, 0, cam.getWidth(), cam.getHeight());
                ofRectangle faceRect;
                if(objects.empty()) {
                    // No Face. Start over;
                    currentState = 10;
                    break;
                } else {
                    ofLog(OF_LOG_NOTICE, "  ---- Face Found ----");
                    faceRect = toOf(objects[0]);
                    faceRect.x += roiRect.x;
                    faceRect.y += roiRect.y;
                    faceRect.getPositionRef() /= scaleFactor;
                    faceRect.scale(1 / scaleFactor);
                    faceRect.scaleFromCenter(facePadding);
                    faceRect.translateY(verticalOffset);
                }
                
                // Process Captured Face Image
                
                // TODO: Minimum face size depending on distance?
                
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
                
                //equalizeHist(face, face);
                face.update();
                
                // Save Face
                string fileName = ofToString(ofGetYear()) + "-" + ofToString(ofGetMonth()) + "-" + ofToString(ofGetDay())
                    + " " + ofToString(ofGetHours()) + "." + ofToString(ofGetMinutes()) + "." + ofToString(ofGetSeconds()) + " face.png";
                ofSaveImage(face, fileName);
                
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
                
                // Is there a decent face drawing here? How much shading is there?
                
                float totalArea;
                for(int i = 0; i < n; i++) {
                    ofPolyline blob = contourFinder.getPolyline(i).getSmoothed(n/10);
                    totalArea += ABS(blob.getArea());
                }
                
                ofLog(OF_LOG_NOTICE) << "totalArea = " << totalArea;
                
                if (n < minBlobNumber || totalArea < minBlobArea) {
                    currentState = 10;
                    break;
                }
                
                // TODO: look for min percentage black to white? Or num of paths? If not, start over
                
                if (shadingOn) {
                     // Create Shading
                    shading.clear();
                    
                   
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
                }
                
                if (drawingOn) {
                    //Get Paths
                    paths = getPaths(thinned, minGapLength, minPathLength);
                    pathsToInstructions();
                }
                    
                // B&W Image -> Vector graphics (using outlines or fill?)
                currentState++;
                ofLog(OF_LOG_NOTICE, "== STATE: AWAITING PORTRAIT CONFIRMATION ==");

            }
        }
        break;
            
            // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case CONFIRM_PORTRAIT:
        {
            confirmFirst = gui.getValueB("confirmFirst");
            if (!confirmFirst) {
                currentState++;
                ofLog(OF_LOG_NOTICE, "== STATE: SCROLLING PAPER ==");
            }
            // Check the portrait
            // currentState++;
            //ofLog(OF_LOG_NOTICE, "== STATE: SCROLLING PAPER ==");
        }
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case SCROLLPAPER:
        {
            
            if (plotterReady) {
                
            }
            // Scroll the paper into place
            // Delay until complete
            // currentState++;
            // ofLog(OF_LOG_NOTICE, "== STATE: DRAWING PORTRAIT ==");
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case DRAWING_PORTRAIT:
        {
            
//            if (instructions.size() > 0 && plotterReady){
//                cout << "== GETTING Instruction #" << currentInstruction << " ==\n";
//                message = instructions[currentInstruction].toString();
//                currentInstruction += 1;
//                
//                if (currentInstruction >= instructions.size()) {
//                    currentState++;
//                    ofLog(OF_LOG_NOTICE, "== STATE: DRAWING COMPLETE ==");
//                }
//                
//                if(message != ""){
//                    cout << "==== SENDING serial: " << message << "\n";
//                    plotterReady = false;
//                    serial.writeString(message);
//                    message = "";
//                }
//                
//            } else {
//                ofLog(OF_LOG_NOTICE, "---- Instructions empty OR plotter not ready. Starting over");
//                currentState++;
//                ofLog(OF_LOG_NOTICE, "== STATE: WATCHING CROWD ==");
//            }
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
    if (drawingState > 0) tspsGrab.draw(0,0);
    if (drawingState < 3) {
        display.draw(OUTPUT_LARGE_WIDTH+PADDING, 0);
        if (backgroundSubtraction) backgroundSub.draw((OUTPUT_LARGE_WIDTH+PADDING)*2, 0);
        face.draw(0, OUTPUT_LARGE_HEIGHT+PADDING, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
        faceThresholded.draw(0, OUTPUT_LARGE_HEIGHT+PADDING*2+CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
        cld.draw(CROPPED_FACE_SIZE/2 + PADDING, OUTPUT_LARGE_HEIGHT+PADDING, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
        thresholded.draw(CROPPED_FACE_SIZE/2 + PADDING, OUTPUT_LARGE_HEIGHT+PADDING*2+CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
        croppedBackground.draw(CROPPED_FACE_SIZE + PADDING*2, OUTPUT_LARGE_HEIGHT+PADDING, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
        thinned.draw(CROPPED_FACE_SIZE + PADDING*2, OUTPUT_LARGE_HEIGHT+PADDING*2+CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2, CROPPED_FACE_SIZE/2);
        //croppedGraySmallMat.draw(0,0);
        
        ofPushMatrix();
        
        // Draw face / cv / roi stuff
        
        ofTranslate(OUTPUT_LARGE_WIDTH+PADDING, 0);
        ofSetColor(0, 0, 255);
        // TODO: Make this the CV ROI
        ofDrawBitmapString("KINECT POS", 2, 13);
        ofDrawBitmapString("x: " + ofToString(leaderOverheadPosition.x),2, 23);
        ofDrawBitmapString("y: " + ofToString(OUTPUT_LARGE_HEIGHT - leaderOverheadPosition.y), 2, 33);
        ofSetColor(0, 255, 0);
        ofDrawBitmapString("x*: " + ofToString(leaderPosAdjusted), 2, 43);
        
        if (objects.size() > 0) {
            //A face? Draw the info
            ofDrawBitmapString("Face = " + ofToString(objects[0].x) + "," + ofToString(objects[0].y), 2, OUTPUT_LARGE_HEIGHT - 3);
            ofScale(0.375 / CV_SCALE_FACTOR, 0.375 / CV_SCALE_FACTOR);
            ofRect(roiRect);
            ofTranslate(roiRect.x, roiRect.y);
            ofSetColor(255, 0 ,0);
            ofRect(toOf(objects[0]));
            ofDrawBitmapString(ofToString(objects[0].x) + "," + ofToString(objects[0].y), objects[0].x + 2, objects[0].y + 13);
            ofDrawBitmapString(ofToString(objects[0].width) + "x" + ofToString(objects[0].height), objects[0].x + 2, objects[0].y + 23);
        }
        ofPopMatrix();
        
        ofPushMatrix(); 
        ofTranslate(CROPPED_FACE_SIZE*1.5 + PADDING*3, OUTPUT_LARGE_HEIGHT+PADDING);
        drawPaths();
        ofPopMatrix();
    }
    
    switch (drawingState) {
            
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
            if (currentState != drawingState) {
                tspsGrab.grabScreen(0, 0, OUTPUT_LARGE_WIDTH, OUTPUT_LARGE_HEIGHT);
                tspsGrab.update();
            }
        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case CREATING_PORTRAIT:
        {

        }
        break;
            
        // - - - - - - - - - - - - - - - - - - - - - - - - - - -
            
        case SCROLLPAPER:
        {
            ofPushMatrix();
            ofTranslate(CROPPED_FACE_SIZE*1.5 + PADDING*3, OUTPUT_LARGE_HEIGHT+PADDING);
            drawPaths();
            ofPopMatrix();
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
    
    ofSetColor(0, 255 ,0);
    font.drawString(currentStateTitle, 25, 25);
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

//--------------------------------------------------------------
void testApp::drawPaths() {
    if (shadingOn) {
        ofSetColor(255,0,0);
        for(int i = 0; i < shading.size(); i++) {
            shading[i].draw();
        }
    }
    if (drawingOn) {
        for(int i = 0; i < paths.size(); i++) {
            ofSetColor(yellowPrint, 200);
            paths[i].draw();
            if(i + 1 < paths.size()) {
                ofVec2f endPoint = paths[i].getVertices()[paths[i].size() - 1];
                ofVec2f startPoint = paths[i + 1].getVertices()[0];
                ofSetColor(55,55,55);
                ofLine(endPoint, startPoint);
            }
        }
    }
}

//--------------------------------------------------------------
void testApp::pathsToInstructions() {
    ofLog(OF_LOG_NOTICE, "---- Paths to Instructions ----");
    float startX = gui.getValueF("startX");
    float startY = gui.getValueF("startY");
    float home_x = gui.getValueF("home_x");
    float home_y = gui.getValueF("home_y");
    firstLastDraw = gui.getValueB("firstLastDraw");
    
    if (instructions.size()>0) instructions.erase(instructions.begin());
    //cout << "--PATHS TO INSTRUCTIONS\n";
    // MOVE TO START:
    ofVec2f startPoint = paths[0].getVertices()[0];
    instructions.push_back(Instruction(MOVE_ABS, startPoint.x*plotScaleFactor + startX, startPoint.y*plotScaleFactor + startY));
    //cout << "---- MOVE FOR #1 TO: [ " << startPoint << "]\n";
	for(int i = 0; i < paths.size(); i++) {
		//DRAW
        for(int j = 1; j < paths[i].getVertices().size(); j++) {
            ofVec2f endPoint = paths[i].getVertices()[j];
            instructions.push_back(Instruction(LINE_ABS, endPoint.x*plotScaleFactor + startX, endPoint.y*plotScaleFactor + startY ));
//            cout << "------ DRAW FOR #" << i << " TO:" << j << " = [ " << endPoint << " ]\n";
        }
        // MOVE
        if (firstLastDraw && i == 0 && paths.size() > 2) {
//            cout << "---- SKIP TO LAST VECTOR SET \n";
            // MOVE STRAIGHT TO LAST DRAW SET
            i = paths.size()-2;
        }
		if(i + 1 < paths.size()) {
			ofVec2f startPoint = paths[i + 1].getVertices()[0];
            instructions.push_back(Instruction(MOVE_ABS, startPoint.x*plotScaleFactor + startX, startPoint.y*plotScaleFactor + startY ));
//            cout << "---- MOVE FOR #" << i + 1 << " TO: [ " << startPoint << " ]\n";
		}
	}
    cout << "--COMPLETE: " << instructions.size() << " INSTRUCTIONS CREATED\n" << "--NOW PRINT";
    if (instructions.size() > 0) {
        instructions.push_back(Instruction(MOVE_ABS, home_x, home_y ));
        currentlyPlotting = true;
    }
}

//--------------------------------------------------------------

void testApp::onNewMessage(string & message)
{
	cout << "onNewMessage, message: " << message << "\n";
	if (message == "OK") {
        cout << "-- PLOTTER READY --\n";
        plotterReady = true;
    }
}