#ifndef _KERNEL32_INCLUDE_DEBUG_H
#define _KERNEL32_INCLUDE_DEBUG_H

#define UNIMPLEMENTED DbgPrint("%s at %s:%d is unimplemented\n",__FUNCTION__,__FILE__,__LINE__);

#ifdef NDEBUG
#define DPRINT(args...)
#define CHECKPOINT
#ifdef assert
#undef assert
#endif
#define assert(x)
#else
#define DPRINT(args...) do { DbgPrint("(KERNEL32:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT do { DbgPrint("(KERNEL32:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);
#ifdef assert
#undef assert
#endif
#define assert(x) do { if(!x) RtlAssert("#x", __FILE__,__LINE__, ""); } while(0);
#endif

#define DPRINT1(args...) do { DbgPrint("(KERNEL32:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT1 do { DbgPrint("(KERNEL32:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);

#endif /* ndef _KERNEL32_INCLUDE_DEBUG_H */
