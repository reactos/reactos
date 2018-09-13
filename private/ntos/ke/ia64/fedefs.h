#define IN_KERNEL 1

#if 0
/* #define this in floatem.c, fedefs.h and EM_support.c */
#define DEBUG_UNIX
#endif



#ifdef IN_KERNEL

#define FP_EMULATION_ERROR0(string) \
              {DbgPrint(string);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR1(string, arg) \
              {DbgPrint(string, arg);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR2(string, arg1, arg2) \
              {DbgPrint(string, arg1, arg2);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR3(string, arg1, arg2, arg3) \
              {DbgPrint(string, arg1, arg2, arg3);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR4(string, arg1, arg2, arg3, arg4) \
              {DbgPrint(string, arg1, arg2, arg3, arg4);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR5(string, arg1, arg2, arg3, arg4, arg5) \
              {DbgPrint(string, arg1, arg2, arg3, arg4, arg5);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR6(string, arg1, arg2, arg3, arg4, arg5, arg6) \
              {DbgPrint(string, arg1, arg2, arg3, arg4, arg5, arg6);  \
               KeBugCheck(FP_EMULATION_ERROR); }

#define FP_EMULATION_PRINT0(string) \
              {DbgPrint(string); }
#define FP_EMULATION_PRINT1(string, arg) \
              {DbgPrint(string, arg); }
#define FP_EMULATION_PRINT2(string, arg1, arg2) \
              {DbgPrint(string, arg1, arg2); }
#define FP_EMULATION_PRINT3(string, arg1, arg2, arg3) \
              {DbgPrint(string, arg1, arg2, arg3); }
#define FP_EMULATION_PRINT4(string, arg1,arg2, arg3, arg4) \
              {DbgPrint(string, arg1, arg2, arg3, arg4); }
#define FP_EMULATION_PRINT5(string, arg1, arg2, arg3, arg4, arg5) \
              {DbgPrint(string, arg1, arg2, arg3, arg4, arg5); }
#define FP_EMULATION_PRINT6(string, arg1, arg2, arg3, arg4, arg5, arg6) \
              {DbgPrint(string, arg1, arg2, arg3, arg4, arg5, arg6); }

#define perror(string) DbgPrint(string)

#define exit(number) KeBugCheck(FP_EMULATION_ERROR)


#elif defined(unix)

#define FP_EMULATION_ERROR0(string) \
              {DbgPrint(string);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR1(string, arg) \
              {DbgPrint(string, arg);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR2(string, arg1, arg2) \
              {DbgPrint(string, arg1, arg2);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR3(string, arg1, arg2, arg3) \
              {DbgPrint(string, arg1, arg2, arg3);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR4(string, arg1, arg2, arg3, arg4) \
              {DbgPrint(string, arg1, arg2, arg3, arg4);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR5(string, arg1, arg2, arg3, arg4, arg5) \
              {DbgPrint(string, arg1, arg2, arg3, arg4, arg5);  \
               KeBugCheck(FP_EMULATION_ERROR); }
#define FP_EMULATION_ERROR6(string, arg1, arg2, arg3, arg4, arg5, arg6) \
              {DbgPrint(string, arg1, arg2, arg3, arg4, arg5, arg6);  \
               KeBugCheck(FP_EMULATION_ERROR); }

#define FP_EMULATION_PRINT0(string) \
              {DbgPrint(string); }
#define FP_EMULATION_PRINT1(string, arg) \
              {DbgPrint(string, arg); }
#define FP_EMULATION_PRINT2(string, arg1, arg2) \
              {DbgPrint(string, arg1, arg2); }
#define FP_EMULATION_PRINT3(string, arg1, arg2, arg3) \
              {DbgPrint(string, arg1, arg2, arg3); }
#define FP_EMULATION_PRINT4(string, arg1,arg2, arg3, arg4) \
              {DbgPrint(string, arg1, arg2, arg3, arg4); }
#define FP_EMULATION_PRINT5(string, arg1, arg2, arg3, arg4, arg5) \
              {DbgPrint(string, arg1, arg2, arg3, arg4, arg5); }
#define FP_EMULATION_PRINT6(string, arg1, arg2, arg3, arg4, arg5, arg6) \
              {DbgPrint(string, arg1, arg2, arg3, arg4, arg5, arg6); }

#ifdef DEBUG_UNIX
#define DbgPrint printf
#else
#define DbgPrint(string)
#endif
#define KeBugCheck(FP_EMULATION_ERROR) return


#else

#define FP_EMULATION_ERROR0(string) \
              { fprintf (stderr, string); exit (1); }
#define FP_EMULATION_ERROR1(string, arg) \
             { fprintf (stderr, string, arg); \
             exit (1); }
#define FP_EMULATION_ERROR2(string, arg1, arg2) \
             { fprintf (stderr, string, arg1, arg2); \
             exit (1); }
#define FP_EMULATION_ERROR3(string, arg1, arg2, arg3) \
             { fprintf (stderr, string, arg1, arg2, arg3); \
             exit (1); }
#define FP_EMULATION_ERROR4(string, arg1, arg2, arg3, arg4) \
             { fprintf (stderr, string, arg1, arg2, arg3, arg4); \
             exit (1); }
#define FP_EMULATION_ERROR5(string, arg1, arg2, arg3, arg4, arg5) \
             { fprintf (stderr, string, arg1, arg2, arg3, arg4, arg5); \
             exit (1); }
#define FP_EMULATION_ERROR6(string, arg1, arg2, arg3, arg4, arg5, arg6) \
             { fprintf (stderr, string, arg1, arg2, arg3, arg4, arg5, arg6); \
             exit (1); }

#define FP_EMULATION_PRINT0(string) \
             { printf (string); \
             fflush (stdout); }
#define FP_EMULATION_PRINT1(string, arg) \
             { printf (string, arg); \
             fflush (stdout); }
#define FP_EMULATION_PRINT2(string, arg1, arg2) \
             { printf (string, arg1, arg2); \
             fflush (stdout); }
#define FP_EMULATION_PRINT3(string, arg1, arg2, arg3) \
             { printf (string, arg1, arg2, arg3); \
             fflush (stdout); }
#define FP_EMULATION_PRINT4(string, arg1, arg2, arg3, arg4) \
             { printf (string, arg1, arg2, arg3, arg4); \
             fflush (stdout); }
#define FP_EMULATION_PRINT5(string, arg1, arg2, arg3, arg4, arg5) \
             { printf (string, arg1, arg2, arg3, arg4, arg5); \
             fflush (stdout); }
#define FP_EMULATION_PRINT6(string, arg1, arg2, arg3, arg4, arg5, arg6) \
             { printf (string, arg1, arg2, arg3, arg4, arg5, arg6); \
             fflush (stdout); }

#endif
