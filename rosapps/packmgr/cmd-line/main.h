///////////////////////////////////////////////////
//
// main.h
//				main.cpp's lumber room :)
///////////////////////////////////////////////////

#include "package.hpp"

#include <iostream>


vector<string> cmdline;
bool done = false;

int Help (void);
int Install (void);
int Show (void);

int SetStatus (int status1, int status2, WCHAR* text);
