/*****************************************************************************/
/*  Data structure for DEFAULT AND PREFERENCES SETTING                       */
/*****************************************************************************/

#ifndef _DOOR_H		//v_mjgran: To avoid data redefinitions
#define _DOOR_H

typedef struct tagKBPREFINFO
{
	//keyboard preference
	int 			g_margin;         	// Margin between rows and columns
	BOOL			smallKb;			// TRUE when working with Small Keyboard
	COLORREF		PrefTextKeyColor;  		// Prefered Color for text in keys
	COLORREF 		PrefCharKeyColor; 		// normal key
	COLORREF 		PrefModifierKeyColor;	// modifier key
	COLORREF 	    PrefDeadKeyColor; 		// dead key
	COLORREF		PrefBackgroundColor; 	// ditto Keyboard backgraund
	int			    PrefDeltakeysize;		// Preference increment in key size
	BOOL			PrefshowActivekey;		// Show cap letters in keys
    int				KBLayout;				// 101, 102, 106, KB layout
	BOOL			Pref3dkey;              // Use 3d keys
	BOOL			Prefusesound;			// Use click sound
	BOOL			PrefAlwaysontop;		// windows always on top control
	BOOL			Prefhilitekey;			// True for hilite eky under cursor
	BOOL			PrefDwellinkey;			// use dwelling system
	UINT			PrefDwellTime;	  		// Dwell time preference
	LOGFONTA	    lf;						// default font


	//import Dlg
	BOOL			DICT_LRNNEXT;
	BOOL			DICT_LRNNUM;
	BOOL			bReadLonger;
	BOOL			bWordToKeep;
	int			    minLength;
	long			maxWords;

	//Predict Dlg
	BOOL			typeFast;      //use clipboard ?
	BOOL			WListVisib;    //Word list visible
	BOOL			PredictNext;   //Predict next words
	BOOL    		Cap;           //Cap after period
	int    		    ShortestWord;  //Shorest Word
	int             space;         //how many space AFTER SENTENCE
	BOOL    		VorH;          //v = vertical h = horizontal word list
                                   //  (TRUE is V)
	int             WordShown;     //how many predict keys to create
	int			    WordFound;	   //how many words to find
	LOGFONTA        PredLF;        //store the font for predict window
	COLORREF		PredTextColor; //prefer color for predict text
    COLORREF        PredKeyColor;  //prefer color for predict key
    BOOL            AddSpace;      //Add space after , : ; or not

    //Scanning Option
    UINT            uKBKey;         // vk of scan key
    BOOL            bKBKey;         // use scan key
    BOOL            bPort;         // open the serial, parallel, game port 

	//Option Dlg
	BOOL			DICT_LRNNEW;
	BOOL			DICT_LRNFREQ;
	BOOL			DICT_PURGAUTO;
	BOOL			DICT_AUTOINCREASE;
	BOOL            WAIT_DLG_SHOWWORDS;

	//size and position of KB and Predictor
	RECT            KB_Rect;
	RECT            Pred_Rect;
    RECT            Pred_Crect;
	float           Pred_Width;
    float           Pred_Height;

	//HotKeys Dlg
	UINT            HotKeyList[30];    //array to store HotKeys
	BOOL            HK_F11;            //disable / enable the f11 hot key
	BOOL            HK_Show;
	UINT            Choice;            //choices for Func, numbers, keypad, all
	BOOL			HK_Enable;         //diasable / enable hotkey
	BOOL            HK_Front;
	BOOL			FastSel;

	BOOL            PrefScanning;      //use scanning
	UINT            PrefScanTime;
	
	BOOL			Actual;            // T - Actual or F - Block KB

	//Application preference
	BOOL			fShowWarningAgain;		// Show the initial warning dialog again


} KBPREFINFO;

typedef KBPREFINFO*  LPKBPREFINFO;

//
// Pointer to keyboard preference and other dialog option
// and the buffer we use to save and read from file
//
extern KBPREFINFO   *kbPref;   

#endif   //_DOOR_H