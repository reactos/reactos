/* logfont.h
 */

#ifdef __cplusplus
extern "C"
{
#endif

void
RosRtlLogFontA2W ( LPLOGFONTW pW, const LPLOGFONTA pA );

void
RosRtlLogFontW2A ( LPLOGFONTA pA, const LPLOGFONTW pW );

#ifdef __cplusplus
}
#endif

/* EOF */
