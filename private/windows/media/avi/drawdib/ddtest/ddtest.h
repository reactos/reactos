/* 'About' dialog box resource id */
#define ABOUTBOX    1

#define ID_APP      1000

/* Menu Items */
#define MENU_ABOUT          2
#define MENU_EXIT           4
#define MENU_OPEN           11
#define MENU_SAVE           12

#define MENU_COPY           20
#define MENU_PASTE          21

#define MENU_DIB            100
#define MENU_DIB_4          104
#define MENU_DIB_8          108
#define MENU_DIB_16         116
#define MENU_DIB_24         124

#define MENU_STRETCH_1      300
#define MENU_STRETCH_15     301
#define MENU_STRETCH_2      302
#define MENU_STRETCH_WIN    303

#define MENU_STRETCH_HUH    343
#define MENU_JUST_DRAW_IT   350
#define MENU_FULLSCREEN     351

#define MENU_TEST_DRAWDIB       401
#define MENU_TEST_DISPLAY       402
#define MENU_TEST_COMPRESS      403
#define MENU_TEST_DECOMPRESS    404
#define MENU_TEST_ESCAPE        405
#define MENU_TIME_DRAWDIB       406

#define MENU_TEST_ALL           499
#define MENU_COMPRESS_IT        410

#define MENU_COMPRESS           500

#ifdef DEBUG
    #define static
    extern void FAR CDECL dprintf(LPSTR, ...);
    #define DPF( x ) dprintf x
#else
    #define DPF(X)
#endif
