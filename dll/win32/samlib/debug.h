#ifdef NDEBUG
#define DPRINT(...)
#define CHECKPOINT
#else
#define DPRINT(...) do { DebugPrint("(SAMLIB:%s:%d) ",__FILE__,__LINE__); DebugPrint(__VA_ARGS__); } while(0)
#define CHECKPOINT do { DebugPrint("(SAMLIB:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0)
#endif

#define DPRINT1(...) do { DebugPrint("(SAMLIB:%s:%d) ",__FILE__,__LINE__); DebugPrint(__VA_ARGS__); } while(0)
#define CHECKPOINT1 do { DebugPrint("(SAMLIB:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0)


void
DebugPrint(char* fmt,...);

/* EOF */
