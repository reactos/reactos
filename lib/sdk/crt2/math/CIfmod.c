// @implemented
double _CDECL _CIfmod(void)
{
	FPU_DOUBLES(x, y);
	return fmod(x, y);
}
