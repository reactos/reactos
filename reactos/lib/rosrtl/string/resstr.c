#include <windows.h>

/* 
 * Utility to measure the length of a string resource
 *
 * IN HINSTANCE hInst -> Instance of the module
 * IN UINT uID        -> ID of the string to measure
 *
 * Returns the number of characters not including the null-terminator.
 * Returns -1 on failure.
 */
int
RosLenOfStrResource(HINSTANCE hInst, UINT uID)
{
  HRSRC hrSrc;
  HGLOBAL hRes;
  LPWSTR lpName, lpStr;
  
  if(hInst == NULL)
  {
    return -1;
  }
  
  /* There are always blocks of 16 strings */
  lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

  /* Find the string table block */
  if((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
     (hRes = LoadResource(hInst, hrSrc)) &&
     (lpStr = LockResource(hRes)))
  {
    UINT x;

    /* Find the string we're looking for */
    uID &= 0xF; /* position in the block, same as % 16 */
    for(x = 0; x < uID; x++)
    {
      lpStr += (*lpStr) + 1;
    }
    
    /* Found the string */
    return (int)(*lpStr);
  }
  return -1;
}

/*
 * Utility to allocate and load a string from the string table
 *
 * OUT LPSTR *lpTarget -> Address to a variable that will get the address to the string allocated
 * IN  HINSTANCE hInst -> Instance of the module
 * IN  UINT uID        -> ID of the string to measure
 *
 * Returns the number of characters not including the null-terminator.
 * Returns 0 on failure. Use LocalFree() to free the memory allocated.
 */
int
RosAllocAndLoadStringA(LPSTR *lpTarget, HINSTANCE hInst, UINT uID)
{
  int ln;
  
  ln = RosLenOfStrResource(hInst, uID);
  if(ln++ > 0)
  {
    (*lpTarget) = (LPSTR)LocalAlloc(LMEM_FIXED, ln * sizeof(CHAR));
    if((*lpTarget) != NULL)
    {
      int Ret;
      if(!(Ret = LoadStringA(hInst, uID, *lpTarget, ln)))
      {
        LocalFree((HLOCAL)(*lpTarget));
      }
      return Ret;
    }
  }
  return 0;
}

/*
 * Utility to allocate and load a string from the string table
 *
 * OUT LPSTR *lpTarget -> Address to a variable that will get the address to the string allocated
 * IN  HINSTANCE hInst -> Instance of the module
 * IN  UINT uID        -> ID of the string to measure
 *
 * Returns the number of characters not including the null-terminator.
 * Returns 0 on failure. Use LocalFree() to free the memory allocated.
 */
int
RosAllocAndLoadStringW(LPWSTR *lpTarget, HINSTANCE hInst, UINT uID)
{
  int ln;

  ln = RosLenOfStrResource(hInst, uID);
  if(ln++ > 0)
  {
    (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED, ln * sizeof(WCHAR));
    if((*lpTarget) != NULL)
    {
      int Ret;
      if(!(Ret = LoadStringW(hInst, uID, *lpTarget, ln)))
      {
        LocalFree((HLOCAL)(*lpTarget));
      }
      return Ret;
    }
  }
  return 0;
}

/*
 * Utility to allocate memory and format a string
 *
 * OUT LPSTR *lpTarget -> Address to a variable that will get the address to the string allocated
 * IN  LPSTR lpFormat  -> String which is to be formatted with the arguments given
 *
 * Returns the number of characters in lpTarget not including the null-terminator.
 * Returns 0 on failure. Use LocalFree() to free the memory allocated.
 */
DWORD
RosFormatStrA(LPSTR *lpTarget, LPSTR lpFormat, ...)
{
  DWORD Ret;
  va_list lArgs;
  
  va_start(lArgs, lpFormat);
  /* let's use FormatMessage to format it because it has the ability to allocate
     memory automatically */
  Ret = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                       lpFormat, 0, 0, (LPSTR)lpTarget, 0, &lArgs);
  va_end(lArgs);
  
  return Ret;
}

/*
 * Utility to allocate memory and format a string
 *
 * OUT LPSTR *lpTarget -> Address to a variable that will get the address to the string allocated
 * IN  LPSTR lpFormat  -> String which is to be formatted with the arguments given
 *
 * Returns the number of characters in lpTarget not including the null-terminator.
 * Returns 0 on failure. Use LocalFree() to free the memory allocated.
 */
DWORD
RosFormatStrW(LPWSTR *lpTarget, LPWSTR lpFormat, ...)
{
  DWORD Ret;
  va_list lArgs;

  va_start(lArgs, lpFormat);
  /* let's use FormatMessage to format it because it has the ability to allocate
     memory automatically */
  Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                       lpFormat, 0, 0, (LPWSTR)lpTarget, 0, &lArgs);
  va_end(lArgs);

  return Ret;
}

/*
 * Utility to allocate memory, load a string from the resources and format it
 *
 * IN  HINSTANCE hInst -> Instance of the module
 * IN  UINT uID        -> ID of the string to measure
 * OUT LPSTR *lpTarget -> Address to a variable that will get the address to the string allocated
 *
 * Returns the number of characters in lpTarget not including the null-terminator.
 * Returns 0 on failure. Use LocalFree() to free the memory allocated.
 */
DWORD
RosLoadAndFormatStrA(HINSTANCE hInst, UINT uID, LPSTR *lpTarget, ...)
{
  DWORD Ret = 0;
  LPSTR lpFormat;
  va_list lArgs;
  
  if(RosAllocAndLoadStringA(&lpFormat, hInst, uID) > 0)
  {
    va_start(lArgs, lpTarget);
    /* let's use FormatMessage to format it because it has the ability to allocate
       memory automatically */
    Ret = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                         lpFormat, 0, 0, (LPSTR)lpTarget, 0, &lArgs);
    va_end(lArgs);
    
    LocalFree((HLOCAL)lpFormat);
  }
  
  return Ret;
}

/*
 * Utility to allocate memory, load a string from the resources and format it
 *
 * IN  HINSTANCE hInst -> Instance of the module
 * IN  UINT uID        -> ID of the string to measure
 * OUT LPSTR *lpTarget -> Address to a variable that will get the address to the string allocated
 *
 * Returns the number of characters in lpTarget not including the null-terminator.
 * Returns 0 on failure. Use LocalFree() to free the memory allocated.
 */
DWORD
RosLoadAndFormatStrW(HINSTANCE hInst, UINT uID, LPWSTR *lpTarget, ...)
{
  DWORD Ret = 0;
  LPWSTR lpFormat;
  va_list lArgs;

  if(RosAllocAndLoadStringW(&lpFormat, hInst, uID) > 0)
  {
    va_start(lArgs, lpTarget);
    /* let's use FormatMessage to format it because it has the ability to allocate
       memory automatically */
    Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                         lpFormat, 0, 0, (LPWSTR)lpTarget, 0, &lArgs);
    va_end(lArgs);
    
    LocalFree((HLOCAL)lpFormat);
  }
  
  return Ret;
}

