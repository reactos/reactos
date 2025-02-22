The ReactOS Console Utilities Library v0.2
==========================================

LICENSE:    GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
COPYRIGHT:  Copyright 2017-2018 ReactOS Team
            Copyright 2017-2018 Hermes Belusca-Maito
CREDITS:    Thanks to the many people who originally wrote the code that finally
            ended up inside this library, with more or less refactoring, or
            whose code served as a basis for some functions of the library.


INTRODUCTION
~-~-~-~-~-~-

This library contains common functions used in many places inside the ReactOS
console utilities and the ReactOS Command-Line Interpreter. Most of these
functions are related with internationalisation and the problem of correctly
displaying Unicode text on the console. Besides those, helpful functions for
retrieving strings and messages from application resources are provided,
together with printf-like functionality.


CONTENTS
~-~-~-~-

-- Main ConUtils Library --

0. "BASE" (utils.c and utils.h): Base set of functions for loading
   string resources and message strings, and handle type identification.

1. "STREAM" (stream.c and stream.h, instream.c and instream.h, outstream.c
   and outstream.h): Console Stream API (CON_STREAM):
   Stream initialization, basic ConStreamRead/Write. Stream utility functions:
   ConPuts/Printf, ConResPuts/Printf, ConMsgPuts/Printf. Depends on "BASE".

2. "SCREEN" (screen.c and screen.h): Console Screen API (CON_SCREEN):
   Introduces the notion of console/terminal screen around the streams. Manages
   console/terminal screen metrics for Win32 consoles and TTYs (serial...).
   Additional Screen utility functions.
   Depends on "STREAM", and indirectly on "BASE".

3. "PAGER" (pager.c and pager.h): Console Pager API (CON_PAGER):
   Implements core console/terminal paging functionality around console screens.
   Depends on "SCREEN", and indirectly on "STREAM" and "BASE".
