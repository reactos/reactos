// Are there such things as UNICODE MIME64 files?  That would be very inefficient
#define LPMSTR LPSTR
#define LPCMSTR LPCSTR
#define MCHAR char

HRESULT Mime64Decode(LPCMSTR pStrData, LPSTREAM *ppstm);
HRESULT Mime64Encode(LPBYTE pData, UINT cbData, LPSTREAM *ppstm);
