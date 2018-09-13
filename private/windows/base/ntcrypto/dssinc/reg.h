/* reg.h */

#ifdef __cplusplus
extern "C" {
#endif

/*********************************/
/* Definitions                   */
/*********************************/

/*********************************/
/* Structure Definitions         */
/*********************************/


/*********************************/
/* Function Definitions          */
/*********************************/

DWORD OpenUserKeyGroup(
                       Context_t *pContext,
                       LPSTR szUserName,
                       DWORD dwFlags
                       );

BOOL openKeyGroup(
                  IN OUT Context_t *pContext
                  );

BOOL closeKeyGroup (
                    IN Context_t *pContext
                    );

// Delete the user group
DWORD DeleteOldKeyGroup(
                        IN Context_t *pContext,
                        IN BOOL fMigration
                        );

DWORD DeleteKeyGroup(
                     IN Context_t *pContext
                     );

BOOL readKey(HKEY hLoc, char *pszName, BYTE **Data, DWORD *pcbLen);
BOOL saveKey(HKEY hLoc, CONST char *pszName, void *Data, DWORD dwLen);

void CheckKeySetType(
                     Context_t *pContext
                     );

#ifdef __cplusplus
}
#endif

