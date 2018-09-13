/****************************************************************************

	DRIVERS.H

	This file defines the external calls and structures of the keyboard and 
	mouse drivers.

****************************************************************************/

typedef	struct tagKBDFILTERKEYSPARM {
	BYTE	fFilterKeysOn;		/* boolean 0 = false, 1 = true */
	BYTE	fOn_Off_Feedback;	/* boolean 0 = false, 1 = true */
	BYTE	fUser_SetUp_Option1;	/* boolean 0 = false, 1 = true */
	BYTE	fUser_SetUp_Option2;	/* boolean 0 = false, 1 = true */
	int		wait_ticks;			/* time till accept key 18.2 ticks/sec */
	int		delay_ticks;		/* delay to repeat		18.2 ticks/sec */
	int		repeat_ticks;		/* repeat rate			18.2 ticks/sec */
	int		bounce_ticks;		/* debounce rate		18.2 ticks/sec */
	BYTE	fRecovery_On;		/* boolean 0 = false, 1 = true */ 
	BYTE	fclick_on;			/* boolean 0 = false, 1 = true */ 
} KBDFILTERKEYSPARM;

extern	void FAR PASCAL Get_FilterKeys_Param(KBDFILTERKEYSPARM FAR *);	/* part of keyboard driver */
extern	void FAR PASCAL Set_FilterKeys_Param(KBDFILTERKEYSPARM FAR *);	/* part of keyboard driver */

typedef	struct tagKBDSTICKEYSPARM {
	BYTE	fSticKeysOn;		/* boolean 0 = false, 1 = true */
	BYTE	fOn_Off_Feedback;	/* boolean 0 = false, 1 = true */
	BYTE	fAudible_Feedback;	/* boolean 0 = false, 1 = true */
	BYTE	fTriState;			/* boolean 0 = false, 1 = true */
	BYTE	fTwo_Keys_Off;		/* boolean 0 = false, 1 = true */
	BYTE	fDialog_Stickeys_Off;	/* boolean 0 = false, 1 = true */
} KBDSTICKEYSPARM;

extern	void FAR PASCAL Get_SticKeys_Param(KBDSTICKEYSPARM FAR *);	/* part of keyboard driver */
extern	void FAR PASCAL Set_SticKeys_Param(KBDSTICKEYSPARM FAR *);	/* part of keyboard driver */

typedef	struct tagKBDMOUSEKEYSPARM {
	BYTE	fMouseKeysOn;		/* boolean 0 = false, 1 = true */
	BYTE	fOn_Off_Feedback;	/* boolean 0 = false, 1 = true */
	int		Max_Speed;			/* in pixels per second        */
	int		Time_To_Max_Speed;	/* in 1/100th of a second      */
	BYTE	Accel_Table_Len;
	BYTE	Accel_Table[128];
	BYTE	Constant_Table_Len;
	BYTE	Constant_Table[128];
} KBDMOUSEKEYSPARM;

extern	void FAR PASCAL Get_MouseKeys_Param(KBDMOUSEKEYSPARM FAR *);	/* part of keyboard driver */
extern	void FAR PASCAL Set_MouseKeys_Param(KBDMOUSEKEYSPARM FAR *);	/* part of keyboard driver */

typedef	struct tagKBDTOGGLEKEYSPARM {
	BYTE	fToggleKeysOn;		/* boolean 0 = false, 1 = true */
	BYTE	fOn_Off_Feedback;	/* boolean 0 = false, 1 = true */
} KBDTOGGLEKEYSPARM;

extern	void FAR PASCAL Get_ToggleKeys_Param(KBDTOGGLEKEYSPARM FAR *);	/* part of keyboard driver */
extern	void FAR PASCAL Set_ToggleKeys_Param(KBDTOGGLEKEYSPARM FAR *);	/* part of keyboard driver */

typedef	struct tagKBDTIMEOUTPARM {
	BYTE	fTimeOutOn;			/* boolean 0 = false, 1 = true */
	BYTE	fOn_Off_Feedback;	/* boolean 0 = false, 1 = true */
	int		to_value;			/* time to turn off 18.2 times/sec */
} KBDTIMEOUTPARM;

extern	void FAR PASCAL Get_TimeOut_Param(KBDTIMEOUTPARM FAR *);	/* part of keyboard driver */
extern	void FAR PASCAL Set_TimeOut_Param(KBDTIMEOUTPARM FAR *);	/* part of keyboard driver */

typedef	struct tagKBDSHOWSOUNDSPARM {
	BYTE	fshow_sound_screen;	/* boolean 0 = false, 1 = true */
	BYTE	fshow_sound_caption;	/* boolean 0 = false, 1 = true */
	BYTE	fvideo_found;
	BYTE	fvideo_flash;
} KBDSHOWSOUNDSPARM;

extern	void FAR PASCAL Get_ShowSounds_Param(KBDSHOWSOUNDSPARM FAR *);	/* part of keyboard driver */
extern	void FAR PASCAL Set_ShowSounds_Param(KBDSHOWSOUNDSPARM FAR *);	/* part of keyboard driver */

typedef  struct tagMouseKeysParam {
	int		NumButtons;		/* holds number of buttons on the mice	*/
	int		Delta_Y;		/* Relative Y motion sign extended		*/
	int		Delta_X;		/* Relative X motion sign extended		*/
	int		Status;			/* status of mouse buttons and motion	*/
} MOUSEKEYSPARAM;

//extern	void FAR PASCAL InjectMouse(MOUSEKEYSPARAM FAR *);	/* part of mouse driver */
//extern	void FAR PASCAL InjectKeys(int);	/* part of keyboard driver */
//extern	void FAR PASCAL ErrorCode(int); 	/* part of keyboard driver */

typedef  struct tagKBDINFOPARAM {
	int		kybdversion;		/* holds handicap keyboard version number */
} KBDINFOPARM;

typedef  struct tagMOUINFOPARAM {
	int		mouversion;	    	/* holds handicap mouse version number */
} MOUINFOPARM;


/*;BCK*/
//extern	void FAR PASCAL Get_KybdInfo_Param(KBDINFOPARM FAR *);	/* part of appcalls in keyboard driver */
//extern	void FAR PASCAL Get_MouInfo_Param(MOUINFOPARM FAR *);	/* part of appcalls in keyboard driver */

extern	void FAR PASCAL Save_SerialKeys_Param(int); 	/* part of appcalls.asm */
