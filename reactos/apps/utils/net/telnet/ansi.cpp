/* $Id: ansi.cpp,v 1.1 2001/01/27 22:38:43 ea Exp $
 *
 * FILE       : ansi.cpp
 * AUTHOR     : unknown (sources found on www.telnet.org)
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: telnet client for the W32 subsystem
 * DATE       : 2001-01-21
 * REVISIONS
 *	2001-02-21 ea	Modified to compile under 0.0.16 src tree
 */
#include <winsock.h>
#include <windows.h>

#include "telnet.h"

// Need to implement a Keymapper.
// here are some example key maps

// vt100 f1 - \eOP
// vt100 f2 - \eOQ

// ansi  f5 - \e[17~
//       f6 - \e[18~
//       f7 - \e[20~
//       f10- \e[[V

enum _ansi_state
{
  as_normal,
  as_esc,
  as_esc1
};

//SetConsoleMode

///////////////////////////////////////////////////////////////////////////////
// SET SCREEN ATTRIBUTE
/*
ESC [ Ps..Ps m  Ps refers to selective parameter. Multiple parameters are
                separated by the semicolon character (073 octal). The param-
                eters are executed in order and have the following meaning:

                0 or none               All attributes off
                1                       Bold on
                4                       Underscore on
                5                       Blink on
                7                       Reverse video on

                3x                      set foreground color to x
                nx                      set background color to x

                Any other parameters are ignored.
*/
static int sa = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

void ansi_set_screen_attribute(char* buffer)
{
  while(*buffer)
  {
    switch(*buffer++)
    {
    case '0': //Normal
      sa = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
      break;
    case '1': //Hign Intensity
      sa |= FOREGROUND_INTENSITY;
      break;
    case '4': //Underscore
      break;
    case '5': //Blink.
      sa |= BACKGROUND_INTENSITY;
      break;
    case '7':
      sa = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
      break;
    case '8':
      sa = 0;
      break;
    case '3':
      sa = sa & (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY) |
        (*buffer & 1)?FOREGROUND_RED:0 |
        (*buffer & 2)?FOREGROUND_GREEN:0 |
        (*buffer & 4)?FOREGROUND_BLUE:0;
      if(*buffer)
        buffer++;
      break;
    case '6':
      sa = sa & (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY) |
        (*buffer & 1)?BACKGROUND_RED:0 |
        (*buffer & 2)?BACKGROUND_GREEN:0 |
        (*buffer & 4)?BACKGROUND_BLUE:0;
      if(*buffer)
        buffer++;
      break;
    }
    if(*buffer && *buffer == ';')
      buffer++;
  }
  SetConsoleTextAttribute(StandardOutput,sa);
}

///////////////////////////////////////////////////////////////////////////////
// ERASE LINE
/*
ESC [ 0K        Same *default*
ESC [ 1K        Erase from beginning of line to cursor
ESC [ 2K        Erase line containing cursor
*/

void ansi_erase_line(char* buffer)
{
  int act = 0;
  while(*buffer)
  {
    act = (*buffer++) - '0';
  }

  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(StandardOutput,&csbi);

  COORD pos;
  DWORD n;

  switch(act)
  {
  case 0: //erase to end of line
    pos.X = csbi.dwCursorPosition.X;
    pos.Y = csbi.dwCursorPosition.Y;
    n = csbi.dwSize.X - csbi.dwCursorPosition.X;
    break;
  case 1: //erase from beginning
    pos.X = 0;
    pos.Y = csbi.dwCursorPosition.Y;
    n = csbi.dwCursorPosition.X;
    break;
  case 2: // erase whole line
    pos.X = 0;
    pos.Y = csbi.dwCursorPosition.Y;
    n = csbi.dwSize.X;
    break;
  }

  DWORD w;
  FillConsoleOutputCharacter(StandardOutput,' ',n,pos,&w);
}


///////////////////////////////////////////////////////////////////////////////
// SET POSITION
// ESC [ Pl;PcH    Direct cursor addressing, where Pl is line#, Pc is column#
// default = (1,1)

void ansi_set_position(char* buffer)
{
  COORD pos = {0,0};

  // Grab line
  while(*buffer && *buffer != ';')
    pos.Y = pos.Y*10 + *buffer++ - '0';

  if(*buffer)
    buffer++;

  // Grab y
  while(*buffer && *buffer != ';')
    pos.X = pos.X*10 + *buffer++ - '0';

  (pos.X)?pos.X--:0;
  (pos.Y)?pos.Y--:0;

  SetConsoleCursorPosition(StandardOutput,pos);
 
}

///////////////////////////////////////////////////////////////////////////////
// ERASE SCREEN
/*
ESC [ 0J        Same *default*
ESC [ 2J        Erase entire screen
*/

void ansi_erase_screen(char* buffer)
{
  int act = 0;
  while(*buffer)
  {
    act = (*buffer++) - '0';
  }

  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(StandardOutput,&csbi);

  COORD pos;
  DWORD n;

  switch(act)
  {
  case 0:
    pos.X = csbi.dwCursorPosition.X;
    pos.Y = csbi.dwCursorPosition.Y;
    n = csbi.dwSize.X*csbi.dwSize.Y;
    break;
  case 2:
    pos.X = 0;
    pos.Y = 0;
    n = csbi.dwSize.X*csbi.dwSize.Y;
    break;
  }

  DWORD w;
  FillConsoleOutputCharacter(StandardOutput,' ',n,pos,&w);
  SetConsoleCursorPosition(StandardOutput,pos);
}

///////////////////////////////////////////////////////////////////////////////
// MOVE UP
// ESC [ Pn A      Cursor up Pn lines (Pn default=1)

void ansi_move_up(char* buffer)
{
  int cnt = *buffer?0:1;
  while(*buffer)
  {
    cnt = cnt*10 + (*buffer++) - '0';
  }

  COORD pos;

  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(StandardOutput,&csbi);

  pos.X = csbi.dwCursorPosition.X;
  pos.Y = ((csbi.dwCursorPosition.Y-cnt)>=0)?(csbi.dwCursorPosition.Y-cnt):0;

  SetConsoleCursorPosition(StandardOutput,pos);
}

///////////////////////////////////////////////////////////////////////////////
char codebuf[256];
unsigned char codeptr;

#define NUM_CODEC 6

typedef void (*LPCODEPROC)(char*);

struct 
{
  unsigned char cmd;
  LPCODEPROC proc;
} codec[NUM_CODEC] = {
  {'m',ansi_set_screen_attribute},
  {'H',ansi_set_position},
  {'K',ansi_erase_line},
  {'J',ansi_erase_screen},
  {'A',ansi_move_up},
  {0,0}
};

void ansi(SOCKET server,unsigned char data)
{
  static _ansi_state state = as_normal;
  DWORD z;
  switch( state)
  {
  case as_normal:
    switch(data)
    {
    case 0:  //eat null codes.
      break;
    case 27: //ANSI esc.
      state = as_esc;
      break;
    default: //Send all else to the console.
      WriteConsole(StandardOutput,&data,1,&z,NULL);
      break;
    }
    break;
  case as_esc:
    state = as_esc1;
    codeptr=0;
    codebuf[codeptr] = 0;
    break;
  case as_esc1:
    if(data > 64)
    {
      int i = 0;
      codebuf[codeptr] = 0;
      for(i=0; codec[i].cmd && codec[i].cmd != data; i++);
      if(codec[i].proc)
        codec[i].proc(codebuf);
#ifdef _DEBUG
      else
      {
        char buf[256];
        wsprintf(buf,"Unknown Ansi code:'%c' (%s)\n",data,codebuf);
        OutputDebugString(buf);
      }
#endif
      state = as_normal;
    }
    else
      codebuf[codeptr++] = data;
    break;
  }
}

/* EOF */
