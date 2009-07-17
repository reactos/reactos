1001 pascal -ret16 WinGCreateDC() WinGCreateDC16
1002 pascal -ret16 WinGRecommendDIBFormat(ptr) WinGRecommendDIBFormat16
1003 pascal -ret16 WinGCreateBitmap(word ptr ptr) WinGCreateBitmap16
1004 pascal WinGGetDIBPointer(word ptr) WinGGetDIBPointer16
1005 pascal -ret16 WinGGetDIBColorTable(word word word ptr) WinGGetDIBColorTable16
1006 pascal -ret16 WinGSetDIBColorTable(word word word ptr) WinGSetDIBColorTable16
1007 pascal -ret16 WinGCreateHalfTonePalette() WinGCreateHalfTonePalette16
1008 pascal -ret16 WinGCreateHalfToneBrush(word word word) WinGCreateHalfToneBrush16
1009 pascal -ret16 WinGStretchBlt(word word word word word word word word word word) WinGStretchBlt16
1010 pascal -ret16 WinGBitBlt(word word word word word word word word) WinGBitBlt16

# Seem that 1299 is the limit... weird...
#1500 stub WINGINITIALIZETHUNK16
#1501 stub WINGTHUNK16

#2000 stub REGISTERWINGPAL
#2001 stub EXCEPTIONHANDLER
