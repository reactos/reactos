#ifdef NDEBUG
#define DPRINT(args...)
#define CHECKPOINT
#else
#define DPRINT(args...) do { DebugPrint("(SAMLIB:%s:%d) ",__FILE__,__LINE__); DebugPrint(args); } while(0)
#define CHECKPOINT do { DebugPrint("(SAMLIB:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0)
#endif

#define DPRINT1(args...) do { DebugPrint("(SAMLIB:%s:%d) ",__FILE__,__LINE__); DebugPrint(args); } while(0)
#define CHECKPOINT1 do { DebugPrint("(SAMLIB:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0)


void
DebugPrint(char* fmt,...);

/* EOF */
