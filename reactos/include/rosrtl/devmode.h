/* devmode.h
 */

#ifdef __cplusplus
extern "C"
{
#endif

void
RosRtlDevModeA2W ( LPDEVMODEW pW, const LPDEVMODEA pA );
void
RosRtlDevModeW2A( LPDEVMODEA pA, const LPDEVMODEW pW );

#ifdef __cplusplus
}
#endif

/* EOF */
