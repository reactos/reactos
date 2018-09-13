//--------------------------------------------------------------------------------
//sudefs.h    
//started - Umesh Madan
//(c)Microsoft Corporation 1995
//standard alert ids etc 
//--------------------------------------------------------------------------------


#ifndef _SUDEFS_
#define _SUDEFS_

//Return codes.
#define DIALFAILED			0
#define DIALSUCCESS			1
#define DIALCANCEL			2
#define	SIGNUPTHROTTLED		3
#define SIGNUPWANTED		4		//do signup
#define	SIGNUPDECLINED		5		//don't do signup
#define SIGNUPSETUPONLY		6		//only do setup
#define TOLLFREECANCEL		7		
#define TOLLFREEOK			8
#define PHONESYNCOK			9
#define PHONESYNCCANCEL		10
#define AUTOPICKOK			11
#define AUTOPICKCANCEL		12
#define SIGNUPDONE			16
#define SIGNUPCONTINUE		17
#define JOINOK				18
#define JOINCANCEL			19
#define JOINFAILED			20
#define LEGALAGREE			21
#define LEGALREFUSE			22
#define PRODINFOOK 			23
#define PRODINFOFAILED		24
#define USERPASSOK			25
#define USERPASSCANCEL		26
#define USERPASSFAILED		27
#define USERPASSRETRY		28
#define USERPASSACCTERROR	29	  
#define USERPASSBADCREDIT	30
#define LOCKOUTOK			31
#define LOCKOUTFAILED		32 

//Alerts - tells the inherited class what alerts to put up.
#define ALERTIDCANCEL		1						//do a cancellation alert
#define ALERTIDRETRY		ALERTIDCANCEL + 1		//do a general retry this action alert
#define ALERTIDSETTINGS		ALERTIDRETRY + 1		//display connection settings etc..
#define ALERTIDGENERAL		ALERTIDSETTINGS + 1		//general no retry alert.. 
#define ALERTIDNOMODEM		ALERTIDGENERAL + 1		//no modem alert
#define ALERTIDLINEDROPPED	ALERTIDNOMODEM + 1		//line dropped alert.
#define ALERTIDOOM			ALERTIDLINEDROPPED + 1	//out of memory
#define ALERTIDFTMERROR		ALERTIDOOM + 1			//ftm error
#define ALERTIDNOTEXT		ALERTIDFTMERROR + 1		//blank edit field
#define ALERTIDDBCS			ALERTIDNOTEXT + 1		//DBCS characters found... 
#define ALERTIDLESSTEXT		ALERTIDDBCS + 1			//not enough text in edit field 
#define ALERTIDHASSPACES    ALERTIDLESSTEXT + 1		//string has spaces

//Status - tells the inherited class the status of the call
#define STATUSIDINIT		1		//initializing..
#define STATUSIDDIAL		2		//dialing..
#define STATUSIDCONNECT		3		//connected..
#define STATUSIDDISCONNECT	4		//disconnected..
#define STATUSIDCANCELLING	5		//cancelling..
#define STATUSIDCANCEL		6		//cancelled..
#define STATUSTRANSFER		7		//transferring data..

#endif
