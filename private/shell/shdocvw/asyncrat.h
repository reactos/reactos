
class CDocObjectHost;

struct PicsQuery {
	DWORD dwSerial;
	HWND hwnd;
	LPVOID lpvRatingDetails;
};

extern DWORD _AddPicsQuery(HWND hwnd);
extern void _RemovePicsQuery(DWORD dwSerial);
extern BOOL _GetPicsQuery(DWORD dwSerial, PicsQuery *pOut);
extern void _RefPicsQueries(void);
extern void _ReleasePicsQueries(void);
extern BOOL _PostPicsMessage(DWORD dwSerial, HRESULT hr, LPVOID lpvRatingDetails);

#define WM_PICS_ASYNCCOMPLETE           (WM_USER + 0x0501)
#define WM_PICS_ROOTDOWNLOADCOMPLETE    (WM_USER + 0x0502)
#define WM_PICS_ALLCHECKSCOMPLETE       (WM_USER + 0x0503)
#define WM_PICS_DOBLOCKINGUI            (WM_USER + 0x0504)
