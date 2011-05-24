@ stdcall CDBuildIntegrityVect(long long)
@ stdcall CDBuildVect(long long)
@ stdcall CDFindCommonCSystem(long long long)
@ stdcall CDFindCommonCSystemWithKey(long long long long long)
@ stdcall CDGenerateRandomBits(long long)
@ stdcall CDLocateCSystem(long long)
@ stdcall CDLocateCheckSum(long long)
@ stdcall CDLocateRng(long long)
@ stdcall CDRegisterCSystem(long)
@ stdcall CDRegisterCheckSum(long)
@ stdcall CDRegisterRng(long)
@ stdcall MD5Final(ptr) advapi32.MD5Final
@ stdcall MD5Init(ptr) advapi32.MD5Init
@ stdcall MD5Update(ptr ptr long) advapi32.MD5Update
