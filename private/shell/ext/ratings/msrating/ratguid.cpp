#include "msrating.h"

// declaring the GUIDs inline avoids having to use INITGUID
// avoiding unneeded GUIDs being pulled in. these will eventually
// need to go in a public header/lib so other people can implement
// these interfaces and use our COM object

// 20EDB660-7CDD-11CF-8DAB-00AA006C1A01
const GUID CLSID_RemoteSite = {0x20EDB660L, 0x7CDD, 0x11CF, 0x8D, 0xAB, 0x00, 0xAA, 0x00, 0x6C, 0x1A, 0x01};
// 19427BA0-826C-11CF-8DAB-00AA006C1A01
const GUID IID_IObtainRating = {0x19427BA0L, 0x826C, 0x11CF, 0x8D, 0xAB, 0x00, 0xAA, 0x00, 0x6C, 0x1A, 0x01};
