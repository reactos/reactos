
//
// cmdlink.c
//

VOID CmlActivate(LPCML lpcml);
LPCML CmlClone(LPCML lpcml);
LPCML CmlCreateWorker(LPSTR lpstrCmd, BOOL fCmdIsLink, BOOL fFilename);

#define CmlCreate(lpstrCmd, fCmdIsLink)     \
        CmlCreateWorker(lpstrCmd, fCmdIsLink, FALSE)

#define CmlCreateFromFilename(lpstrCmd, fCmdIsLink) \
        CmlCreateWorker(lpstrCmd, fCmdIsLink, TRUE)

VOID CmlDelete(LPCML lpcml);
VOID CmlDraw(LPCML lpcml, HDC hdc, LPRECT lprc, INT xHSB, BOOL fFocus);
VOID CmlFixBounds(LPCML lpcml);
LPCML CmlReadFromNative(LPSTR *lplpstr);
DWORD CmlWriteToNative(LPCML lpcml, LPSTR *lplpstr);


//
// dlgprocs.c
//

INT_PTR MyDialogBox(UINT idd, HWND hwndParent, DLGPROC lpfnDlgProc);
BOOL IconDialog(LPIC lpic);
BOOL ChangeCmdLine(LPCML lpcml);
VOID ChangeLabel(LPIC lpic);
INT_PTR CALLBACK fnChangeCmdText(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK fnProperties(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK fnChangeText(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK fnInvalidLink(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);


//
// embed.c
//

BOOL EmbActivate(LPEMBED lpembed, UINT wVerb);
LPEMBED EmbCreate(LPSTR lpstrFile);
VOID EmbDelete(LPEMBED lpembed);
VOID EmbDraw(LPEMBED lpembed, HDC hdc, LPRECT lprc, BOOL fFocus);
LPEMBED EmbReadFromNative(LPSTR *lplpstr);
DWORD EmbWriteToNative(LPEMBED lpembed, LPSTR *lplpstr);
VOID EmbWriteToFile(LPEMBED lpembed, INT fh);
VOID EmbRead(LPEMBED lpembed);
BOOL EmbDoVerb(LPEMBED lpembed, UINT wVerb);
BOOL EmbActivateThroughOle(LPEMBED lpembed, LPSTR lpdocname, UINT wVerb);
INT CALLBACK EmbCallBack(LPOLECLIENT lpclient, OLE_NOTIFICATION flags,
    LPOLEOBJECT lpObject);
VOID EmbDeleteLinkObject(LPEMBED lpembed);


//
// filedlgs.c
//

VOID OfnInit(VOID);
BOOL OfnGetName(HWND hwnd, UINT msg);
HANDLE OfnGetNewLinkName(HWND hwnd, HANDLE hData);
VOID Normalize(LPSTR lpstrFile);


//
// icon.c
//

LPIC IconClone(LPIC lpic);
LPIC IconCreateFromFile(LPSTR lpstrFile);
LPIC IconCreateFromObject(LPOLEOBJECT lpObject);
VOID IconDelete(LPIC lpic);
VOID IconDraw(LPIC lpic, HDC hdc, LPRECT lprc, BOOL fFocus, INT cxImage,
    INT cyImage);
LPIC IconReadFromNative(LPSTR *lplpstr);
DWORD IconWriteToNative(LPIC lpic, LPSTR *lplpstr);
VOID GetCurrentIcon(LPIC lpic);


//
// packager.c
//

LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID SetTitle(BOOL fRegistering);
VOID InitFile(VOID);
VOID ErrorMessage(UINT id);
BOOL ProcessMessage(VOID);
LPSTR Contains(LPSTR lpString, LPSTR lpPattern);
VOID Dirty(VOID);
VOID DeregisterDoc(VOID);
VOID Raise(INT iPane);
INT_PTR MessageBoxAfterBlock(HWND hwndParent, LPSTR lpText, LPSTR lpCaption,
    UINT fuStyle);
INT_PTR DialogBoxAfterBlock(LPCSTR lpTemplate, HWND hwndParent,
    DLGPROC lpDialogFunc);


//
// pane.c
//

BOOL InitPaneClasses(VOID);
BOOL InitPanes(VOID);
VOID EndPanes(VOID);
LRESULT CALLBACK SubtitleWndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
LRESULT CALLBACK PaneWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SplitterFrame(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
VOID DeletePane(INT iPane, BOOL fDeleteUndo);
VOID DeletePaneObject(LPVOID lpobj, INT objType);


//
// picture.c
//

BOOL InitClient(VOID);
VOID EndClient(VOID);
LPPICT PicCreate(LPOLEOBJECT lpObject, LPRECT lprcObject);
VOID PicDelete(LPPICT lppict);
BOOL PicDraw(LPPICT lppict, HDC hDC, LPRECT lprc, INT xHSB, INT yVSB,
    BOOL fPicture, BOOL fFocus);
LPPICT PicPaste(BOOL fPaste, LPSTR lpstrName);
BOOL Error(OLESTATUS olestat);
INT CALLBACK CallBack(LPOLECLIENT lpclient, OLE_NOTIFICATION flags,
    LPOLEOBJECT lpObject);
VOID WaitForObject(LPOLEOBJECT lpObject);
BOOL PicSetUpdateOptions(LPPICT lppict, UINT idCmd);
LPPICT PicReadFromNative(LPSTR *lplpstr, LPSTR lpstrName);
DWORD PicWriteToNative(LPPICT lppict, LPOLEOBJECT lpObject, LPSTR *lplpstr);
VOID Hourglass(BOOL fOn);
VOID PicActivate(LPPICT lppict, UINT idCmd);
VOID PicUpdate(LPPICT lppict);
VOID PicFreeze(LPPICT lppict);
VOID PicChangeLink(LPPICT lppict);
BOOL PicCopy(LPPICT lppict);
VOID PicSaveUndo(LPPICT lppict);
LPPICT PicFromFile(BOOL fEmbedded, LPSTR szFile);

typedef int (__stdcall *PCALL_BACK)(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);
LPOLECLIENT PicCreateClient(PCALL_BACK fnCallBack, LPOLECLIENTVTBL lpclivtbl);


//
// register.c
//

VOID RegInit(VOID);
VOID RegGetClassId(LPSTR lpstrName, LPSTR lpstrClass);
INT RegMakeFilterSpec(LPSTR lpstrClass, LPSTR lpstrExt, LPSTR lpstrFilterSpec);
VOID RegGetExeName(LPSTR lpstrExe, LPSTR lpstrClass, DWORD dwBytes);


//
// server.c
//

BOOL InitServer(VOID);
VOID DeleteServer(LPSAMPSRVR lpsrvr);
VOID DestroyServer(VOID);
LPSAMPDOC InitDoc(LPSAMPSRVR lpsrvr, LHSERVERDOC lhdoc, LPSTR lptitle);
VOID ChangeDocName(LPSAMPDOC *lplpdoc, LPSTR lpname);
BOOL SendDocChangeMsg(LPSAMPDOC lpdoc, UINT options);
LPSAMPDOC CreateNewDoc(LPSAMPSRVR lpsrvr, LHSERVERDOC lhdoc, LPSTR lpstr);
LPSAMPDOC CreateDocFromFile(LPSAMPSRVR lpsrvr, LHSERVERDOC lhdoc, LPSTR lpstr);
BOOL CopyObjects(VOID);
LPSAMPITEM CreateNewItem(LPSAMPDOC lpdoc);
HANDLE GetNative(BOOL fClip);
BOOL PutNative(HANDLE hdata);
HANDLE GetLink(VOID);
HANDLE GetMF(VOID);
VOID InitEmbedded(BOOL fCreate);
LPSAMPITEM AddItem(LPSAMPITEM lpitem);
BOOL DeleteItem(LPSAMPITEM lpitem);
VOID EndEmbedding(VOID);


//
// stream.c
//

VOID SetFile(STREAMOP sop, INT fh, LPSTR *lplpstr);
DWORD ReadStream(LPAPPSTREAM lpStream, LPSTR lpstr, DWORD cb);
DWORD PosStream(LPAPPSTREAM lpStream, LONG pos, INT iorigin);
DWORD WriteStream(LPAPPSTREAM lpStream, LPSTR lpstr, DWORD cb);
DWORD MemRead(LPSTR *lplpStream, LPSTR lpItem, DWORD dwSize);
DWORD MemWrite(LPSTR *lplpStream, LPSTR lpItem, DWORD dwSize);


//
// virtable.c
//

OLESTATUS SrvrOpen(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc, LPSTR lpdocname,
    LPOLESERVERDOC *lplpoledoc);
OLESTATUS SrvrCreate(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
    LPSTR lpclassname, LPSTR lpdocname, LPOLESERVERDOC *lplpoledoc);
OLESTATUS SrvrCreateFromTemplate(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
    LPSTR lpclassname, LPSTR lpdocname, LPSTR lptemplatename,
    LPOLESERVERDOC *lplpoledoc);
OLESTATUS SrvrEdit(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc, LPSTR lpclassname,
    LPSTR lpdocname, LPOLESERVERDOC *lplpoledoc);
OLESTATUS SrvrExit(LPOLESERVER lpolesrvr);
OLESTATUS SrvrRelease(LPOLESERVER lpolesrvr);
OLESTATUS SrvrExecute(LPOLESERVER lpolesrvr, HANDLE hCmds);

OLESTATUS DocSave(LPOLESERVERDOC lpoledoc);
OLESTATUS DocClose(LPOLESERVERDOC lpoledoc);
OLESTATUS DocRelease(LPOLESERVERDOC lpoledoc);
OLESTATUS DocGetObject(LPOLESERVERDOC lpoledoc, LPSTR lpitemname,
    LPOLEOBJECT *lplpoleobject, LPOLECLIENT lpoleclient);
OLESTATUS DocSetHostNames(LPOLESERVERDOC lpoledoc, LPSTR lpclientName,
    LPSTR lpdocName);
OLESTATUS DocSetDocDimensions(LPOLESERVERDOC lpoledoc, LPRECT lprc);
OLESTATUS DocSetColorScheme(LPOLESERVERDOC lpoledoc, LPLOGPALETTE lppal);
OLESTATUS DocExecute(LPOLESERVERDOC lpoledoc, HANDLE hCmds);

OLESTATUS ItemDelete(LPOLEOBJECT lpoleobject);
OLESTATUS ItemGetData(LPOLEOBJECT lpoleobject, OLECLIPFORMAT cfFormat,
    LPHANDLE lphandle);
OLESTATUS ItemSetData(LPOLEOBJECT lpoleobject, OLECLIPFORMAT cfFormat,
    HANDLE hdata);
OLESTATUS ItemDoVerb(LPOLEOBJECT lpoleobject, UINT wVerb, BOOL fShow,
    BOOL fActivate);
OLESTATUS ItemShow(LPOLEOBJECT lpoleobject, BOOL fActivate);
OLESTATUS ItemSetBounds(LPOLEOBJECT lpoleobject, LPRECT lprc);
OLESTATUS ItemSetTargetDevice(LPOLEOBJECT lpoleobject, HANDLE h);
OLECLIPFORMAT ItemEnumFormats(LPOLEOBJECT lpobject, OLECLIPFORMAT cfFormat);
LPVOID ItemQueryProtocol(LPOLEOBJECT lpoleobject, LPSTR lpprotocol);
OLESTATUS ItemSetColorScheme(LPOLEOBJECT lpoleobject, LPLOGPALETTE lppal);

BOOL IsOleServerDoc(LPSTR lpdocname);
