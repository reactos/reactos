#include <windows.h>
#include <stdio.h>

#define BUFSIZE 128
#define MAX_PORTNAME_LEN 20
#define APP_VERSION_STR "0.01"

int main(int argc, char *argv[])
{
    CHAR txBuffer[BUFSIZE];
    CHAR rxBuffer[BUFSIZE];
    DWORD dwBaud = 9600;
    DWORD dwNumWritten;
    DWORD dwNumRead;
    DWORD dwErrors;
    DCB dcb;
    BOOL bResult;
    HANDLE hPort;
    int i;
    int j;
    int k;
	int nPortNum = 1;

	TCHAR szPortName[MAX_PORTNAME_LEN];

    if (argc > 1) {
        //sscanf(argv[1], "%d", &dwBaud);
        sscanf(argv[1], "%d", &nPortNum);
    }
	sprintf(szPortName, _T("COM%d"), nPortNum);

    printf("Serial Port Test Application Version %s\n", APP_VERSION_STR);
    printf("Attempting to open serial port %d - %s\n", nPortNum, szPortName);
    hPort = CreateFile(szPortName,
                       GENERIC_READ|GENERIC_WRITE,
                       0,     // exclusive
                       NULL,  // sec attr
                       OPEN_EXISTING,
                       0,     // no attributes
                       NULL); // no template

    if (hPort == (HANDLE)-1) {
        printf("ERROR: CreateFile() failed with result: %lx\n", hPort);
        return 1;
    }
    printf("CreateFile() returned: %lx\n", hPort);

    printf("Fiddling with DTR and RTS control lines...\n");
	for (i = 0; i < 100; i++) {
	bResult = EscapeCommFunction(hPort, SETDTR);
    if (!bResult) {
        printf("WARNING: EscapeCommFunction(SETDTR) failed: %lx\n", bResult);
    }
	bResult = EscapeCommFunction(hPort, SETRTS);
    if (!bResult) {
        printf("WARNING: EscapeCommFunction(SETRTS) failed: %lx\n", bResult);
    }
	for (j = 0; j < 1000; j++) {
		k *= j;
	}
/*
#define CLRDTR	(6)
#define CLRRTS	(4)
#define SETDTR	(5)
#define SETRTS	(3)
#define SETXOFF	(1)
#define SETXON	(2)
#define SETBREAK	(8)
#define CLRBREAK	(9)
 */
	bResult = EscapeCommFunction(hPort, CLRDTR);
    if (!bResult) {
        printf("WARNING: EscapeCommFunction(CLRDTR) failed: %lx\n", bResult);
    }
	bResult = EscapeCommFunction(hPort, CLRRTS);
    if (!bResult) {
        printf("WARNING: EscapeCommFunction(CLRRTS) failed: %lx\n", bResult);
    }
	}
    printf("Getting the default line characteristics...\n");
	dcb.DCBlength = sizeof(DCB);
	if (!GetCommState(hPort, &dcb)) {
        printf("ERROR: failed to get the dcb: %d\n", GetLastError());
        return 2;
    }
    printf("Setting the line characteristics to 9600,8,N,1\n");
    dcb.BaudRate = dwBaud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    bResult = SetCommState(hPort, &dcb);
    if (!bResult) {
        printf("ERROR: failed to set the comm state: %lx\n", bResult);
        return 3;
    }
	for (i = 0; i < BUFSIZE; i++) {
        txBuffer[i] = (CHAR)i;
        //printf(" %d ", txBuffer[i]);
        rxBuffer[i] = 0xFF;
    }
    printf("\n");
    printf("Writting transmit buffer to the serial port\n");
    bResult = WriteFile(hPort, txBuffer, BUFSIZE, &dwNumWritten, NULL);
    if (!bResult) {
        printf("ERROR: failed to write to the serial port: %lx\n", bResult);
        return 4;
    }
    printf("WriteFile() returned: %lx, byteswritten: %lx\n", bResult, dwNumWritten);
#if 0
	printf("Attempting to read %d bytes from the serial port\n", BUFSIZE);
    bResult = ReadFile(hPort, rxBuffer, BUFSIZE, &dwNumRead, NULL);
	if (!bResult) {
        printf("ERROR: failed to read from the serial port: %lx\n", bResult);
        return 5;
    }
    printf("ReadFile() returned: %lx, bytesread: %lx\n", bResult, dwNumRead);
    for (i = 0; i < BUFSIZE; i++) {
        printf(" %d ",rxBuffer[i]);
    }
#endif
    printf("Attempting to close the serial port\n");
    bResult = ClearCommError(hPort, &dwErrors, NULL);
    printf("ClearCommError returned: %lx, dwErrors: %lx\n", bResult, dwErrors);
    bResult = CloseHandle(hPort);
    if (!bResult) {
        printf("ERROR: failed to close the serial port: %lx\n", bResult);
        return 6;
    }
    printf("Finished\n");
    return 0;
}
