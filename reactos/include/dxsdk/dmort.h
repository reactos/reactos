
#ifndef __DMORT_H__
#define __DMORT_H__

STDAPI MoCopyMediaType(DMO_MEDIA_TYPE *pmtDest, const DMO_MEDIA_TYPE *pmtSrc);
STDAPI MoCreateMediaType(DMO_MEDIA_TYPE **ppmt, DWORD cbFormat);
STDAPI MoDeleteMediaType(DMO_MEDIA_TYPE *pmt);
STDAPI MoDuplicateMediaType(DMO_MEDIA_TYPE **ppmtDest, const DMO_MEDIA_TYPE *pmtSrc);
STDAPI MoFreeMediaType(DMO_MEDIA_TYPE *pmt);
STDAPI MoInitMediaType(DMO_MEDIA_TYPE *pmt, DWORD cbFormat);

#endif

