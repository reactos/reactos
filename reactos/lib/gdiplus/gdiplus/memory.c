#include <windows.h>
#include <GdiPlusPrivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
void* WINGDIPAPI
GdipAlloc(size_t size)
{
  return NULL;
}

/*
 * @unimplemented
 */
void WINGDIPAPI
GdipFree(void* ptr)
{
}
