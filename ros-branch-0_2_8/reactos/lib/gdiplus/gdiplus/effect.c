#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
Status __stdcall
GdipCreateEffect(const GUID guid,
  CGpEffect **effect)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
Status __stdcall
GdipDeleteEffect(CGpEffect *effect)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
Status __stdcall
GdipGetEffectParameterSize(CGpEffect *effect,
  UINT *size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
Status __stdcall
GdipSetEffectParameters(CGpEffect *effect,
  const VOID *params,
  const UINT size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
Status __stdcall
GdipGetEffectParameters(CGpEffect *effect,
  UINT *size,
  VOID *params)
{
  return NotImplemented;
}
