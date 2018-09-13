//*****************************************************************************
//
// 32bit Control message thunking
//
//
// 01-FEB-92 NanduriR Created
//*****************************************************************************


BOOL ThunkEMMsg32(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam,
                  PWORD pwMsgNew, PWORD pwParamNew, PLONG plParamNew);
BOOL ThunkCBMsg32(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam,
                  PWORD pwMsgNew, PWORD pwParamNew, PLONG plParamNew);
BOOL ThunkLBMsg32(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam,
                  PWORD pwMsgNew, PWORD pwParamNew, PLONG plParamNew);
