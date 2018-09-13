// File Error Identifiers
//
// When adding error ids here, be sure to add an entry in the mpidserr
// table in mytlab.cpp to indicate what string to display to the user!

#define ferrFirst           1000

#define ferrIllformedGroup      1000
#define ferrReadFailed          1001
#define ferrIllformedFile       1002
#define ferrCantProcNewExeHdr   1003
#define ferrCantProcOldExeHdr   1004
#define ferrBadMagicNewExe      1005
#define ferrBadMagicOldExe      1006
#define ferrNotWindowsExe       1007
#define ferrExeWinVer3          1008
#define ferrNotValidRc          1009
#define ferrNotValidExe         1010
#define ferrNotValidRes         1011
#define ferrNotValidBmp         1012
#define ferrNotValidIco         1013
#define ferrNotValidCur         1014
#define ferrRcInvalidExt        1015
#define ferrFileAlreadyOpen     1016
#define ferrExeTooLarge         1017
#define ferrCantCopyOldToNew    1018
#define ferrReadLoad            1019
#define ferrExeAlloc            1020
#define ferrExeInUse            1021
#define ferrExeEmpty            1022
#define ferrGroup               1023
#define ferrResSave             1024
#define ferrSaveOverOpen        1025
#define ferrSaveOverReadOnly    1026
#define ferrCantDetermineType   1027
#define ferrSameName            1028
#define ferrSaveAborted         1029
#define ferrLooksLikeNtRes      1030
#define ferrCantSaveReadOnly    1031
