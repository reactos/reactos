
#ifndef __MPEGTYPE__
#define __MPEGTYPE__

#ifdef __cplusplus
extern "C" {
#endif 



typedef struct tagAM_MPEGSTREAMTYPE
{
  DWORD dwStreamId;
  DWORD dwReserved;
  AM_MEDIA_TYPE mt;
  BYTE bFormat[1];
} AM_MPEGSTREAMTYPE;

typedef struct tagAM_MPEGSYSTEMTYPE
{
  DWORD dwBitRate;
  DWORD cStreams;
  AM_MPEGSTREAMTYPE Streams[1];
} AM_MPEGSYSTEMTYPE;

DECLARE_INTERFACE_(IMpegAudioDecoder, IUnknown)
{
  STDMETHOD(get_FrequencyDivider) (THIS_ unsigned long *pDivider) PURE;
  STDMETHOD(put_FrequencyDivider) (THIS_ unsigned long Divider) PURE;
  STDMETHOD(get_DecoderAccuracy) (THIS_ unsigned long *pAccuracy) PURE;
  STDMETHOD(put_DecoderAccuracy) (THIS_ unsigned long Accuracy) PURE;
  STDMETHOD(get_Stereo) (THIS_ unsigned long *pStereo ) PURE;
  STDMETHOD(put_Stereo) (THIS_ unsigned long Stereo) PURE;
  STDMETHOD(get_DecoderWordSize) (THIS_ unsigned long *pWordSize) PURE;
  STDMETHOD(put_DecoderWordSize) (THIS_ unsigned long WordSize) PURE;
  STDMETHOD(get_IntegerDecode) (THIS_ unsigned long *pIntDecode) PURE;
  STDMETHOD(put_IntegerDecode) (THIS_ unsigned long IntDecode) PURE;
  STDMETHOD(get_DualMode) (THIS_ unsigned long *pIntDecode) PURE;
  STDMETHOD(put_DualMode) (THIS_ unsigned long IntDecode) PURE;
  STDMETHOD(get_AudioFormat) (THIS_ MPEG1WAVEFORMAT *lpFmt) PURE;
};

#ifdef
}
#endif
#endif

#define AM_MPEGSTREAMTYPE_ELEMENTLENGTH(pStreamType)  FIELD_OFFSET(AM_MPEGSTREAMTYPE, bFormat[(pStreamType)->mt.cbFormat])
#define AM_MPEGSTREAMTYPE_NEXT(pStreamType)           ((AM_MPEGSTREAMTYPE *)((PBYTE)(pStreamType) + ((AM_MPEGSTREAMTYPE_ELEMENTLENGTH(pStreamType) + 7) & ~7)))
#define AM_MPEG_AUDIO_DUAL_MERGE 0
#define AM_MPEG_AUDIO_DUAL_LEFT  1
#define AM_MPEG_AUDIO_DUAL_RIGHT 2

