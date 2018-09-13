#define ID_APP      1000

/* Menu Items */
#define MENU_ABOUT          2
#define MENU_EXIT           4
#define MENU_BALL	    99
#define MENU_OPEN           11
#define	MENU_NEW	    10
#define MENU_ADD	    17
#define MENU_SAVE           12
#define MENU_SAVEAS         13
#define MENU_SAVERAW        14
#define MENU_OPTIONS	    15
#define MENU_SAVEWAVE	    16
#define MENU_UNDO	    18
#define MENU_CUT	    19
#define MENU_COPY           20
#define MENU_PASTE          21
#define MENU_DELETE	    22

#define MENU_CSTREAM	    25
#define MENU_CFILE	    26

#define MENU_MARK	    30
#define MENU_ZOOMQUARTER    54
#define MENU_ZOOMHALF	    50
#define MENU_ZOOM1	    51
#define MENU_ZOOM2	    52
#define MENU_ZOOM4	    53

#define MENU_WAVEFORMAT	    96
#define MENU_OPENLYR	    97
#define MENU_SAVESMALL	    98

#define MENU_PLAY	    100
#define MENU_PAUSE	    101
#define MENU_STOP           102

#define MENU_PLAY_FILE      200
#define MENU_PLAY_STREAM    201

#ifdef DEBUG
    extern void FAR CDECL dprintf(LPSTR, ...);
    #define DPF dprintf
#else
    #define DPF / ## /
#endif
