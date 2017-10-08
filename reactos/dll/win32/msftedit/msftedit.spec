2 extern IID_IRichEditOle
3 extern IID_IRichEditOleCallback
4 stdcall CreateTextServices(ptr ptr ptr) riched20.CreateTextServices
5 extern IID_ITextServices riched20.IID_ITextServices
6 extern IID_ITextHost riched20.IID_ITextHost
7 extern IID_ITextHost2 riched20.IID_ITextHost2
8 stdcall REExtendedRegisterClass() riched20.REExtendedRegisterClass
9 stdcall RichEdit10ANSIWndProc(ptr long long long) riched20.RichEdit10ANSIWndProc
10 stdcall RichEditANSIWndProc(ptr long long long) riched20.RichEditANSIWndProc
11 stub SetCustomTextOutHandlerEx
12 stdcall -private DllGetVersion(ptr)
13 stub RichEditWndProc
14 stub RichListBoxWndProc
15 stub RichComboBoxWndProc
