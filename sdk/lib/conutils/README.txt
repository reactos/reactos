The ReactOS Console Utilities Library v0.1
==========================================

COPYRIGHT:  Under GPLv2, see COPYING in the top level directory.
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

0. 'conutils_base' (utils.c and utils.h): Base set of functions for loading
   string resources and message strings, and handle type identification.

1. 'conutils_stream' (stream.c and stream.h): Console Stream API (CON_STREAM):
   Stream initialization, basic ConStreamRead/Write. Stream utility functions:
   ConPuts/Printf, ConResPuts/Printf, ConMsgPuts/Printf.
   Depends on 'conutils_base'.

2. 'conutils_screen' (screen.c and screen.h): Console Screen API (CON_SCREEN):
   Introduces the notion of console/terminal screen around the streams. Manages
   console/terminal screen metrics for Win32 consoles and TTYs (serial...).
   Additional Screen utility functions.
   Depends on 'conutils_stream', and indirectly on 'conutils_base'.

3. 'conutils_pager' (pager.c and pager.h): Console Pager API (CON_PAGER):
   Implements core console/terminal paging functionality around console screens.
   Depends on 'conutils_screen'  and indirectly on 'conutils_stream' and
   'conutils_base'.

4. More to come!
