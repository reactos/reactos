#ifndef __DBG_H__
#define __DBG_H__

/*
void Assert(void* assert, const char* file, int line, void* msg);
#define ASSERT Assert(void* assert, const char* file, int line, void* msg)
#define D(_x_) \
        printf("(%hS:%d)(%hS) ", __FILE__, __LINE__, __FUNCTION__); \
	    printf _x_;
 */

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef NASSERT
#define ASSERT(x)
#else /* NASSERT */
//#define ASSERT(x) if (!(x)) { printf("Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__); assert(0); }
#define ASSERT(x) if (!(x)) { printf("Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__); }
#endif /* NASSERT */


#ifdef __GNUC__

#else

//#define __FUNCTION__ "unknown"

#endif /*__GNUC__*/


#endif /*__DBG_H__*/
