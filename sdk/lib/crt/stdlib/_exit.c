/* taken from wine exit.c */
#include <precomp.h>

/*
 * @implemented
 */
void _cexit( void )
{
  LOCK_EXIT;
  __call_atexit();
  UNLOCK_EXIT;
}

/*
 * @implemented
 */
void _c_exit( void )
{
  /* All cleanup is done on DLL detach; Return to caller */
}

/*
 * @implemented
 */
void _exit(int exitcode)
{
    ExitProcess(exitcode);
}

/*
 * @implemented
 */
void exit(int exitcode)
{
#if 0
  HMODULE hmscoree;
  static const WCHAR mscoreeW[] = {'m','s','c','o','r','e','e',0};
  void (WINAPI *pCorExitProcess)(int);
#endif
  WARN("exit(%d) called\n",exitcode);
  _cexit();
#if 0
  hmscoree = GetModuleHandleW(mscoreeW);

  if (hmscoree)
  {
    pCorExitProcess = (void*)GetProcAddress(hmscoree, "CorExitProcess");

    if (pCorExitProcess)
      pCorExitProcess(exitcode);
  }
#endif
  ExitProcess(exitcode);

}
