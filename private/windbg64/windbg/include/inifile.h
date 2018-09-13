/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
//Loads ini file from disk into memory
int FAR PASCAL LoadIniFile(
    void);

//Saves ini file to disk
void FAR PASCAL SaveIniFile(void );

BOOL FAR PASCAL ReadStrFromFile(int hFile, LPSTR lpszStr, int wMaxSize, BOOL iniFile);

BOOL FAR PASCAL WriteStrToFile(
    int hFile,
    LPSTR lpszStr,
    BOOL iniFile);
