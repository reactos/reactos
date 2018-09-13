#define ID_APP      1000

/* Menu Items */
#define MENU_ABOUT          2
#define MENU_EXIT           4
#define MENU_BALL	    99
#define MENU_OPEN           11
#define	MENU_CLOSE	    10
#define MENU_MERGE	    17
#define MENU_SAVEAS         13
#define MENU_SAVERAW        14
#define MENU_OPTIONS	    15
#define MENU_UNDO	    18
#define MENU_CUT	    19
#define MENU_COPY           20
#define MENU_PASTE          21
#define MENU_DELETE	    22
#define MENU_MARK	    30
#define MENU_ZOOMQUARTER    54
#define MENU_ZOOMHALF	    50
#define MENU_ZOOM1	    51
#define MENU_ZOOM2	    52
#define MENU_ZOOM4	    53

#define MENU_PLAY	    100
#define MENU_PAUSE	    101
#define MENU_STOP	    102

#define MENU_PLAY_DECOMPRESS	    200
#define MENU_PLAY_CHEAT		    201
#define MENU_PLAY_YIELD_BOUND	    202
#define MENU_PLAY_READ_BOUND	    203
#define MENU_PLAY_DECOMPRESS_BOUND  204
#define MENU_PLAY_DRAW_BOUND	    205

#ifdef DEBUG
    extern void FAR CDECL dprintf(LPSTR, ...);
    #define DPF dprintf
#else
    #define DPF / ## /
#endif
