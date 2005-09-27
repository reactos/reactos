#define ECHO_PORT 7
#define CHARGEN_PORT 19
#define DAYTIME_PORT 13
#define DISCARD_PORT 9
#define QOTD_PORT 17

#define MAX_THREADS 2
#define BUF_SIZE 255

typedef struct _MyData {
    INT Port;
    LPTHREAD_START_ROUTINE Service;
} MYDATA, *PMYDATA;
