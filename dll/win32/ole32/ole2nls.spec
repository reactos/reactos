1 pascal GetUserDefaultLCID() GetUserDefaultLCID16
2 pascal GetSystemDefaultLCID() GetSystemDefaultLCID16
3 pascal -ret16 GetUserDefaultLangID() GetUserDefaultLangID16
4 pascal -ret16 GetSystemDefaultLangID() GetSystemDefaultLangID16
5 pascal GetLocaleInfoA(long long ptr word) GetLocaleInfo16
6 pascal -ret16 LCMapStringA(word long ptr word ptr word) LCMapString16
7 pascal -ret16 GetStringTypeA(long long str word ptr) GetStringType16
8 pascal -ret16 CompareStringA(long long str word str word) CompareString16
9 pascal -ret16 RegisterNLSInfoChanged(ptr) RegisterNLSInfoChanged16
#10 stub WEP
11 stub LIBMAIN
12 stub NOTIFYWINDOWPROC
