@ stdcall -stub CDBuildIntegrityVect(long long)
@ stdcall -stub CDBuildVect(long long)
@ stdcall -stub CDFindCommonCSystem(long long long)
@ stdcall -stub CDFindCommonCSystemWithKey(long long long long long)
@ stdcall -stub CDGenerateRandomBits(long long)
@ stdcall -stub CDLocateCSystem(long long)
@ stdcall -stub CDLocateCheckSum(long long)
@ stdcall -stub CDLocateRng(long long)
@ stdcall -stub CDRegisterCSystem(long)
@ stdcall -stub CDRegisterCheckSum(long)
@ stdcall -stub CDRegisterRng(long)
@ stdcall MD5Final(ptr) advapi32.MD5Final
@ stdcall MD5Init(ptr) advapi32.MD5Init
@ stdcall MD5Update(ptr ptr long) advapi32.MD5Update
