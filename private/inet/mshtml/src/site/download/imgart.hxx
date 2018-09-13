#ifndef I_IMGART_HXX_
#define I_IMGART_HXX_
#pragma INCMSG("--- Beg 'imgart.hxx'")

#ifndef X_IMGBITS_HXX
#define X_IMGBITS_HXX
#include "imgbits.hxx"
#endif


MtExtern(CArtPlayer)

// CArtPlayer -----------------------------------------------------------------

class CArtPlayer {

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CArtPlayer))

    ~CArtPlayer();
    
    BOOL    QueryPlayState(int iCommand);
    void    DoPlayCommand(int iCommand);

    BOOL    GetArtReport(CImgBitsDIB **ppibd,
                LONG _yHei, LONG _colorMode);

    RECT        _rcUpdateRect;
    UINT        _uiUpdateRate;
    ULONG       _ulCurrentTime;
    ULONG       _ulAvailPlayTime;
    DWORD_PTR   _dwShowHandle;
    BOOL        _fTemporalART:1;
    BOOL        _fHasSound:1;
    BOOL        _fDynamicImages:1;
    BOOL        _fPlaying:1;
    BOOL        _fPaused:1;
    BOOL        _fRewind:1;
    BOOL        _fIsDone:1;
    BOOL        _fUpdateImage:1;
    BOOL        _fInPlayer:1;
};

#pragma INCMSG("--- End 'imgart.hxx'")
#else
#pragma INCMSG("*** Dup 'imgart.hxx'")
#endif
