/*
 * dataobj.hpp - IDataObject implementation description.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Prototypes
 *************/

// dataobj.cpp

#define InitDataObjectModule() RegisterClipboardFormats()
extern BOOL RegisterClipboardFormats(void);
extern void ExitDataObjectModule(void);


/* Global Variables
 *******************/

// dataobj.cpp

extern UINT g_cfURL;
extern UINT g_cfFileGroupDescriptor;
extern UINT g_cfFileContents;


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

