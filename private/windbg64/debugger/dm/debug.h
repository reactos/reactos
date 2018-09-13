#define PPDBFromPid(x) (PDB*)x  // Take these out when the kernel function
                // is written
#define PTDBFromTid(x) (TDB*)x

#define PidFromPPDB(x)      (PID)x

#define TidFromPTDB(x)      (TID)x


//typedef   PDB *           PID;
//typedef   TDB *           TID;

typedef enum    {
    DBERR_NO_ERROR  = 0,
    DBERR_PROCESS_CREATION,
    DBERR_THREAD_CREATION,
    DBERR_BAD_ACCESS
} DB_ERRORS;


typedef enum        {
    ACCESS_READ = 0x01,
    ACCESS_WRITE    = 0x02,
    ACCESS_EXECUTE  = 0x04
} DW_DESIRED_ACCESS;

#define EVENT_UNION_SIZE    sizeof(EVENT_UNION)

typedef struct  rq {
    struct  rq      *next;
    struct  rq      *prev;
    TID thread;
}   RUNQUEUE;


void PrintDebug(void);
