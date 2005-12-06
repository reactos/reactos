
int fgetcSocket(int s);
const char *fputsSocket(const char *format, int s);

const char *fprintfSocket(int s, const char *format, ...);

int fputcSocket(int s, char putChar);
int fputSocket(int s, char *putChar, int len);
char *fgetsSocket(int s, char *string);

char *hookup();
char **glob();
int herror(char *s);

int getreply(int expecteof);
int ruserpass(const char *host, char **aname, char **apass, char **aacct);
char *getpass(const char *prompt);
void makeargv(void);
void domacro(int argc, const char *argv[]);
void proxtrans(const char *cmd, const char *local, const char *remote);
int null(void);
int initconn(void);
void disconnect(void);
void ptransfer(const char *direction, long bytes, struct timeval *t0, struct timeval *t1);
void setascii(void);
void setbinary(void);
void setebcdic(void);
void settenex(void);
void tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0);
void setpassive(int argc, char *argv[]);
void setpeer(int argc, const char *argv[]);
void cmdscanner(int top);
void pswitch(int flag);
void quit(void);
int login(const char *host);
int command(const char *fmt, ...);
int globulize(const char **cpp);
void sendrequest(const char *cmd, const char *local, const char *remote, int printnames);
void recvrequest(const char *cmd, const char *local, const char *remote, const char *mode,
                int printnames);
int confirm(const char *cmd, const char *file);
void blkfree(char **av0);
int getit(int argc, const char *argv[], int restartit, const char *mode);
int sleep(int time);

char *tail();
int errno;
char *mktemp();
void	setbell(), setdebug();
void	setglob(), sethash(), setport();
void	setprompt();
void	settrace(), setverbose();
void	settype(), setform(), setstruct();
void	restart(), syst();
void	cd(), lcd(), delete(), mdelete();
void	ls(), mls(), get(), mget(), help(), append(), put(), mput(), reget();
void	status();
void	renamefile();
void	quote(), rmthelp(), site();
void	pwd(), makedir(), removedir(), setcr();
void	account(), doproxy(), reset(), setcase(), setntrans(), setnmap();
void	setsunique(), setrunique(), cdup(), macdef();
void	sizecmd(), modtime(), newer(), rmtstatus();
void	do_chmod(), do_umask(), idle();
void	shell(), user(), fsetmode();
struct cmd	*getcmd();
