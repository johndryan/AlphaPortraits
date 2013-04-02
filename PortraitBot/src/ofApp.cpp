#define MOVE_ABS 0
#define MOVE_REL 1
#define LINE_ABS 2
#define LINE_REL 3
#define SCROLL_PAPER 4

#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

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

vector<Instruction> instructions;

//--------------------------------------------------------------
void ofApp::setup() {
    min_X = 5300;
    max_X = 13940;
    min_Y = 3000;
    max_Y = 13500;
    scaleFactor = 20;
    currentlyPlotting = false;
    plotterReady = false;
    currentInstruction = 0;
    
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofEnableSmoothing();
	
    cam.setDeviceID(0);
	camWidth = 640, camHeight = 480;
	cam.initGrabber(camWidth, camHeight);
	
	faceTrackingScaleFactor = .5;
	croppedSize = 256;
	classifier.load(ofToDataPath("haarcascade_frontalface_alt2.xml"));
	graySmall.allocate(camWidth * faceTrackingScaleFactor, camHeight * faceTrackingScaleFactor, OF_IMAGE_GRAYSCALE);
	cropped.allocate(croppedSize, croppedSize, OF_IMAGE_GRAYSCALE);
	
	img.init(croppedSize, croppedSize);
	imitate(gray, cropped);
	imitate(cld, cropped);
	imitate(thresholded, cropped);
	imitate(thinned, cropped);
	
	gui.setup();
	gui.addPanel("Photo Settings");
	gui.addSlider("black", 42, -255, 255, true);        // kyle's 0, 65, 48, 20
	gui.addSlider("sigma1", 0.99, 0.01, 2.0, false);    // kyle's 0.4, 0.85
	gui.addSlider("sigma2", 4.45, 0.01, 10.0, false);   // kyle's 3.0, 4.45
	gui.addSlider("tau", 0.97, 0.8, 1.0, false);        // kyle's 0.97
	gui.addSlider("halfw", 4, 1, 8, true);              // kyle's 4
	gui.addSlider("smoothPasses", 4, 1, 4, true);       // kyle's 2, 3
	gui.addSlider("thresh", 121.8, 0, 255, false);      // kyle's 128, 121.8
	gui.addSlider("minGapLength", 5.5, 2, 12, false);   // kyle's 2, 6.8
	gui.addSlider("minPathLength", 40, 0, 50, true);    // kyle's 20
	gui.addSlider("facePadding", 1.5, 0, 2, false);     // kyle's 1.5, 1.77
    gui.addSlider("verticalOffset", int(-croppedSize/12), int(-croppedSize/2), int(croppedSize/2), false);
	gui.loadSettings("settings.xml");
    
    gui.addPanel("Print Variables");
    gui.addSlider("Tolerance", 20, 1, 200, true);                             // TOL
    gui.addSlider("Jog Speed", 900, 500, 1200, true);          // DELAY_FAST
    gui.addSlider("Printing LOW delay", 2000, 1000, 4000, true);    // DELAY_MIN
    gui.addSlider("Printing HIGH delay", 50, 10, 100, true);            // HIGH_DELAY
    gui.addSlider("Ink Delay", 7000, 2000, 8000, true);                 // INK_DELAY
    gui.addSlider("Ink Start Delay", 800, 200, 2000, true);             // INK_START_DELAY
    gui.addSlider("Ink Start Steps", 500, 100, 1000, true);             // INK_START_STEPS
    gui.addSlider("Ink Stop Delay", 800, 200, 2000, true);              // INK_STOP_DELAY
    gui.addSlider("Ink Stop Steps", 300, 100, 1000, true);              // INK_STOP_STEPS
    gui.addSlider("Ink Wait", 500000, 100000, 900000, true);           // INK_WAIT
    gui.addSlider("17 turns ink on", 17, 17, 18, true);
    gui.loadSettings("printvariables.xml");
    
    gui.addPanel("Calibration");
    gui.addSlider("SCALE", 24, 1, 40, true);
    gui.addSlider("home_x", 9620, min_X, max_X, true);
    gui.addSlider("home_y", 9620, min_Y, max_Y, true);
    gui.addSlider("startX", 6500, min_X, max_X, true);
    gui.addSlider("startY", 3500, min_Y, max_Y, true);
    // Add Booleans as buttons?
    gui.loadSettings("calibration.xml");
    
//    AT.setup();
//    AT.cropped_size = croppedSize;
//    AT.start();
	
	serial.listDevices();
	vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();
    
    string deviceLine, serialID;
    for(int i=0; i<deviceList.size();i++){
        
        deviceLine = deviceList[i].getDeviceName().c_str();
        
        if(deviceLine.substr(0,12) == "tty.usbmodem"){
            serialID = "/dev/" +deviceLine;
            cout<<"arduino serial = "<<serialID<<endl;
        }
    }
	
	serial.setup(serialID,57600);
	serial.startContinuesRead(false);
	ofAddListener(serial.NEW_MESSAGE,this,&ofApp::onNewMessage);
    
    timer = 0;
    timeout = 10;
    runOnTimer = false;
    
    ofTrueTypeFont::setGlobalDpi(72);
    font.loadFont("verdana.ttf", 15, true, true);
    firstLastDraw = false;
    randomLoc = false;
}

void ofApp::onNewMessage(string & message)
{
	cout << "onNewMessage, message: " << message << "\n";
	if (message == "OK") {
        cout << "-- PLOTTER READY --\n";
        plotterReady = true;
    }
}


//--------------------------------------------------------------
void ofApp::update(){
cam.update();
    // -- NO WE'RE NOT PLOTTING, CHECK FOR NEW FACE?

    // -- Is camera, plotter and X ready to go
    if(cam.isFrameNew() && plotterReady && !currentlyPlotting) {
        
        // -- DO TIMER --> TRIGGER
        if((ofGetElapsedTimef() - timer) > timeout && runOnTimer) {
            cout << "-- CHECKING FOR FACES -- " << endl;
            timer = ofGetElapsedTimef();
            needToUpdate = true;
        }
        
        // UPDATE, TRIGGERED BY TIMER or SPACEBAR
        if(needToUpdate) {
            int black = gui.getValueI("black");
            float sigma1 = gui.getValueF("sigma1");
            float sigma2 = gui.getValueF("sigma2");
            float tau = gui.getValueF("tau");
            float thresh = gui.getValueF("thresh");
            int halfw = gui.getValueI("halfw");
            int smoothPasses = gui.getValueI("smoothPasses");
            minGapLength = gui.getValueF("minGapLength");
            minPathLength = gui.getValueI("minPathLength");
            float facePadding = gui.getValueF("facePadding");
            int verticalOffset = gui.getValueI("verticalOffset");
            
            scaleFactor = gui.getValueI("SCALE");
            
            convertColor(cam, gray, CV_RGB2GRAY);
            
            // Save the original image
//            string fileName = ofToString(ofGetYear()) + "-" + ofToString(ofGetMonth()) + "-" + ofToString(ofGetDay())
//            + " " + ofToString(ofGetHours()) + "." + ofToString(ofGetMinutes()) + "." + ofToString(ofGetSeconds()) + " original.png";
//                ofSaveImage(gray.getPixelsRef(), fileName);
//                cout << "saved " << ofToString(fileName) << endl;
            
            resize(gray, graySmall);
            Mat graySmallMat = toCv(graySmall);
            equalizeHist(graySmallMat, graySmallMat);		
            classifier.detectMultiScale(graySmallMat, objects, 1.05, 1,
                                        CASCADE_DO_CANNY_PRUNING |
                                        CASCADE_FIND_BIGGEST_OBJECT |
                                        //CASCADE_DO_ROUGH_SEARCH |
                                        0);
            
            ofRectangle faceRect;
            if(objects.empty()) {
                // if there are no faces found, use the whole canvas
                faceRect.set(0, 0, camWidth, camHeight);
            } else {
                faceRect = toOf(objects[0]);
                faceRect.getPositionRef() /= faceTrackingScaleFactor;
                faceRect.scale(1 / faceTrackingScaleFactor);
                faceRect.scaleFromCenter(facePadding);
                faceRect.translateY(verticalOffset);
            }
            
            ofRectangle camBoundingBox(0, 0, camWidth, camHeight);
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
            resize(croppedGrayMat, cropped);
            cropped.update();
            
            // Save the cropped face
//            fileName = ofToString(ofGetYear()) + "-" + ofToString(ofGetMonth()) + "-" + ofToString(ofGetDay())
//            + " " + ofToString(ofGetHours()) + "." + ofToString(ofGetMinutes()) + "." + ofToString(ofGetSeconds()) + " face.png";
//            ofSaveImage(cropped, fileName);
//            cout << "saved " << ofToString(fileName) << endl;
            
            int j = 0;
            unsigned char* grayPixels = cropped.getPixels();
            for(int y = 0; y < croppedSize; y++) {
                for(int x = 0; x < croppedSize; x++) {
                    img[x][y] = grayPixels[j++] - black;
                }
            }
            etf.init(croppedSize, croppedSize);
            etf.set(img);
            etf.Smooth(halfw, smoothPasses);
            GetFDoG(img, etf, sigma1, sigma2, tau);
            j = 0;
            unsigned char* cldPixels = cld.getPixels();
            for(int y = 0; y < croppedSize; y++) {
                for(int x = 0; x < croppedSize; x++) {
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
            
            // Save the thinned paths
//            fileName = ofToString(ofGetYear()) + "-" + ofToString(ofGetMonth()) + "-" + ofToString(ofGetDay())
//            + " " + ofToString(ofGetHours()) + "." + ofToString(ofGetMinutes()) + "." + ofToString(ofGetSeconds()) + " thinned.png";
//            ofSaveImage(thinned, fileName);
//            cout << "saved " << ofToString(fileName) << endl;
            
            paths = getPaths(thinned, minGapLength, minPathLength);
            needToUpdate = false;
            
            //START PLOTTER AUTOMATICALLY
            if (runOnTimer && paths.size() > 0) {
                pathsToInstructions();
            }
            
            // get arduino thread loaded with the paths and started
        //    AT.lock();
            //    AT.paths = paths;
            //    AT.points = paths.begin()->getVertices();
            //    AT.unlock();
        }
    }
    
    //    AT.update();
        
        // update AT variables
    //    AT.TOL = gui.getValueI("Tolerance");
    //    AT.DELAY_FAST = gui.getValueI("Jog Speed");
    //    AT.DELAY_MIN = gui.getValueI("Printing LOW delay");
    //    AT.HIGH_DELAY = gui.getValueI("Printing HIGH delay");
    //    AT.INK_DELAY = gui.getValueI("Ink Delay");
    //    AT.INK_START_DELAY = gui.getValueI("Ink Start Delay");
    //    AT.INK_START_STEPS = gui.getValueI("Ink Start Steps");
    //    AT.INK_STOP_DELAY = gui.getValueI("Ink Stop Delay");
    //    AT.INK_STOP_STEPS = gui.getValueI("Ink Stop Steps");
    //    AT.INK_WAIT = gui.getValueI("Ink Wait");
    //    AT.INK_SLEEP_PIN = gui.getValueI("17 turns ink on");
        
    //    AT.home_x = gui.getValueI("home_x");
    //    AT.home_y = gui.getValueI("home_y");
    //    AT.SCALE_X = gui.getValueI("SCALE_X");
    //    AT.SCALE_Y = gui.getValueI("SCALE_Y");
    //    AT.y_height = gui.getValueI("Y Height for Pic");
    //    AT.z_height = gui.getValueI("Z Height for Pic");
    
    // -- ARE WE PLOTTING?
	if (currentlyPlotting && instructions.size() > 0 && plotterReady){
        cout << "== GETTING Instruction #" << currentInstruction << " ==\n";
        message = instructions[currentInstruction].toString();
        currentInstruction += 1;
        
        if (currentInstruction >= instructions.size()) {
            currentlyPlotting = false;
        }
        
        if(message != ""){
            cout << "==== SENDING serial: " << message << "\n";
            plotterReady = false;
            serial.writeString(message);
            message = "";
        }
        
	}
}


//--------------------------------------------------------------
void ofApp::drawPaths() {
	for(int i = 0; i < paths.size(); i++) {
		ofSetColor(yellowPrint);
		paths[i].draw();
		if(i + 1 < paths.size()) {
			ofVec2f endPoint = paths[i].getVertices()[paths[i].size() - 1];
			ofVec2f startPoint = paths[i + 1].getVertices()[0];
			ofSetColor(magentaPrint, 128);
			ofLine(endPoint, startPoint);
		}
	}
}


//--------------------------------------------------------------
void ofApp::pathsToInstructions() {
    float startX = gui.getValueF("startX");
    float startY = gui.getValueF("startY");
    float home_x = gui.getValueF("home_x");
    float home_y = gui.getValueF("home_y");
    scaleFactor = gui.getValueI("SCALE");
    
    if (randomLoc) {
        startX = ofRandom(min_X, (max_X-croppedSize*scaleFactor));
        startY = ofRandom(min_Y, (max_Y-croppedSize*scaleFactor));
    }
    if (instructions.size()>0) instructions.erase(instructions.begin());
    cout << "--PATHS TO INSTRUCTIONS\n";
    // SCROLL PAPER
    instructions.push_back(Instruction(SCROLL_PAPER, 0, 0));
    
    // MOVE TO START:
    ofVec2f startPoint = paths[0].getVertices()[0];
    instructions.push_back(Instruction(MOVE_ABS, startPoint.x*scaleFactor + startX, startPoint.y*scaleFactor + startY));
    cout << "---- MOVE FOR #1 TO: [ " << startPoint << "]\n";
	for(int i = 0; i < paths.size(); i++) {
		//DRAW
        for(int j = 1; j < paths[i].getVertices().size(); j++) {
            ofVec2f endPoint = paths[i].getVertices()[j];
            instructions.push_back(Instruction(LINE_ABS, endPoint.x*scaleFactor + startX, endPoint.y*scaleFactor + startY ));
            cout << "------ DRAW FOR #" << i << " TO:" << j << " = [ " << endPoint << " ]\n";
        }
        // MOVE
        if (firstLastDraw && i == 0 && paths.size() > 2) {
            cout << "---- SKIP TO LAST VECTOR SET \n";
            // MOVE STRAIGHT TO LAST DRAW SET
            i = paths.size()-2;
        }
		if(i + 1 < paths.size()) {
			ofVec2f startPoint = paths[i + 1].getVertices()[0];
            instructions.push_back(Instruction(MOVE_ABS, startPoint.x*scaleFactor + startX, startPoint.y*scaleFactor + startY ));
            cout << "---- MOVE FOR #" << i + 1 << " TO: [ " << startPoint << " ]\n";
		}
	}
    cout << "--COMPLETE: x INSTRUCTIONS CREATED\n" << "--NOW PRINT";
    if (instructions.size() > 0) {
        instructions.push_back(Instruction(MOVE_ABS, home_x, home_y ));
        currentlyPlotting = true;
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(0);
	
    ofSetColor(255);
    gray.draw(0, 0);
    int y = 0;
    cropped.draw(gray.getWidth(), 0);
    cld.draw(gray.getWidth(), (y+=cropped.getHeight()));
    thresholded.draw(gray.getWidth(), (y+=cld.getHeight()));
    thinned.draw(gray.getWidth(), (y+=thresholded.getHeight()));

    cam.draw(0, gray.getHeight(), gray.getWidth(), gray.getHeight());
    
    ofPushMatrix();
    ofTranslate(gray.getWidth(), 0);
    drawPaths();
    ofPopMatrix();

    ofPushMatrix();
    ofTranslate(gray.getWidth() + cropped.getWidth(), 0);
    ofScale(2, 2);
    ofPushStyle();
    ofSetLineWidth(3);
    drawPaths();
    ofPopStyle();
    ofPopMatrix();
    
//    AT.draw();
    
    if (plotterReady) {
        ofSetColor(0, 255, 0);
    } else {
        ofSetColor(255, 0, 0);
    }
    ofCircle(20, 20, 10);
    
    ofSetColor(225);
    font.drawString("Plotter Ready", 40, 25);
    
    if (runOnTimer) {
        ofSetColor(0, 255, 0);
    } else {
        ofSetColor(255, 0, 0);
    }
    ofCircle(20, 60, 10);
    
    ofSetColor(225);
    font.drawString("Auto (r)", 40, 65);
    
    if (currentlyPlotting) {
        ofSetColor(0, 255, 0);
    } else {
        ofSetColor(255, 0, 0);
    }
    ofCircle(20, 100, 10);
    
    ofSetColor(225);
    font.drawString("Plotting", 40, 105);
    
    if (firstLastDraw) {
        ofSetColor(0, 255, 0);
    } else {
        ofSetColor(255, 0, 0);
    }
    ofCircle(20, 140, 10);
    
    ofSetColor(225);
    font.drawString("Short Draw (1)", 40, 145);
    
    if (randomLoc) {
        ofSetColor(0, 255, 0);
    } else {
        ofSetColor(255, 0, 0);
    }
    ofCircle(20, 180, 10);
    
    ofSetColor(225);
    font.drawString("Random Location (2)", 40, 185);
}

int ofApp::getPoints(int steps){
    int points = float(steps) / 100 / 80 * croppedSize;
//    cout << "getPoints: " << points << endl;
    return points;
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {	
    switch (key) {
        case 'N':
            // if coffee photo is good, press b
            // sends the arm up to start over, ready for a new person
        //    AT.shootCoffee();
            break;
        case ' ':
            // when the arm is up press space to take a face photo
            // press again until you like it
            needToUpdate = true;
            break;
        case 'p':
            // once you have a good face photo, press p
            // machine will lower and go home then print starts automatically
            // after print machine raises up and takes a coffee photo
//        //    AT.curState = AT.HOME;
            if (paths.size() > 0) {
                cout << "--PRINT REQUESTED\n";
                pathsToInstructions();
            //    AT.curState = AT.FACE_PHOTO;
            } else {
            //    AT.curState = AT.NEED_PHOTO;
            }
            break;
        case 'g':
            // GO PRINT IT!!
            if (instructions.size() > 0) {
                cout << "--PLOT REQUESTED\n";
                currentlyPlotting = true;
            }
            break;
        case 'c':
            // take a look at the coffee photo, if it's not good press c
            // retake photo of coffee (or make this happen in spacebar for consistency)
            break;
            
        // programmers controls
        case 'a':
            // load pattern: 20,400 steps in Y with 500 step X zigs every 500 Y steps
            
            for (int i = 21; i > 0; i--) {
                int x = getPoints(400);
                if (i%2) { x += getPoints(400); }
                int y = getPoints(i*400);
                ln.addVertex(x + croppedSize, y);
            }
            ln.addVertex(getPoints(2000) + croppedSize, getPoints(400));
            ln.addVertex(getPoints(2000) + croppedSize, getPoints(8400));
            ln.addVertex(getPoints(800) + croppedSize, getPoints(8400));
            
            paths.push_back(ln);
        //    AT.lock();
        //    AT.paths = paths;
        //    AT.points = paths.begin()->getVertices();
        //    AT.unlock();
            break;
        case 'R':
        //    AT.curState = AT.RESET;
        //    AT.reset();
            break;
        case 's':
            // stop in the middle of a print and reset with same image
            // must manually put machine back at origin
        //    AT.curState = AT.SHOOT_FACE;
        //    AT.lock();
        //    AT.paths = paths;
        //    AT.points = paths.begin()->getVertices();
        //    AT.unlock();
        // image1: save the current image
        case 'i':
            ofSaveImage(thinned, "saved_image_io.png");
            break;
        // image1: load the image
        case 'o':
            ofLoadImage(io, "saved_image_io.png");
            io.update();
            paths = getPaths(io, minGapLength, minPathLength);
        //    AT.lock();
        //    AT.paths = paths;
        //    AT.points = paths.begin()->getVertices();
        //    AT.unlock();
            break;
        // image2: save the current image
        case 'j':
            ofSaveImage(thinned, "saved_image_jk.png");
            break;
        // image2: load the image
        case 'k':
            ofLoadImage(jk, "saved_image_jk.png");
            jk.update();
            paths = getPaths(jk, minGapLength, minPathLength);
        //    AT.lock();
        //    AT.paths = paths;
        //    AT.points = paths.begin()->getVertices();
        //    AT.unlock();
            break;
        // image3: save the current image
        case 'n':
            ofSaveImage(thinned, "saved_image_nm.png");
            break;
        // image3: load the image
        case 'm':
            ofLoadImage(nm, "saved_image_nm.png");
            nm.update();
            paths = getPaths(nm, minGapLength, minPathLength);
        //    AT.lock();
        //    AT.paths = paths;
        //    AT.points = paths.begin()->getVertices();
        //    AT.unlock();
            break;
        case '0':
            cout << "-- RETURN TO HOME -- " << endl;
            message = "M " + ofToString(gui.getValueI("home_x")) + " " + ofToString(gui.getValueI("home_y")) + "/n";
            serial.writeString(message);
            message = "";
            break;
        case '1':
            if (firstLastDraw) {
                cout << "-- firstLastDraw OFF -- " << endl;
                firstLastDraw = false;
            } else {
                cout << "-- firstLastDraw ON -- " << endl;
                firstLastDraw = true;
            }
        //    AT.DELAY_MIN = 500;
            break;
        case '2':
            if (randomLoc) {
                cout << "-- randomLoc OFF -- " << endl;
                randomLoc = false;
            } else {
                cout << "-- randomLoc ON -- " << endl;
                randomLoc = true;
            }
        //    AT.DELAY_MIN = 600;
            break;
        case '3':
        //    AT.DELAY_MIN = 700;
            break;
        case '4':
        //    AT.DELAY_MIN = 800;
            break;
        case '5':
        //    AT.DELAY_MIN = 900;
            break;
        case '6':
        //    AT.DELAY_MIN = 1000;
            break;
        case 'q':
        //    AT.HIGH_DELAY = 50;
            break;
        case 'w':
        //    AT.HIGH_DELAY = 60;
            break;
        case 'e':
        //    AT.HIGH_DELAY = 70;
            break;
        case 'r':
            if (runOnTimer) {
                cout << "-- TIMER OFF -- " << endl;
                runOnTimer = false;
            } else {
                cout << "-- TIMER ON -- " << endl;
                runOnTimer = true;
            }
        //    AT.HIGH_DELAY = 80;
            break;
        case 't':
        //    AT.HIGH_DELAY = 90;
            break;
        case 'y':
        //    AT.HIGH_DELAY = 100;
            break;
        case 'h':
            // for debugging
        //    AT.goHome();
            break;
        case OF_KEY_RIGHT:
        //    AT.jogRight();
        //    AT.curState = AT.JOG;
            break;
        case OF_KEY_LEFT:
        //    AT.jogLeft();
        //    AT.curState = AT.JOG;
            break;
        case OF_KEY_UP:
        //    AT.jogForward();
        //    AT.curState = AT.JOG;
            break;
        case OF_KEY_DOWN:
        //    AT.jogBack();
        //    AT.curState = AT.JOG;
            break;
        case OF_KEY_HOME:
        //    AT.jogUp();
        //    AT.curState = AT.JOG;
            break;
        case OF_KEY_END:
        //    AT.jogDown();
        //    AT.curState = AT.JOG;
            break;
        case OF_KEY_PAGE_UP:
        //    AT.plungerUp();
            break;
        case OF_KEY_PAGE_DOWN:
        //    AT.plungerDown();
            break;
        default:
            break;
    }
}

void ofApp::exit() {
//    AT.stop();
    ofSleepMillis(1000);
}



