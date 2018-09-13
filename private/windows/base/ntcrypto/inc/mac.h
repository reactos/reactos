// Message Authentication Code structures

typedef struct _MACstate {
	DWORD		dwBufLen;
	HCRYPTKEY	hKey;
	BYTE		Feedback[CRYPT_BLKLEN];
	BYTE		Buffer[CRYPT_BLKLEN];
        BOOL            FinishFlag;
} MACstate;
