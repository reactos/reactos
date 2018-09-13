// SqldprxyKey.h - startup key for sqldprxy

// proxy gets started up with 2 args, the first is the key,
// the 2nd is the event name (which includes the PID to make it unique)
#define SZSQLPROXYKEY	"SqlProxyKeyArg"

void __inline BuildProxyEventName( char *p )
{
	wsprintf(p, "MS.MSDev.Event.Ent.%x", GetCurrentProcessId() );
}

#ifdef DEBUG
#define	szSqlProxyBase	"sqlprxyd"
#else
#define	szSqlProxyBase	"sqlprxy"
#endif
