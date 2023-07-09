
/*
 * MMSYSTEM precludes
 */

#define         MMNOSOUND
#define         MMNOWAVE
#define         MMNOSEQ
#define         MMNOTIMER
#define         MMNOJOY
#define         MMNOMCI
#define         MMNOTASK

/*
 * WINDOWS precludes
*/

#define         NOGDICAPMASKS           //- CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define         NOVIRTUALKEYCODES       //- VK_*
#define         NOICONS                 //- IDI_*
#define         NOKEYSTATES             //- MK_*
#define         NOSYSCOMMANDS           //- SC_*
#define         NORASTEROPS             //- Binary and Tertiary raster ops
#define         OEMRESOURCE             //- OEM Resource values
//#define               NOATOM                  //- Atom Manager routines
#define         NOCLIPBOARD             //- Clipboard routines
#define         NOMETAFILE              //- typedef METAFILEPICT
//#define               NOOPENFILE              //- OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define         NOSOUND                 //- Sound driver routines
#define         NOWH                    //- SetWindowsHook and WH_*
#define         NOCOMM                  //- COMM driver routines
#define         NOKANJI                 //- Kanji support stuff.
//#define               NOHELP                  //- Help engine interface.
#define         NOPROFILER              //- Profiler interface.

