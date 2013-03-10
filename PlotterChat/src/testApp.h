#pragma once

#include "ofMain.h"
#include "ofxSimpleSerial.h"
#include "Instruction.h"

class testApp : public ofBaseApp{
	
	public:
		void setup();
		void update();
		void draw();
        void onNewMessage();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
		ofTrueTypeFont		font;

		bool		startSending;			// a flag for sending serial
        bool        backAndForth;
        bool        plotterReady;
        string      message;
        void		onNewMessage(string & message);
    
        ofxSimpleSerial	serial;
    
        Instruction** instructionList;
        int nInstructions, currentInstruction;
};

