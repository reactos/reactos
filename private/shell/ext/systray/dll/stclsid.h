// CLSIDs for my objects

// SysTray - This object creates normally and supports IOleCommandTarget to launch
// the systray thread.
// {35CEC8A3-2BE6-11d2-8773-92E220524153}
DEFINE_GUID(CLSID_SysTray, 
0x35cec8a3, 0x2be6, 0x11d2, 0x87, 0x73, 0x92, 0xe2, 0x20, 0x52, 0x41, 0x53);

// SysTrayInvoker - This guy automatically launches the systray thread as
// soon as he's created; works good with SHLoadInProc
// {730F6CDC-2C86-11d2-8773-92E220524153}
DEFINE_GUID(CLSID_SysTrayInvoker, 
0x730f6cdc, 0x2c86, 0x11d2, 0x87, 0x73, 0x92, 0xe2, 0x20, 0x52, 0x41, 0x53);
