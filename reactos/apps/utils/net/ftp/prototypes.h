
int fgetcSocket(int s);
char *fputsSocket(char *format, int s);

char *fprintfSocket(int s, char *format, ...);

int fputcSocket(int s, char putChar);
int fputSocket(int s, char *putChar, int len);
char *fgetsSocket(int s, char *string);

char *hookup();
char **glob();
int herror(char *s);

int getreply(int expecteof);
int ruserpass(char *host, char **aname, char **apass, char **aacct);
char *getpass(const char *prompt);
void makeargv(void);
void domacro(int argc, char *argv[]);
void proxtrans(char *cmd, char *local, char *remote);
int null(void);
int initconn(void);
void disconnect(void);
void ptransfer(char *direction, long bytes, struct timeval *t0, struct timeval *t1);
void setascii(void);
void setbinary(void);
void setebcdic(void);
void settenex(void);
void tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0);
void setpassive(int argc, char *argv[]);
void setpeer(int argc, char *argv[]);
void cmdscanner(int top);
void pswitch(int flag);
void quit(void);
int login(char *host);
int command(char *fmt, ...);
int globulize(char **cpp);
void sendrequest(char *cmd, char *local, char *remote, int printnames);
void recvrequest(char *cmd, char *local, char *remote, char *mode,
                int printnames);
int confirm(char *cmd, char *file);
void blkfree(char **av0);
int getit(int argc, char *argv[], int restartit, char *mode);
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
