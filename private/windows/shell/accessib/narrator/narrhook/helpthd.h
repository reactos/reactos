/*************************************************************************

  Include file for helpthd.cpp

  defines STACAKBLE_EVENT_INFO structure and GINFO (Global Info) structure

*************************************************************************/
typedef struct STACKABLE_EVENT_INFO *PSTACKABLE_EVENT_INFO;

typedef struct STACKABLE_EVENT_INFO {

	enum Action {NewEvent, EndHelper};
	Action       m_Action;

	DWORD        event;
	HWND         hwndMsg;
	LONG         idObject;
	LONG         idChild;
	DWORD        idThread;
	DWORD        dwmsEventTime;

} STACKABLE_EVENT_INFO;


typedef struct GINFO {
    CRITICAL_SECTION HelperCritSect;
    HANDLE           hHelperEvent;
    HANDLE           hHelperThread;
    CList            EventInfoList;
} GINFO;


//
// Function Prototypes
//
void InitHelperThread();
void UnInitHelperThread();

void AddEventInfoToStack(DWORD event, HWND hwndMsg, LONG idObject, LONG idChild, DWORD idThread, DWORD dwmsEventTime);
BOOL RemoveInfoFromStack(STACKABLE_EVENT_INFO *pEventInfo);


