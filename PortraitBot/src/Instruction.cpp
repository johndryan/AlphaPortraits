//
//  Instruction.c
//  plotterChat
//
//  Created by John Ryan on 09/03/2013.
//
//

// MOVE_ABS = 0;
// MOVE_REL = 1;
// LINE_ABS = 2;
// LINE_REL = 3;

#include "Instruction.h"

Instruction::Instruction(int _type, float _x, float _y)
{
    x = _x;
    y = _y;
    type = _type;
}

string Instruction::toString(){
    string commandChar = "";
    
    switch (type) {
        case 0:
            commandChar = "M";
            break;
        case 1:
            commandChar = "m";
            break;
        case 2:
            commandChar = "L";
            break;
        case 3:
            commandChar = "l";
            break;
    }
    
    commandChar = commandChar + " " + ofToString(x) + " " + ofToString(y) + "\n";
    
    return commandChar;
}