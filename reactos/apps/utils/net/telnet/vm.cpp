/* $Id: vm.cpp,v 1.1 2001/01/27 22:38:43 ea Exp $
 *
 * FILE       : vm.cpp
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

//      NAME          CODE   MEANING
// NVT codes
#define NUL     0    // No Operation
#define BEL     7    // BELL 
#define BS      8    // Back Space 
#define HT      9    // Horizontal Tab 
#define LF     10    // Line Feed 
#define VT     11    // Vertical Tab 
#define FF     12    // Form Feed 
#define CR     13    // Carriage Return 
  
// telnet command codes
#define SE    240    // End of subnegotiation parameters.
#define NOP   241    // No operation.
#define DM    242    // Data Mark
#define BRK   243    // Break
#define IP    244    // Interrupt Process
#define AO    245    // Abort output
#define AYT   246    // Are You There
#define EC    247    // Erase character
#define EL    248    // Erase Line          
#define GA    249    // Go ahead
#define SB    250    // SuBnegotiate
#define WILL  251    // 
#define WONT  252    // 
#define DO    253    // 
#define DONT  254    // 
#define IAC   255    // Interpret As Command

//Telnet options: 
//    0x00 - binary mode
//    0x01 - Local Echo
//    0x03 - Suppress GA (char at a time)
//    0x05 - status
//    0x06 - Timing Mark   
//
// do 0x25 - zub?
//
//    0xff - Extended Options List

enum _option
{
  TOPT_BIN = 0,   // Binary Transmission
  TOPT_ECHO = 1,  // Echo
  TOPT_RECN = 2,  // Reconnection
  TOPT_SUPP = 3,  // Suppress Go Ahead
  TOPT_APRX = 4,  // Approx Message Size Negotiation
  TOPT_STAT = 5,  // Status
  TOPT_TIM = 6,   // Timing Mark
  TOPT_REM = 7,   // Remote Controlled Trans and Echo
  TOPT_OLW = 8,   // Output Line Width
  TOPT_OPS = 9,   // Output Page Size
  TOPT_OCRD = 10, // Output Carriage-Return Disposition
  TOPT_OHT = 11,  // Output Horizontal Tabstops
  TOPT_OHTD = 12, // Output Horizontal Tab Disposition
  TOPT_OFD = 13,  // Output Formfeed Disposition
  TOPT_OVT = 14,  // Output Vertical Tabstops
  TOPT_OVTD = 15, // Output Vertical Tab Disposition
  TOPT_OLD = 16,  // Output Linefeed Disposition
  TOPT_EXT = 17,  // Extended ASCII
  TOPT_LOGO = 18, // Logout
  TOPT_BYTE = 19, // Byte Macro
  TOPT_DATA = 20, // Data Entry Terminal
  TOPT_SUP = 21,  // SUPDUP
  TOPT_SUPO = 22, // SUPDUP Output
  TOPT_SNDL = 23, // Send Location
  TOPT_TERM = 24, // Terminal Type
  TOPT_EOR = 25,  // End of Record
  TOPT_TACACS = 26, // TACACS User Identification
  TOPT_OM = 27,   // Output Marking
  TOPT_TLN = 28,  // Terminal Location Number
  TOPT_3270 = 29, // Telnet 3270 Regime
  TOPT_X3 = 30,  // X.3 PAD
  TOPT_NAWS = 31, // Negotiate About Window Size
  TOPT_TS = 32,   // Terminal Speed
  TOPT_RFC = 33,  // Remote Flow Control
  TOPT_LINE = 34, // Linemode
  TOPT_XDL = 35,  // X Display Location
  TOPT_ENVIR = 36,// Telnet Environment Option
  TOPT_AUTH = 37, // Telnet Authentication Option
  TOPT_NENVIR = 39,// Telnet Environment Option
  TOPT_EXTOP = 255, // Extended-Options-List
  TOPT_ERROR = 256  // Magic number
};

// Wanted by linux box:
// 37 - TOPT_AUTH
// 24 - TOPT_TERM

enum _verb
{
  verb_sb   = 250,
  verb_will = 251,
  verb_wont = 252,
  verb_do   = 253, 
  verb_dont = 254
};

enum _state
{
  state_data,   //we expect a data byte
  state_code,   //we expect a code
  state_option  //we expect an option
};

int option_error(_verb,_option,int,SOCKET);

typedef void(*LPOPTIONPROC)(SOCKET,_verb,_option);
typedef void(*LPDATAPROC)(SOCKET,unsigned char data);

///////////////////////////////////////////////////////////////////////////////

inline void yesreply(SOCKET server, _verb verb,_option option)
{
  unsigned char buf[3];
  buf[0] = IAC;
  buf[1] = (verb==verb_do)?WILL:(verb==verb_dont)?WONT:(verb==verb_will)?DO:DONT;
  buf[2] = (unsigned char)option;
  send(server,(char*)buf,3,0);
}

inline void noreply(SOCKET server, _verb verb,_option option)
{
  unsigned char buf[3];
  buf[0] = IAC;
  buf[1] = (verb==verb_do)?WONT:(verb==verb_dont)?WILL:(verb==verb_will)?DONT:DO;
  buf[2] = (unsigned char)option;
  send(server,(char*)buf,3,0);
}

inline void askfor(SOCKET server, _verb verb,_option option)
{
  unsigned char buf[3];
  buf[0] = IAC;
  buf[1] = (unsigned char)verb;
  buf[2] = (unsigned char)option;
  send(server,(char*)buf,3,0);
}


void ddww_error(SOCKET server,_verb verb,_option option)
{
#ifdef _DEBUG
  char tmp[256];
  wsprintf(tmp,"Unknown Option Code: %s, %i\n",(verb==verb_do)?"DO":(verb==verb_dont)?"DON'T":(verb==verb_will)?"WILL":"WONT",(int)option);
  OutputDebugString(tmp);
#endif

  switch(verb)
  {
  case verb_will: // server wants to support something
    noreply(server,verb,option); // I don't want that.
    break;
  case verb_wont: // server waants to disable support
    return;       // don't confirm - already disabled.
  case verb_do:   // server wants me to support something
    noreply(server,verb,option); //I won't do that
    break;
  case verb_dont: // server wants me to disable something
    return;       // don't worry, we don't do that anyway (I hope :)
  }
}

///////////////////////////////////////////////////////////////////////////////
// Option ECHO & SUPPRESS GA
//
// These options are curiously intertwined...
// The Win32 console doesn't support ECHO_INPUT (echo) if
// LINE_INPUT (==GA) isn't set.
// I can't see how to code this negotiation without using
// some form of Lock-Step algorythm
// ie: if("WILL ECHO")
//       Send "DO SUPP"
//       if("WILL SUPP")
//         Send "DO ECHO"
//       else 
//         Send "DONT ECHO"


void ddww_echo(SOCKET server,_verb verb, _option option)
{
  DWORD mode;
  GetConsoleMode(StandardInput, & mode); // ENABLE_ECHO_INPUT
  
  int set = !(mode & ENABLE_ECHO_INPUT);

  switch(verb)
  {
  case verb_will: // server wants to echo stuff
    if(set) return; //don't confirm - already set.
    SetConsoleMode(StandardInput,mode & (~ENABLE_ECHO_INPUT));
    break;
  case verb_wont: // server don't want to echo
    if(!set) return; //don't confirm - already unset.
    SetConsoleMode(StandardInput,mode | ENABLE_ECHO_INPUT);
    break;
  case verb_do:   // server wants me to loopback
    noreply(server,verb,option);
    return;
  case verb_dont: // server doesn't want me to echo
    break;        // don't bother to reply - I don't
  }
  yesreply(server,verb,option);
}


void ddww_supp(SOCKET server,_verb verb,_option option) //Suppress GA
{
  DWORD mode;
  GetConsoleMode(StandardInput,&mode); // ENABLE_LINE_INPUT
  
  int set = !(mode & ENABLE_LINE_INPUT);

  switch(verb)
  {
  case verb_will: // server wants to suppress GA's
    if(set) break; //don't confirm - already set.
    SetConsoleMode(StandardInput,mode & (~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)));
    askfor(server,verb_do,TOPT_SUPP);
    askfor(server,verb_will,TOPT_SUPP);
    askfor(server,verb_do,TOPT_ECHO);
    break;
  case verb_wont: // server wants to send GA's 
    if(!set) break; //don't confirm - already unset.
    SetConsoleMode(StandardInput,mode | ENABLE_LINE_INPUT);
    askfor(server,verb_dont,TOPT_SUPP);
    askfor(server,verb_wont,TOPT_SUPP);
    break;
  case verb_do:   // server wants me to suppress GA's
    if(set) break;
    askfor(server,verb_do,TOPT_SUPP);
    break;
  case verb_dont: // server wants me to send GA's
    if(!set) break;
    askfor(server,verb_dont,TOPT_SUPP);
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Option TERMINAL-TYPE

void ddww_term(SOCKET server,_verb verb,_option option) //Subnegotiate terminal type
{
  switch(verb)
  {
  case verb_will:
    noreply(server,verb,option); // I don't want terminal info
    break;
  case verb_wont:
    //dat be cool - its not going to send. no need to confirm
    break;
  case verb_do:
    yesreply(server,verb,option); //I'll send it when asked
    break;
  case verb_dont://Ok - I won't
    break;
  }
}

// TERMINAL TYPE subnegotation
enum
{
  SB_TERM_IS = 0,
  SB_TERM_SEND = 1
};

#define NUM_TERMINALS 2

struct
{
  char* name;
  LPDATAPROC termproc;
  //pre requsites.
} terminal[NUM_TERMINALS] = {
  { "NVT", nvt }, 
  { "ANSI", ansi }
};

int term_index = 0;

void sbproc_term(SOCKET server,unsigned char data)
{

  if(data == SB_TERM_SEND)
  {
    if(term_index == NUM_TERMINALS)
      term_index = 0;
    else
      term_index++;
    char buf[16]; //pls limit 
    buf[0] = IAC;
    buf[1] = SB;
    buf[2] = TOPT_TERM;
    buf[3] = SB_TERM_IS;
    lstrcpy(&buf[4],terminal[(term_index==NUM_TERMINALS)?(NUM_TERMINALS-1):term_index].name);
    int nlen = lstrlen(&buf[4]);
    buf[4+nlen] = IAC;
    buf[5+nlen] = SE;
    send(server,buf,4+nlen+2,0);
  }
}

///////////////////////////////////////////////////////////////////////////////

struct
{
  _option option;
  LPOPTIONPROC OptionProc;
  LPDATAPROC DataProc;
}  ol[] = {
  {TOPT_ECHO,   ddww_echo,  NULL},
  {TOPT_SUPP,   ddww_supp,  NULL},
  {TOPT_TERM,   ddww_term,  sbproc_term},
  {TOPT_ERROR,  ddww_error, NULL}
};


void vm(SOCKET server,unsigned char code)
{
//These vars are the finite state
  static int state = state_data;
  static _verb verb = verb_sb;
  static LPDATAPROC DataProc = terminal[(term_index==NUM_TERMINALS)?(NUM_TERMINALS-1):term_index].termproc;
// for index
  int i=0;

//Decide what to do (state based)
  switch(state)
  {
  case state_data:
    switch(code)
    {
    case IAC: state = state_code; break;
    default: DataProc(server,code);
    }
    break;
  case state_code:
    state = state_data;
    switch(code)
    {
    // State transition back to data
    case IAC: 
      DataProc(server,code);
      break;
    // Code state transitions back to data
    case SE:
      DataProc = terminal[(term_index==NUM_TERMINALS)?(NUM_TERMINALS-1):term_index].termproc;
      break;
    case NOP:
      break;
    case DM:
      break;
    case BRK:
      break;
    case IP:
      break;
    case AO:
      break;
    case AYT:
      break;
    case EC:
      break;
    case EL:
      break;
    case GA:
      break;
    // Transitions to option state
    case SB:
      verb = verb_sb;
      state = state_option;
      break;
    case WILL:
      verb = verb_will;
      state = state_option;
      break;
    case WONT:
      verb = verb_wont;
      state = state_option;
      break;
    case DO:
      verb = verb_do;
      state = state_option;
      break;
    case DONT:
      verb = verb_dont;
      state = state_option;
      break;
    }
    break;
  case state_option:
    state = state_data;

    //Find the option entry
    for(
      i = 0;
      ol[i].option != TOPT_ERROR && ol[i].option != code;
      i++);

    //Do some verb specific stuff
    if(verb == verb_sb)
      DataProc = ol[i].DataProc;
    else
      ol[i].OptionProc(server,verb,(_option)code);
    break;
  }
}

/* EOF */
