/* logfont.h
 */

#ifdef __cplusplus
extern "C"
{
#endif

void
RosRtlLogFontA2W ( LPLOGFONTW pW, const LOGFONTA *pA );

void
RosRtlLogFontW2A ( LPLOGFONTA pA, const LOGFONTW *pW );

#ifdef __cplusplus
}
#endif

/* EOF */
