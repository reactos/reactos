#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

HRESULT FAR PASCAL InitOle(BOOL fForceLoad);
void FAR PASCAL TermOle(void);

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */
