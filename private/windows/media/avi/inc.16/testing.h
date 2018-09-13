/*---------------------------------------------------------*\
|                                                           |
|  TESTING.H                                                |
|                                                           |
|  Testing's very own include file!                         |
\*---------------------------------------------------------*/



/* This has all of the defines for the wParam and lParam that go along with
 * the WM_TESTING message 
 */
/* wParam defines - Area
 */
#define   TEST_PRINTMAN          0x0001
#define   TEST_GDI               0x0002


/* lParam defines - Details (in LOWORD) 
 */
#define   TEST_PRINTJOB_START    0x0001 /* when bits start going to the port */
#define   TEST_PRINTJOB_END      0x0002 /* when bits stop going to the port  */
#define   TEST_QUEUE_READY       0x0003 /* when the queue is ready to accept a job */
#define   TEST_QUEUE_EMPTY       0x0004 /* when the last job is done being sent    */

#define   TEST_START_DOC         0x0001 /* print job is started              */
#define   TEST_END_DOC           0x0002 /* print job is ended                */


/* Defines for UserSeeUserDo and GDISeeGDIDo functions 
 */
LONG API UserSeeUserDo(WORD wMsg, WORD wParam, LONG lParam);
LONG API GDISeeGDIDo(WORD wMsg, WORD wParam, LONG lParam);

/* Defines for the various messages one can pass for the SeeDo functions. 
 */
#define SD_LOCALALLOC   0x0001  /* Alloc using flags wParam and lParam bytes.
                                 * Returns handle to data.  
                                 */
#define SD_LOCALFREE    0x0002  /* Free the memory allocated by handle wParam
                                 */
#define SD_LOCALCOMPACT 0x0003  /* Return the number of free bytes available 
                                 */
#define SD_GETUSERMENUHEAP 0x0004 /* Return the handle to the far menu heap
                                   * maintained by user. 
                                   */
#define SD_GETCLASSHEADPTR 0x0005 /* Return the near pointer to the head of 
                                   * the linked list of CLS structures.
                                   * Interface: wParam = NULL; lParam = NULL;
                                   */

#define SD_GETUSERHWNDHEAP 0x0006 /* Return the handle to the far window heap
                                   * maintained by user.
                                   */

#define SD_GETGDIHEAP      0x0007 /* Return the handle to the far heap
                                   * maintained by gdi.
                                   */
#define SD_GETPDCEFIRST    0x0008 /* Returns USER's head of dc cache entry list */
#define SD_GETHWNDDESKTOP  0x0009 /* Returns USER's head of window tree */

#define SD_LOCAL32ALLOC    0x000A /* Allocs mem from 32-bit heap.
                                   * wParam = heap (0=Window/GDI, 1=Menu)
                                   * lParam = amount of memory to allocate.
                                   * returns handle of memory
                                   */

#define SD_LOCAL32FREE     0x000B /* Frees mem allocated by SD_LOCAL32ALLOC.
                                   * wParam = heap (0=Window/GDI, 1=Menu)
                                   * lParam = handle
                                   * returns nothing.
                                   */

#define SD_GETSAFEMODE     0x000C /* Returns GDI's safe mode setting.
                                   * 0 = full acceleration
                                   * 1 = minimal acceleration
                                   * 2 = no acceleration
                                   * If the user requests safe mode but the
                                   *   display driver doesn't support it the
                                   *   safe mode setting will be 0
                                   */

#define SD_GETESIEDIPTRS   0x000D /* Returns a bit array indicating which
                                   * display driver DDIs have trashed esi or
                                   * edi.  This only works in DEBUG.  In RETAIL
                                   * this function will return 0.
                                   *
                                   * Each DDI uses two bits in the bit array
                                   * that indicate if esi, edi, resp. have been
                                   * trashed.  The table is 32 bytes long.
                                   *
                                   * For the DCT tests make sure all 32 bytes
                                   * are 0.
                                   */

