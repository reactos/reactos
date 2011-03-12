
int fgetcSocket(int s);
const char *fputsSocket(const char *format, int s);

const char *fprintfSocket(int s, const char *format, ...);

int fputcSocket(int s, char putChar);
int fputSocket(int s, char *putChar, int len);
char *fgetsSocket(int s, char *string);

char *hookup(void);
char **glob(void);
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

char *tail(void);
void	setbell(void), setdebug(void);
void	setglob(void), sethash(void), setport(void);
void	setprompt(void);
void	settrace(void), setverbose(void);
void	settype(void), setform(void), setstruct(void);
void	restart(void), syst(void);
void	cd(void), lcd(void), delete(void), mdelete(void);
void	ls(void), mls(void), get(void), mget(void), help(void), append(void), put(void), mput(void), reget(void);
void	status(void);
void	renamefile(void);
void	quote(void), rmthelp(void), site(void);
void	pwd(void), makedir(void), removedir(void), setcr(void);
void	account(void), doproxy(void), reset(void), setcase(void), setntrans(void), setnmap(void);
void	setsunique(void), setrunique(void), cdup(void), macdef(void);
void	sizecmd(void), modtime(void), newer(void), rmtstatus(void);
void	do_chmod(void), do_umask(void), idle(void);
void	shell(void), user(void), fsetmode(void);
struct cmd	*getcmd(void);
