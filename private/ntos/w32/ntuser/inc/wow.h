/****************************** Module Header ******************************\
* Module Name: wow.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This header file contains macros to be used in rtl\wow.c client\ and kernel\
*
* History:
* 22-AUG-97 CLupu      created
\***************************************************************************/


#if !defined(_WIN64) && !defined(BUILD_WOW6432)

#define StartValidateHandleMacro(h)                                         \
{                                                                           \
    PHE phe;                                                                \
    DWORD dw;                                                               \
    WORD uniq;                                                              \
                                                                            \
    /*                                                                      \
     * This is a macro that does an AND with HMINDEXBITS,                   \
     * so it is fast.                                                       \
     */                                                                     \
    dw = HMIndexFromHandle(h);                                              \
                                                                            \
    /*                                                                      \
     * Make sure it is part of our handle table.                            \
     */                                                                     \
    if (dw < gpsi->cHandleEntries) {                                        \
        /*                                                                  \
         * Make sure it is the handle                                       \
         * the app thought it was, by                                       \
         * checking the uniq bits in                                        \
         * the handle against the uniq                                      \
         * bits in the handle entry.                                        \
         */                                                                 \
        phe = &gSharedInfo.aheList[dw];                                     \
        uniq = HMUniqFromHandle(h);                                         \
        if (   uniq == phe->wUniq                                           \
            || uniq == 0                                                    \
            || uniq == HMUNIQBITS                                           \
            ) {                                                             \

#else  /* _WIN64 */

#define StartValidateHandleMacro(h)                                         \
{                                                                           \
    PHE phe;                                                                \
    DWORD dw;                                                               \
    WORD uniq;                                                              \
                                                                            \
    /*                                                                      \
     * This is a macro that does an AND with HMINDEXBITS,                   \
     * so it is fast.                                                       \
     */                                                                     \
    dw = HMIndexFromHandle(h);                                              \
                                                                            \
    /*                                                                      \
     * Make sure it is part of our handle table.                            \
     */                                                                     \
    if (dw < gpsi->cHandleEntries) {                                        \
        /*                                                                  \
         * Make sure it is the handle                                       \
         * the app thought it was, by                                       \
         * checking the uniq bits in                                        \
         * the handle against the uniq                                      \
         * bits in the handle entry.                                        \
         * For Win64 uniq can't be zero!                                    \
         */                                                                 \
        phe = &gSharedInfo.aheList[dw];                                     \
        uniq = HMUniqFromHandle(h);                                         \
        if (   uniq == phe->wUniq                                           \
            || uniq == HMUNIQBITS                                           \
            ) {                                                             \

#endif /* _WIN64 */

#define BeginAliveValidateHandleMacro() \
          /*                                                                   \
           * Now make sure that the handle is not destroyed.  On free          \
           * builds the RIP disappears and the main line is straightthrough.   \
           */                                                                  \
            if (!(phe->bFlags & HANDLEF_DESTROY)) {  \


#define EndAliveValidateHandleMacro() \
            } else {                                \
                RIPMSG2(RIP_WARNING, "ValidateAliveHandle: Object phe %#p is destroyed. Handle: %#p", \
                    phe, h);   \
            }   \


#define BeginTypeValidateHandleMacro(pobj, bTypeTest)                       \
            /*                                                              \
             * Now make sure the app is passing the right handle            \
             * type for this api. If the handle is TYPE_FREE, this'll       \
             * catch it.  Also let Generic requests through.                \
             */                                                             \
            if ((phe->bType == bTypeTest) ||                                \
                (bTypeTest == TYPE_GENERIC && phe->bType != TYPE_FREE)) {   \
                                                                            \
                /*                                                          \
                 * Instead of try/except we use the heap range check        \
                 * mechanism to verify that the given 'pwnd' belongs to     \
                 * the default desktop. We also have to do a Win 3.1 like   \
                 * check to make sure the window is not deleted             \
                 * See NT bug 12242 Kitchen app.  Also 6479                 \
                 *                                                          \
                 * TESTDESKOP returns the handle if the handle is valid     \
                 * in the current desktop                                   \
                 */                                                         \
                pobj = phe->phead;                                          \
                {                                                           \

#define EndTypeValidateHandleMacro                                          \
                }                                                           \
            }                                                               \

#define EndValidateHandleMacro                                              \
        }                                                                   \
    }                                                                       \
}
