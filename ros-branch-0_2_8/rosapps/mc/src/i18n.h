/* GNOME already handles the intenrationalization issues */
#ifndef _MC_I18N_H_
#define _MC_I18N_H_

         /* Stubs that do something close enough.  */
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,Message) (Message)
#define dcgettext(Domain,Message,Type) (Message)
#define bindtextdomain(Domain,Directory) (Domain)
#define _(String) (String)
#define N_(String) (String)

#endif /* _MC_I18N_H_ */
