#define MOVE_ABS 0
#define MOVE_REL 1
#define LINE_ABS 2
#define LINE_REL 3

#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
	ofSetVerticalSync(true);
	
	startSending = false;
	ofBackground(255);	
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	font.loadFont("DIN.otf", 64);
	
	serial.listDevices();
	vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();
	
	serial.setup("/dev/tty.usbmodemfa131",57600);
	serial.startContinuesRead(false);
	ofAddListener(serial.NEW_MESSAGE,this,&testApp::onNewMessage);
    
    nInstructions = 6;
    currentInstruction = 0;
    instructionList = new Instruction*[nInstructions];
    // Let's draw a cube!
    instructionList[0] = new Instruction(MOVE_REL, 1500, -1500);
    instructionList[1] = new Instruction(LINE_REL, -3000, 0);
    instructionList[2] = new Instruction(LINE_REL, 0, 3000);
    instructionList[3] = new Instruction(LINE_REL, 3000, 0);
    instructionList[4] = new Instruction(LINE_REL, 0, -3000);
    instructionList[5] = new Instruction(MOVE_REL, -1500, 1500);
    message = "";
    
    backAndForth = true;
    plotterReady = false;
}

void testApp::onNewMessage(string & message)
{
	cout << "onNewMessage, message: " << message << "\n";
	if (message == "OK") {
        cout << "-- PLOTTER READY --\n";
        plotterReady = true;
    }
}

//--------------------------------------------------------------
void testApp::update(){
    
//    int numSent = 0;
//    string message = myInstruction->toString();
//    
//    while (numSent < message.length()) {
//        string subMessage = message.substr (numSent, message.length()-numSent);
//        unsigned char *cMessage = subMessage.c_str();
//        numSent = serial.writeBytes(cMessage, subMessage.length());
//    }
	
	if (startSending){
        
        if(plotterReady)
        {
            cout << "-- Instruction #" << currentInstruction << " --\n";
            message = instructionList[currentInstruction]->toString();
            currentInstruction += 1;
            
            if (currentInstruction >= nInstructions) {
                startSending = false;
            }
            
            if(message != ""){
                cout << "sending message: " << message << "\n";
                plotterReady = false;
                serial.writeString(message);
                message = "";
            }
        }
	}
}

//--------------------------------------------------------------
void testApp::draw(){
	string msg;
	msg += "click to test serial:\n";
	font.drawString(msg, 50, 100);
}

//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){ 
	
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){
	
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	startSending = true;
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

