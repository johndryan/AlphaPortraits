//
//  Instruction.h
//  plotterChat
//
//  Created by John Ryan on 09/03/2013.
//
//

#ifndef ofApp_Instruction_h
#define ofApp_Instruction_h

#include "ofMain.h"

class Instruction {
    
    public: // place public functions or variables declarations here
        
        // methods, equivalent to specific functions of your class objects
        string toString();  // update method, used to refresh your objects properties
        
        // variables
        float x;      // position
        float y;
        int type;      // size
        
        Instruction(int type, float x, float y); // constructor - used to initialize an object, if no properties are passed
        //               the program sets them to the default value
        
    private: // place private functions or variables declarations here
    
}; // dont't forget the semicolon!!

#endif
