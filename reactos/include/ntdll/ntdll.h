
#define UNIMPLEMENTED DbgPrint("%s in %s:%d is unimplemented\n",__FUNCTION__,__FILE__,__LINE__);

#ifdef NDEBUG
#define DPRINT(args...) 
#else
#define DPRINT(args...) do { DbgPrint("(NTDLL:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#endif

#define DPRINT1(args...) do { DbgPrint("(NTDLL:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

