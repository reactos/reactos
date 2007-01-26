#ifndef _KERNEL32_INCLUDE_DEBUG_H
#define _KERNEL32_INCLUDE_DEBUG_H

#define UNIMPLEMENTED DbgPrint("%s at %s:%d is unimplemented\n",__FUNCTION__,__FILE__,__LINE__);

#ifdef NDEBUG
#ifdef __GNUC__
#define DPRINT(args...)
#else
#define DPRINT
#endif
#define CHECKPOINT
#else
#define DPRINT(...) do { DbgPrint("(KERNEL32:%s:%d) ",__FILE__,__LINE__); DbgPrint(__VA_ARGS__); } while(0);
#define CHECKPOINT do { DbgPrint("(KERNEL32:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);
#endif

#ifdef ASSERT
#undef ASSERT
#define ASSERT(x) do { if(!x) RtlAssert("#x", __FILE__,__LINE__, ""); } while(0);
#endif
#define DPRINT1(...) do { DbgPrint("(KERNEL32:%s:%d) ",__FILE__,__LINE__); DbgPrint(__VA_ARGS__); } while(0);
#define CHECKPOINT1 do { DbgPrint("(KERNEL32:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);

#endif /* ndef _KERNEL32_INCLUDE_DEBUG_H */
