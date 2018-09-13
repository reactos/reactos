%{  /* <-- needed */
///*++
//
//Copyright (c) 1992-1996  Microsoft Corporation
//
//Module Name:
//
//    miblex.l
//
//Abstract:
//
//    This file is the FLEX specification for the MIB compiler.
//
//Environment:
//
//    User Mode - Win32
//
//Revision History:
//
//    10-May-1996 DonRyan
//        Removed banner from Technology Dynamics, Inc.
//
//--*/
   
#include<string.h>

#ifdef WIN32	/* if compiling for NT */
#include<io.h>
#endif

#include "mibyacc.h"	/* for defines for token ids */
#include "mibcc.h"	/* for global variables */


//#define DONTSUCKEXPORTS
//#define DONTSUCKIMPORTS

extern int yy_init;		/* whether we need to initialize */

void mark_flex_to_init(void) {
   yy_init = 1;
}

BOOL warning_level(unsigned int nLevel) {
   if (nWarningLevel >= nLevel)
      return TRUE;
   else
      return FALSE;
}

void comment (void)
{
   char c, lastc=' ';
   while(c = (char) input()) {
      if ('\n' == c) { /* line feed ends a comment */
         // increment the number of lines seen
         lineno++;
         break;
      }
      if ((lastc == '-') && (c == '-')) { /* two dashes ends a comment */
         break;
      }
      lastc = c;
   }
}

void string (void)
{
   char c;
   while(c = (char) input()) {
      if ('\"' == c)
         break;
      if ('\n' == c) /* keep track of line feeds */
         lineno++;
   }
}

void suckUpImports (void)
{
   char c;
   if (warning_level(2)) {
      fprintf (error_out, "warning : IMPORTS on line %i not supported (ignored)\n", lineno);
   }
   while(c = (char) input()) {
      if (';' == c)
         break;
      if ('\n' == c) /* keep track of line feeds */
         lineno++;
   }
}

void suckUpExports (void)
{
   char c;
   if (warning_level(2)) {
      fprintf (error_out, "warning : EXPORTS on line %i not supported (ignored)\n", lineno);
   }
   while(c = (char) input()) {
      if (';' == c)
         break;
      if ('\n' == c) /* keep track of line feeds */
         lineno++;
   }
}

%}

A	[Aa]
B	[Bb]
C	[Cc]
D	[Dd]
E	[Ee]
F	[Ff]
G	[Gg]
H	[Hh]
I	[Ii]
J	[Jj]
K	[Kk]
L	[Ll]
M	[Mm]
N	[Nn]
O	[Oo]
P	[Pp]
Q	[Qq]
R	[Rr]
S	[Ss]
T	[Tt]
U	[Uu]
V	[Vv]
W	[Ww]
X	[Xx]
Y	[Yy]
Z	[Zz]
ALPHA	[a-zA-ZÀ-ÿ]
DIGIT	[0-9]
SEXIT	[0-5]
HEXALPHA	[a-fA-F]
OCTECT	({DIGIT}|{DIGIT}{DIGIT}|1{DIGIT}{DIGIT}|2{SEXIT}{SEXIT})
%%
\n						{ lineno++; /* line feed */ }
\f						{ /* form feed */ }
\r						{ /* carriage return (no line feed) */ }
\v						{ /* vertical tab */ }
[ \t\32]					{ /* white space */ }
ACCESS					{ return (kwACCESS); }
BEGIN					{ return (kwBEGIN); }
CHOICE					{ return (kwCHOICE); }
COUNTER					{ return (kwCOUNTER); }
DEFAULT					{ return (kwDEFAULT); }
DEFINITIONS				{ return (kwDEFINITIONS); }
DEFVAL					{ return (kwDEFVAL); }
DEPRECATED				{ return (kwDEPRECATED); }
DESCRIPTION				{ return (kwDESCRIPTION); }
END					{ return (kwEND); }
ENTERPRISE				{ return (kwENTERPRISE); }
EXPORTS					{ 
					#ifdef DONTSUCKEXPORTS
						return (kwEXPORTS);
					#else
						suckUpExports ();
					#endif
					}
FROM					{ return (kwFROM); }
GAUGE					{ return (kwGAUGE); }
IDENTIFIER				{ return (kwIDENTIFIER); }
IMPLICIT				{ return (kwIMPLICIT); }
IMPORTS					{
					#ifdef DONTSUCKIMPORTS
						return (kwIMPORTS);
					#else
						suckUpImports ();
					#endif
					}
INDEX					{ return (kwINDEX); }
INTEGER					{ return (kwINTEGER); }
IPADDRESS				{ return (kwIPADDRESS); }
MANDATORY				{ return (kwMANDATORY); }
MACRO					{ return (kwMACRO); }
NETWORKADDRESS				{ return (kwNETWORKADDRESS); }
NOT-ACCESSIBLE				{ return (kwNOT_ACCESSIBLE); }
NOTATION				{ return (kwNOTATION); }
NULL					{ return (kwNULL); }
OBJECT					{ return (kwOBJECT); }
OBJECT-TYPE				{ return (kwOBJECT_TYPE); }
OBSOLETE				{ return (kwOBSOLETE); }
OCTET					{ return (kwOCTET); }
OF					{ return (kwOF); }
OPAQUE					{ return (kwOPAQUE); }
OPTIONAL				{ return (kwOPTIONAL); }
READ-ONLY				{ return (kwREAD_ONLY); }
READ-WRITE				{ return (kwREAD_WRITE); }
REFERENCE				{ return (kwREFERENCE); }
SEQUENCE				{ return (kwSEQUENCE); }
{S}{I}{Z}{E}				{ return (kwSIZE); }
STATUS					{ return (kwSTATUS); }
STRING					{ return (kwSTRING); }
SYNTAX					{ return (kwSYNTAX); }
TIMETICKS				{ return (kwTIMETICKS); }
TRAP-TYPE				{ return (kwTRAP_TYPE); }
TYPE					{ return (kwTYPE); }
VARIABLES				{ return (kwVARIABLES); }
VALUE					{ return (kwVALUE); }
WRITE-ONLY				{ return (kwWRITE_ONLY); }
".."					{ return (tokDOTDOT); }
"::="					{ return (tokIS); }
{ALPHA}([_-]|{ALPHA}|{DIGIT})*		{ strcpy (yylval.strval, yytext);
					  return (tokNAME); }
{OCTECT}\.{OCTECT}\.{OCTECT}\.{OCTECT}	{ return (tokIP); }
{DIGIT}({DIGIT})*			{ yylval.numval = atol (yytext);
					  return (tokNUMBER); }
\'(0|1)*\'{B}				{ return (tokBINARYSTR); }
\'({DIGIT}|{HEXALPHA})*\'{H}		{ return (tokHEXSTR); }
\"	{ string (); return (tokSTRING); }
"--"	{ comment (); }
"("	|
")"	|
","	|
"{"	|
"}"	{ return (yytext[0]); }
.	{ return (yytext[0]); /* return the bad character */ }

