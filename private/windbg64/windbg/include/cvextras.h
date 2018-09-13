/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/

enum _MSG_WHERE {
    CMDWINDOW = 1,
    MSGBOX,
    STATLINE,
    MSGSTRING,
    MSGGERRSTR
    };
typedef int     MSGWHERE;

enum _MSG_TYPE {
    ERRORMSG = 1,
    WARNMSG,
    INFOMSG,
    MESSAGEMSG,
    FMESSAGEMSG,
    EXPREVALMSG
    };
typedef int     MSGTYPE;
typedef DWORD   MSGID;

enum {
    mStack = 1
};

#define EENOERROR   0

#define EXPRERROR   255

int ESilan(void);

void PASCAL CVExprErr(EESTATUS, MSGWHERE, PHTM, char FAR *);
int         ESSetFromIndex(int);
void        SYSetFrame(HFRAME hFrame);
int         get_initial_context(PCXT pCXT, BOOL fSearchAll);

void PASCAL UpdateUserEnvir( unsigned short fReq );

void
CVMessage (
    MSGTYPE msgtype,
    MSGID msgid,
    MSGWHERE msgwhere,
    ...
    );
