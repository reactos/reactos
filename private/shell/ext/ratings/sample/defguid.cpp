#include "project.h"


// declaring the GUIDs inline avoids having to use INITGUID
// avoiding unneeded GUIDs being pulled in.

/* IMPORTANT: Run GUIDGEN and insert your own GUID for CLSID_SAMPLE */
/* ALSO insert the registry format equivalent for szOurGUID */
// {E9489DE0-B311-11cf-83B1-00C04FD705B2}
const GUID CLSID_Sample = 
{ 0xe9489de0, 0xb311, 0x11cf, { 0x83, 0xb1, 0x0, 0xc0, 0x4f, 0xd7, 0x5, 0xb2 } };
const char szOurGUID[] = "{E9489DE0-B311-11cf-83B1-00C04FD705B2}";

/* Interface ID for the IObtainRating interface. Leave this the way it is. */
// 19427BA0-826C-11CF-8DAB-00AA006C1A01
const GUID IID_IObtainRating = {0x19427BA0L, 0x826C, 0x11CF, 0x8D, 0xAB, 0x00, 0xAA, 0x00, 0x6C, 0x1A, 0x01};
