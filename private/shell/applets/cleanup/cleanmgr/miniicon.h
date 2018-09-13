#ifndef MINIICON_H
#define MINIICON_H


/*
 * Includes ___________________________________________________________________
 *
 */
#ifndef COMMON_H
   #include "common.h"
#endif

#define MINIICON_CHECK_ON   0
#define MINIICON_CHECK_OFF  1

#define DMI_MASK            1
#define DMI_BKCOLOR         2
#define DMI_USERECT         4

#define MINIX 16
#define MINIY 16

#define RGB_WHITE (RGB(255, 255, 255))
#define RGB_BLACK (RGB(0, 0, 0))
#define RGB_TRANSPARENT (RGB(0, 128, 128))


/*
 * CLASSES ___________________________________________________________________
 *
 */
class MiniIcon
{
public:														
    MiniIcon(void);
    ~MiniIcon(void);
    
	static void Register(HINSTANCE hInstance);
	static void Unregister();
    
    int DrawMiniIcon(HDC hdc, RECT rc, INT MiniIconIndex, DWORD Flags);
    

protected:
    static      HINSTANCE   hInstance;


private:
    BOOL CreateMiniIcons(void);
    void DestroyMiniIcons(void);

    HDC         hdcMiniMem;
    HBITMAP     hbmMiniImage;
    HBITMAP     hbmMiniMask;
    UINT        NumImages;
    
};

typedef MiniIcon *pMiniIcon;


#endif

