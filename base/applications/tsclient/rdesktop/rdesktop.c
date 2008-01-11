/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Entrypoint and utility functions
   Copyright (C) Matthew Chapman 1999-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdarg.h>		/* va_list va_start va_end */
#include <unistd.h>		/* read close getuid getgid getpid getppid gethostname */
#include <fcntl.h>		/* open */
#include <pwd.h>		/* getpwuid */
#include <termios.h>		/* tcgetattr tcsetattr */
#include <sys/stat.h>		/* stat */
#include <sys/time.h>		/* gettimeofday */
#include <sys/times.h>		/* times */
#include <ctype.h>		/* toupper */
#include <errno.h>
#include "rdesktop.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_ICONV
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#endif

#ifdef EGD_SOCKET
#include <sys/types.h>
#include <sys/socket.h>		/* socket connect */
#include <sys/un.h>		/* sockaddr_un */
#endif

#include <openssl/md5.h>

#ifdef RDP2VNC
void
rdp2vnc_connect(char *server, uint32 flags, char *domain, char *password,
		char *shell, char *directory);
#endif
/* Display usage information */
static void
usage(char *program)
{
	fprintf(stderr, "rdesktop: A Remote Desktop Protocol client.\n");
	fprintf(stderr, "Version " VERSION ". Copyright (C) 1999-2005 Matt Chapman.\n");
	fprintf(stderr, "See http://www.rdesktop.org/ for more information.\n\n");

	fprintf(stderr, "Usage: %s [options] server[:port]\n", program);
#ifdef RDP2VNC
	fprintf(stderr, "   -V: vnc port\n");
	fprintf(stderr, "   -Q: defer time (ms)\n");
#endif
	fprintf(stderr, "   -u: user name\n");
	fprintf(stderr, "   -d: domain\n");
	fprintf(stderr, "   -s: shell\n");
	fprintf(stderr, "   -c: working directory\n");
	fprintf(stderr, "   -p: password (- to prompt)\n");
	fprintf(stderr, "   -n: client hostname\n");
	fprintf(stderr, "   -k: keyboard layout on server (en-us, de, sv, etc.)\n");
	fprintf(stderr, "   -g: desktop geometry (WxH)\n");
	fprintf(stderr, "   -f: full-screen mode\n");
	fprintf(stderr, "   -b: force bitmap updates\n");
#ifdef HAVE_ICONV
	fprintf(stderr, "   -L: local codepage\n");
#endif
	fprintf(stderr, "   -A: enable SeamlessRDP mode\n");
	fprintf(stderr, "   -B: use BackingStore of X-server (if available)\n");
	fprintf(stderr, "   -e: disable encryption (French TS)\n");
	fprintf(stderr, "   -E: disable encryption from client to server\n");
	fprintf(stderr, "   -m: do not send motion events\n");
	fprintf(stderr, "   -C: use private colour map\n");
	fprintf(stderr, "   -D: hide window manager decorations\n");
	fprintf(stderr, "   -K: keep window manager key bindings\n");
	fprintf(stderr, "   -S: caption button size (single application mode)\n");
	fprintf(stderr, "   -T: window title\n");
	fprintf(stderr, "   -N: enable numlock syncronization\n");
	fprintf(stderr, "   -X: embed into another window with a given id.\n");
	fprintf(stderr, "   -a: connection colour depth\n");
	fprintf(stderr, "   -z: enable rdp compression\n");
	fprintf(stderr, "   -x: RDP5 experience (m[odem 28.8], b[roadband], l[an] or hex nr.)\n");
	fprintf(stderr, "   -P: use persistent bitmap caching\n");
	fprintf(stderr, "   -r: enable specified device redirection (this flag can be repeated)\n");
	fprintf(stderr,
		"         '-r comport:COM1=/dev/ttyS0': enable serial redirection of /dev/ttyS0 to COM1\n");
	fprintf(stderr, "             or      COM1=/dev/ttyS0,COM2=/dev/ttyS1\n");
	fprintf(stderr,
		"         '-r disk:floppy=/mnt/floppy': enable redirection of /mnt/floppy to 'floppy' share\n");
	fprintf(stderr, "             or   'floppy=/mnt/floppy,cdrom=/mnt/cdrom'\n");
	fprintf(stderr, "         '-r clientname=<client name>': Set the client name displayed\n");
	fprintf(stderr, "             for redirected disks\n");
	fprintf(stderr,
		"         '-r lptport:LPT1=/dev/lp0': enable parallel redirection of /dev/lp0 to LPT1\n");
	fprintf(stderr, "             or      LPT1=/dev/lp0,LPT2=/dev/lp1\n");
	fprintf(stderr, "         '-r printer:mydeskjet': enable printer redirection\n");
	fprintf(stderr,
		"             or      mydeskjet=\"HP LaserJet IIIP\" to enter server driver as well\n");
	fprintf(stderr, "         '-r sound:[local|off|remote]': enable sound redirection\n");
	fprintf(stderr, "                     remote would leave sound on server\n");
	fprintf(stderr,
		"         '-r clipboard:[off|PRIMARYCLIPBOARD|CLIPBOARD]': enable clipboard\n");
	fprintf(stderr, "                      redirection.\n");
	fprintf(stderr,
		"                      'PRIMARYCLIPBOARD' looks at both PRIMARY and CLIPBOARD\n");
	fprintf(stderr, "                      when sending data to server.\n");
	fprintf(stderr, "                      'CLIPBOARD' looks at only CLIPBOARD.\n");
	fprintf(stderr, "   -0: attach to console\n");
	fprintf(stderr, "   -4: use RDP version 4\n");
	fprintf(stderr, "   -5: use RDP version 5 (default)\n");
}

static void
print_disconnect_reason(uint16 reason)
{
	char *text;

	switch (reason)
	{
		case exDiscReasonNoInfo:
			text = "No information available";
			break;

		case exDiscReasonAPIInitiatedDisconnect:
			text = "Server initiated disconnect";
			break;

		case exDiscReasonAPIInitiatedLogoff:
			text = "Server initiated logoff";
			break;

		case exDiscReasonServerIdleTimeout:
			text = "Server idle timeout reached";
			break;

		case exDiscReasonServerLogonTimeout:
			text = "Server logon timeout reached";
			break;

		case exDiscReasonReplacedByOtherConnection:
			text = "The session was replaced";
			break;

		case exDiscReasonOutOfMemory:
			text = "The server is out of memory";
			break;

		case exDiscReasonServerDeniedConnection:
			text = "The server denied the connection";
			break;

		case exDiscReasonServerDeniedConnectionFips:
			text = "The server denied the connection for security reason";
			break;

		case exDiscReasonLicenseInternal:
			text = "Internal licensing error";
			break;

		case exDiscReasonLicenseNoLicenseServer:
			text = "No license server available";
			break;

		case exDiscReasonLicenseNoLicense:
			text = "No valid license available";
			break;

		case exDiscReasonLicenseErrClientMsg:
			text = "Invalid licensing message";
			break;

		case exDiscReasonLicenseHwidDoesntMatchLicense:
			text = "Hardware id doesn't match software license";
			break;

		case exDiscReasonLicenseErrClientLicense:
			text = "Client license error";
			break;

		case exDiscReasonLicenseCantFinishProtocol:
			text = "Network error during licensing protocol";
			break;

		case exDiscReasonLicenseClientEndedProtocol:
			text = "Licensing protocol was not completed";
			break;

		case exDiscReasonLicenseErrClientEncryption:
			text = "Incorrect client license enryption";
			break;

		case exDiscReasonLicenseCantUpgradeLicense:
			text = "Can't upgrade license";
			break;

		case exDiscReasonLicenseNoRemoteConnections:
			text = "The server is not licensed to accept remote connections";
			break;

		default:
			if (reason > 0x1000 && reason < 0x7fff)
			{
				text = "Internal protocol error";
			}
			else
			{
				text = "Unknown reason";
			}
	}
	fprintf(stderr, "disconnect: %s.\n", text);
}

static void
rdesktop_reset_state(RDPCLIENT * This)
{
	rdp_reset_state(This);
}

static BOOL
read_password(char *password, int size)
{
	struct termios tios;
	BOOL ret = False;
	int istty = 0;
	char *p;

	if (tcgetattr(STDIN_FILENO, &tios) == 0)
	{
		fprintf(stderr, "Password: ");
		tios.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &tios);
		istty = 1;
	}

	if (fgets(password, size, stdin) != NULL)
	{
		ret = True;

		/* strip final newline */
		p = strchr(password, '\n');
		if (p != NULL)
			*p = 0;
	}

	if (istty)
	{
		tios.c_lflag |= ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &tios);
		fprintf(stderr, "\n");
	}

	return ret;
}

static void
parse_server_and_port(RDPCLIENT * This, char *server)
{
	char *p;
#ifdef IPv6
	int addr_colons;
#endif

#ifdef IPv6
	p = server;
	addr_colons = 0;
	while (*p)
		if (*p++ == ':')
			addr_colons++;
	if (addr_colons >= 2)
	{
		/* numeric IPv6 style address format - [1:2:3::4]:port */
		p = strchr(server, ']');
		if (*server == '[' && p != NULL)
		{
			if (*(p + 1) == ':' && *(p + 2) != '\0')
				This->tcp_port_rdp = strtol(p + 2, NULL, 10);
			/* remove the port number and brackets from the address */
			*p = '\0';
			strncpy(server, server + 1, strlen(server));
		}
	}
	else
	{
		/* dns name or IPv4 style address format - server.example.com:port or 1.2.3.4:port */
		p = strchr(server, ':');
		if (p != NULL)
		{
			This->tcp_port_rdp = strtol(p + 1, NULL, 10);
			*p = 0;
		}
	}
#else /* no IPv6 support */
	p = strchr(server, ':');
	if (p != NULL)
	{
		This->tcp_port_rdp = strtol(p + 1, NULL, 10);
		*p = 0;
	}
#endif /* IPv6 */

}

/* Client program */
int
main(int argc, char *argv[])
{
	char server[64];
	char fullhostname[64];
	char domain[16];
	char password[64];
	char shell[256];
	char directory[256];
	BOOL prompt_password, deactivated;
	struct passwd *pw;
	uint32 flags, ext_disc_reason = 0;
	char *p;
	int c;
	char *locale = NULL;
	int username_option = 0;
	BOOL geometry_option = False;
	int run_count = 0;	/* Session Directory support */
	BOOL continue_connect = True;	/* Session Directory support */
	RDPCLIENT * This;

	This = xmalloc(sizeof(RDPCLIENT));
	memset(This, 0, sizeof(RDPCLIENT));

	This->keylayout = 0x409;	/* Defaults to US keyboard layout */
	This->keyboard_type = 0x4;	/* Defaults to US keyboard layout */
	This->keyboard_subtype = 0x0;	/* Defaults to US keyboard layout */
	This->keyboard_functionkeys = 0xc;	/* Defaults to US keyboard layout */
	This->width = 800;		/* width is special: If 0, the
					   geometry will be fetched from
					   _NET_WORKAREA. If negative,
					   absolute value specifies the
					   percent of the whole screen. */
	This->height = 600;
	This->server_depth = -1;
	This->bitmap_compression = True;
	This->sendmotion = True;
	This->bitmap_cache = True;
	This->bitmap_cache_persist_enable = False;
	This->bitmap_cache_precache = True;
	This->encryption = True;
	This->packet_encryption = True;
	This->desktop_save = True;	/* desktop save order */
	This->polygon_ellipse_orders = True;	/* polygon / ellipse orders */
	This->fullscreen = False;
	This->grab_keyboard = True;
	This->hide_decorations = False;
	This->use_rdp5 = True;
	This->rdpclip = True;
	This->console_session = False;
	This->numlock_sync = False;
	This->lspci_enabled = False;
	This->owncolmap = False;
	This->ownbackstore = True;	/* We can't rely on external BackingStore */
	This->seamless_rdp = False;
	This->rdp5_performanceflags = RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;
	This->tcp_port_rdp = TCP_PORT_RDP;

#define NOT_SET -1
	This->cache.bmpcache_lru[0] = NOT_SET;
	This->cache.bmpcache_lru[1] = NOT_SET;
	This->cache.bmpcache_lru[2] = NOT_SET;
	This->cache.bmpcache_mru[0] = NOT_SET;
	This->cache.bmpcache_mru[1] = NOT_SET;
	This->cache.bmpcache_mru[2] = NOT_SET;

#ifdef HAVE_ICONV
	This->rdp.iconv_works = True;
#endif

	This->xclip.auto_mode = True;

#ifdef HAVE_LOCALE_H
	/* Set locale according to environment */
	locale = setlocale(LC_ALL, "");
	if (locale)
	{
		locale = xstrdup(locale);
	}

#endif
	flags = RDP_LOGON_NORMAL;
	prompt_password = False;
	domain[0] = password[0] = shell[0] = directory[0] = 0;
	This->embed_wnd = 0;

	This->num_devices = 0;

#ifdef RDP2VNC
#define VNCOPT "V:Q:"
#else
#define VNCOPT
#endif

	while ((c = getopt(argc, argv,
			   VNCOPT "Au:L:d:s:c:p:n:k:g:fbBeEmzCDKS:T:NX:a:x:Pr:045h?")) != -1)
	{
		switch (c)
		{
#ifdef RDP2VNC
			case 'V':
				This->rfb_port = strtol(optarg, NULL, 10);
				if (This->rfb_port < 100)
					This->rfb_port += 5900;
				break;

			case 'Q':
				This->defer_time = strtol(optarg, NULL, 10);
				if (This->defer_time < 0)
					This->defer_time = 0;
				break;
#endif

			case 'A':
				This->seamless_rdp = True;
				break;

			case 'u':
				STRNCPY(This->username, optarg, sizeof(This->username));
				username_option = 1;
				break;

			case 'L':
#ifdef HAVE_ICONV
				STRNCPY(This->codepage, optarg, sizeof(This->codepage));
#else
				error("iconv support not available\n");
#endif
				break;

			case 'd':
				STRNCPY(domain, optarg, sizeof(domain));
				break;

			case 's':
				STRNCPY(shell, optarg, sizeof(shell));
				break;

			case 'c':
				STRNCPY(directory, optarg, sizeof(directory));
				break;

			case 'p':
				if ((optarg[0] == '-') && (optarg[1] == 0))
				{
					prompt_password = True;
					break;
				}

				STRNCPY(password, optarg, sizeof(password));
				flags |= RDP_LOGON_AUTO;

				/* try to overwrite argument so it won't appear in ps */
				p = optarg;
				while (*p)
					*(p++) = 'X';
				break;

			case 'n':
				STRNCPY(This->hostname, optarg, sizeof(This->hostname));
				break;

			case 'k':
				STRNCPY(This->keymapname, optarg, sizeof(This->keymapname));
				break;

			case 'g':
				geometry_option = True;
				This->fullscreen = False;
				if (!strcmp(optarg, "workarea"))
				{
					This->width = This->height = 0;
					break;
				}

				This->width = strtol(optarg, &p, 10);
				if (This->width <= 0)
				{
					error("invalid geometry\n");
					return 1;
				}

				if (*p == 'x')
					This->height = strtol(p + 1, &p, 10);

				if (This->height <= 0)
				{
					error("invalid geometry\n");
					return 1;
				}

				if (*p == '%')
				{
					This->width = -This->width;
					p++;
				}

				if (*p == '+' || *p == '-')
				{
					This->pos |= (*p == '-') ? 2 : 1;
					This->xpos = strtol(p, &p, 10);

				}
				if (*p == '+' || *p == '-')
				{
					This->pos |= (*p == '-') ? 4 : 1;
					This->ypos = strtol(p, NULL, 10);
				}

				break;

			case 'f':
				This->fullscreen = True;
				break;

			case 'b':
				This->bitmap_cache = False;
				break;

			case 'B':
				This->ownbackstore = False;
				break;

			case 'e':
				This->encryption = False;
				break;
			case 'E':
				This->packet_encryption = False;
				break;
			case 'm':
				This->sendmotion = False;
				break;

			case 'C':
				This->owncolmap = True;
				break;

			case 'D':
				This->hide_decorations = True;
				break;

			case 'K':
				This->grab_keyboard = False;
				break;

			case 'S':
				if (!strcmp(optarg, "standard"))
				{
					This->win_button_size = 18;
					break;
				}

				This->win_button_size = strtol(optarg, &p, 10);

				if (*p)
				{
					error("invalid button size\n");
					return 1;
				}

				break;

			case 'T':
				STRNCPY(This->title, optarg, sizeof(This->title));
				break;

			case 'N':
				This->numlock_sync = True;
				break;

			case 'X':
				This->embed_wnd = strtol(optarg, NULL, 0);
				break;

			case 'a':
				This->server_depth = strtol(optarg, NULL, 10);
				if (This->server_depth != 8 &&
				    This->server_depth != 16 &&
				    This->server_depth != 15 && This->server_depth != 24)
				{
					error("Invalid server colour depth.\n");
					return 1;
				}
				break;

			case 'z':
				DEBUG(("rdp compression enabled\n"));
				flags |= (RDP_LOGON_COMPRESSION | RDP_LOGON_COMPRESSION2);
				break;

			case 'x':
				if (str_startswith(optarg, "m"))	/* modem */
				{
					This->rdp5_performanceflags =
						RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG |
						RDP5_NO_MENUANIMATIONS | RDP5_NO_THEMING;
				}
				else if (str_startswith(optarg, "b"))	/* broadband */
				{
					This->rdp5_performanceflags = RDP5_NO_WALLPAPER;
				}
				else if (str_startswith(optarg, "l"))	/* lan */
				{
					This->rdp5_performanceflags = RDP5_DISABLE_NOTHING;
				}
				else
				{
					This->rdp5_performanceflags = strtol(optarg, NULL, 16);
				}
				break;

			case 'P':
				This->bitmap_cache_persist_enable = True;
				break;

			case 'r':

				if (str_startswith(optarg, "sound"))
				{
					optarg += 5;

					if (*optarg == ':')
					{
						optarg++;
						while ((p = next_arg(optarg, ',')))
						{
							if (str_startswith(optarg, "remote"))
								flags |= RDP_LOGON_LEAVE_AUDIO;

							if (str_startswith(optarg, "local"))
#ifdef WITH_RDPSND
								This->rdpsnd_enabled = True;
#else
								warning("Not compiled with sound support\n");
#endif

							if (str_startswith(optarg, "off"))
#ifdef WITH_RDPSND
								This->rdpsnd_enabled = False;
#else
								warning("Not compiled with sound support\n");
#endif

							optarg = p;
						}
					}
					else
					{
#ifdef WITH_RDPSND
						This->rdpsnd_enabled = True;
#else
						warning("Not compiled with sound support\n");
#endif
					}
				}
				else if (str_startswith(optarg, "disk"))
				{
					/* -r disk:h:=/mnt/floppy */
					disk_enum_devices(This, &This->num_devices, optarg + 4);
				}
				else if (str_startswith(optarg, "comport"))
				{
					serial_enum_devices(This, &This->num_devices, optarg + 7);
				}
				else if (str_startswith(optarg, "lspci"))
				{
					This->lspci_enabled = True;
				}
				else if (str_startswith(optarg, "lptport"))
				{
					parallel_enum_devices(This, &This->num_devices, optarg + 7);
				}
				else if (str_startswith(optarg, "printer"))
				{
					printer_enum_devices(This, &This->num_devices, optarg + 7);
				}
				else if (str_startswith(optarg, "clientname"))
				{
					This->rdpdr_clientname = xmalloc(strlen(optarg + 11) + 1);
					strcpy(This->rdpdr_clientname, optarg + 11);
				}
				else if (str_startswith(optarg, "clipboard"))
				{
					optarg += 9;

					if (*optarg == ':')
					{
						optarg++;

						if (str_startswith(optarg, "off"))
							This->rdpclip = False;
						else
							cliprdr_set_mode(This, optarg);
					}
					else
						This->rdpclip = True;
				}
				else
				{
					warning("Unknown -r argument\n\n\tPossible arguments are: comport, disk, lptport, printer, sound, clipboard\n");
				}
				break;

			case '0':
				This->console_session = True;
				break;

			case '4':
				This->use_rdp5 = False;
				break;

			case '5':
				This->use_rdp5 = True;
				break;

			case 'h':
			case '?':
			default:
				usage(argv[0]);
				return 1;
		}
	}

	if (argc - optind != 1)
	{
		usage(argv[0]);
		return 1;
	}

	STRNCPY(server, argv[optind], sizeof(server));
	parse_server_and_port(This, server);

	if (This->seamless_rdp)
	{
		if (This->win_button_size)
		{
			error("You cannot use -S and -A at the same time\n");
			return 1;
		}
		This->rdp5_performanceflags &= ~RDP5_NO_FULLWINDOWDRAG;
		if (geometry_option)
		{
			error("You cannot use -g and -A at the same time\n");
			return 1;
		}
		if (This->fullscreen)
		{
			error("You cannot use -f and -A at the same time\n");
			return 1;
		}
		if (This->hide_decorations)
		{
			error("You cannot use -D and -A at the same time\n");
			return 1;
		}
		if (This->embed_wnd)
		{
			error("You cannot use -X and -A at the same time\n");
			return 1;
		}
		if (!This->use_rdp5)
		{
			error("You cannot use -4 and -A at the same time\n");
			return 1;
		}
		This->width = -100;
		This->grab_keyboard = False;
	}

	if (!username_option)
	{
		pw = getpwuid(getuid());
		if ((pw == NULL) || (pw->pw_name == NULL))
		{
			error("could not determine username, use -u\n");
			return 1;
		}

		STRNCPY(This->username, pw->pw_name, sizeof(This->username));
	}

#ifdef HAVE_ICONV
	if (This->codepage[0] == 0)
	{
		if (setlocale(LC_CTYPE, ""))
		{
			STRNCPY(This->codepage, nl_langinfo(CODESET), sizeof(This->codepage));
		}
		else
		{
			STRNCPY(This->codepage, DEFAULT_CODEPAGE, sizeof(This->codepage));
		}
	}
#endif

	if (This->hostname[0] == 0)
	{
		if (gethostname(fullhostname, sizeof(fullhostname)) == -1)
		{
			error("could not determine local hostname, use -n\n");
			return 1;
		}

		p = strchr(fullhostname, '.');
		if (p != NULL)
			*p = 0;

		STRNCPY(This->hostname, fullhostname, sizeof(This->hostname));
	}

	if (This->keymapname[0] == 0)
	{
		if (locale && xkeymap_from_locale(This, locale))
		{
			fprintf(stderr, "Autoselected keyboard map %s\n", This->keymapname);
		}
		else
		{
			STRNCPY(This->keymapname, "en-us", sizeof(This->keymapname));
		}
	}
	if (locale)
		xfree(locale);


	if (prompt_password && read_password(password, sizeof(password)))
		flags |= RDP_LOGON_AUTO;

	if (This->title[0] == 0)
	{
		strcpy(This->title, "rdesktop - ");
		strncat(This->title, server, sizeof(This->title) - sizeof("rdesktop - "));
	}

#ifdef RDP2VNC
	rdp2vnc_connect(server, flags, domain, password, shell, directory);
	return 0;
#else

	if (!ui_init(This))
		return 1;

#ifdef WITH_RDPSND
	if (This->rdpsnd_enabled)
		rdpsnd_init(This);
#endif

	if (This->lspci_enabled)
		lspci_init(This);

	rdpdr_init(This);

	while (run_count < 2 && continue_connect)	/* add support for Session Directory; only reconnect once */
	{
		if (run_count == 0)
		{
			if (!rdp_connect(This, server, flags, domain, password, shell, directory))
				return 1;
		}
		else if (!rdp_reconnect
			 (This, server, flags, domain, password, shell, directory, This->redirect_cookie))
			return 1;

		/* By setting encryption to False here, we have an encrypted login
		   packet but unencrypted transfer of other packets */
		if (!This->packet_encryption)
			This->encryption = False;


		DEBUG(("Connection successful.\n"));
		memset(password, 0, sizeof(password));

		if (run_count == 0)
			if (!ui_create_window(This))
				continue_connect = False;

		if (continue_connect)
			rdp_main_loop(This, &deactivated, &ext_disc_reason);

		DEBUG(("Disconnecting...\n"));
		rdp_disconnect(This);

		if ((This->redirect == True) && (run_count == 0))	/* Support for Session Directory */
		{
			/* reset state of major globals */
			rdesktop_reset_state(This);

			STRNCPY(domain, This->redirect_domain, sizeof(domain));
			STRNCPY(This->username, This->redirect_username, sizeof(This->username));
			STRNCPY(password, This->redirect_password, sizeof(password));
			STRNCPY(server, This->redirect_server, sizeof(server));
			flags |= RDP_LOGON_AUTO;

			This->redirect = False;
		}
		else
		{
			continue_connect = False;
			ui_destroy_window(This);
			break;
		}

		run_count++;
	}

	cache_save_state(This);
	ui_deinit(This);

	if (ext_disc_reason >= 2)
		print_disconnect_reason(ext_disc_reason);

	if (deactivated)
	{
		/* clean disconnect */
		return 0;
	}
	else
	{
		if (ext_disc_reason == exDiscReasonAPIInitiatedDisconnect
		    || ext_disc_reason == exDiscReasonAPIInitiatedLogoff)
		{
			/* not so clean disconnect, but nothing to worry about */
			return 0;
		}
		else
		{
			/* return error */
			return 2;
		}
	}

#endif

}

#ifdef EGD_SOCKET
/* Read 32 random bytes from PRNGD or EGD socket (based on OpenSSL RAND_egd) */
static BOOL
generate_random_egd(uint8 * buf)
{
	struct sockaddr_un addr;
	BOOL ret = False;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
		return False;

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, EGD_SOCKET, sizeof(EGD_SOCKET));
	if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		goto err;

	/* PRNGD and EGD use a simple communications protocol */
	buf[0] = 1;		/* Non-blocking (similar to /dev/urandom) */
	buf[1] = 32;		/* Number of requested random bytes */
	if (write(fd, buf, 2) != 2)
		goto err;

	if ((read(fd, buf, 1) != 1) || (buf[0] == 0))	/* Available? */
		goto err;

	if (read(fd, buf, 32) != 32)
		goto err;

	ret = True;

      err:
	close(fd);
	return ret;
}
#endif

/* Generate a 32-byte random for the secure transport code. */
void
generate_random(uint8 * random)
{
	struct stat st;
	struct tms tmsbuf;
	MD5_CTX md5;
	uint32 *r;
	int fd, n;

	/* If we have a kernel random device, try that first */
	if (((fd = open("/dev/urandom", O_RDONLY)) != -1)
	    || ((fd = open("/dev/random", O_RDONLY)) != -1))
	{
		n = read(fd, random, 32);
		close(fd);
		if (n == 32)
			return;
	}

#ifdef EGD_SOCKET
	/* As a second preference use an EGD */
	if (generate_random_egd(random))
		return;
#endif

	/* Otherwise use whatever entropy we can gather - ideas welcome. */
	r = (uint32 *) random;
	r[0] = (getpid()) | (getppid() << 16);
	r[1] = (getuid()) | (getgid() << 16);
	r[2] = times(&tmsbuf);	/* system uptime (clocks) */
	gettimeofday((struct timeval *) &r[3], NULL);	/* sec and usec */
	stat("/tmp", &st);
	r[5] = st.st_atime;
	r[6] = st.st_mtime;
	r[7] = st.st_ctime;

	/* Hash both halves with MD5 to obscure possible patterns */
	MD5_Init(&md5);
	MD5_Update(&md5, random, 16);
	MD5_Final(random, &md5);
	MD5_Update(&md5, random + 16, 16);
	MD5_Final(random + 16, &md5);
}

/* malloc; exit if out of memory */
void *
xmalloc(int size)
{
	void *mem = malloc(size);
	if (mem == NULL)
	{
		error("xmalloc %d\n", size);
		exit(1);
	}
	return mem;
}

/* strdup */
char *
xstrdup(const char *s)
{
	char *mem = strdup(s);
	if (mem == NULL)
	{
		perror("strdup");
		exit(1);
	}
	return mem;
}

/* realloc; exit if out of memory */
void *
xrealloc(void *oldmem, int size)
{
	void *mem;

	if (size < 1)
		size = 1;
	mem = realloc(oldmem, size);
	if (mem == NULL)
	{
		error("xrealloc %d\n", size);
		exit(1);
	}
	return mem;
}

/* free */
void
xfree(void *mem)
{
	free(mem);
}

/* report an error */
void
error(char *format, ...)
{
	va_list ap;

	fprintf(stderr, "ERROR: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

/* report a warning */
void
warning(char *format, ...)
{
	va_list ap;

	fprintf(stderr, "WARNING: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

/* report an unimplemented protocol feature */
void
unimpl(char *format, ...)
{
	va_list ap;

	fprintf(stderr, "NOT IMPLEMENTED: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

/* produce a hex dump */
void
hexdump(unsigned char *p, unsigned int len)
{
	unsigned char *line = p;
	int i, thisline, offset = 0;

	while (offset < len)
	{
		printf("%04x ", offset);
		thisline = len - offset;
		if (thisline > 16)
			thisline = 16;

		for (i = 0; i < thisline; i++)
			printf("%02x ", line[i]);

		for (; i < 16; i++)
			printf("   ");

		for (i = 0; i < thisline; i++)
			printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');

		printf("\n");
		offset += thisline;
		line += thisline;
	}
}

/*
  input: src is the string we look in for needle.
  	 Needle may be escaped by a backslash, in
	 that case we ignore that particular needle.
  return value: returns next src pointer, for
  	succesive executions, like in a while loop
	if retval is 0, then there are no more args.
  pitfalls:
  	src is modified. 0x00 chars are inserted to
	terminate strings.
	return val, points on the next val chr after ins
	0x00

	example usage:
	while( (pos = next_arg( optarg, ',')) ){
		printf("%s\n",optarg);
		optarg=pos;
	}

*/
char *
next_arg(char *src, char needle)
{
	char *nextval;
	char *p;
	char *mvp = 0;

	/* EOS */
	if (*src == (char) 0x00)
		return 0;

	p = src;
	/*  skip escaped needles */
	while ((nextval = strchr(p, needle)))
	{
		mvp = nextval - 1;
		/* found backslashed needle */
		if (*mvp == '\\' && (mvp > src))
		{
			/* move string one to the left */
			while (*(mvp + 1) != (char) 0x00)
			{
				*mvp = *(mvp + 1);
				mvp++;
			}
			*mvp = (char) 0x00;
			p = nextval;
		}
		else
		{
			p = nextval + 1;
			break;
		}

	}

	/* more args available */
	if (nextval)
	{
		*nextval = (char) 0x00;
		return ++nextval;
	}

	/* no more args after this, jump to EOS */
	nextval = src + strlen(src);
	return nextval;
}


void
toupper_str(char *p)
{
	while (*p)
	{
		if ((*p >= 'a') && (*p <= 'z'))
			*p = toupper((int) *p);
		p++;
	}
}


BOOL
str_startswith(const char *s, const char *prefix)
{
	return (strncmp(s, prefix, strlen(prefix)) == 0);
}


/* Split input into lines, and call linehandler for each
   line. Incomplete lines are saved in the rest variable, which should
   initially point to NULL. When linehandler returns False, stop and
   return False. Otherwise, return True.  */
BOOL
str_handle_lines(RDPCLIENT * This, const char *input, char **rest, str_handle_lines_t linehandler, void *data)
{
	char *buf, *p;
	char *oldrest;
	size_t inputlen;
	size_t buflen;
	size_t restlen = 0;
	BOOL ret = True;

	/* Copy data to buffer */
	inputlen = strlen(input);
	if (*rest)
		restlen = strlen(*rest);
	buflen = restlen + inputlen + 1;
	buf = (char *) xmalloc(buflen);
	buf[0] = '\0';
	if (*rest)
		STRNCPY(buf, *rest, buflen);
	strncat(buf, input, inputlen);
	p = buf;

	while (1)
	{
		char *newline = strchr(p, '\n');
		if (newline)
		{
			*newline = '\0';
			if (!linehandler(This, p, data))
			{
				p = newline + 1;
				ret = False;
				break;
			}
			p = newline + 1;
		}
		else
		{
			break;

		}
	}

	/* Save in rest */
	oldrest = *rest;
	restlen = buf + buflen - p;
	*rest = (char *) xmalloc(restlen);
	STRNCPY((*rest), p, restlen);
	xfree(oldrest);

	xfree(buf);
	return ret;
}

/* Execute the program specified by argv. For each line in
   stdout/stderr output, call linehandler. Returns false on failure. */
BOOL
subprocess(RDPCLIENT * This, char *const argv[], str_handle_lines_t linehandler, void *data)
{
	pid_t child;
	int fd[2];
	int n = 1;
	char output[256];
	char *rest = NULL;

	if (pipe(fd) < 0)
	{
		perror("pipe");
		return False;
	}

	if ((child = fork()) < 0)
	{
		perror("fork");
		return False;
	}

	/* Child */
	if (child == 0)
	{
		/* Close read end */
		close(fd[0]);

		/* Redirect stdout and stderr to pipe */
		dup2(fd[1], 1);
		dup2(fd[1], 2);

		/* Execute */
		execvp(argv[0], argv);
		perror("Error executing child");
		_exit(128);
	}

	/* Parent. Close write end. */
	close(fd[1]);
	while (n > 0)
	{
		n = read(fd[0], output, 255);
		output[n] = '\0';
		str_handle_lines(This, output, &rest, linehandler, data);
	}
	xfree(rest);

	return True;
}


/* not all clibs got ltoa */
#define LTOA_BUFSIZE (sizeof(long) * 8 + 1)

char *
l_to_a(long N, int base)
{
	static char ret[LTOA_BUFSIZE];

	char *head = ret, buf[LTOA_BUFSIZE], *tail = buf + sizeof(buf);

	register int divrem;

	if (base < 36 || 2 > base)
		base = 10;

	if (N < 0)
	{
		*head++ = '-';
		N = -N;
	}

	tail = buf + sizeof(buf);
	*--tail = 0;

	do
	{
		divrem = N % base;
		*--tail = (divrem <= 9) ? divrem + '0' : divrem + 'a' - 10;
		N /= base;
	}
	while (N);

	strcpy(head, tail);
	return ret;
}


int
load_licence(RDPCLIENT * This, unsigned char **data)
{
	char *home, *path;
	struct stat st;
	int fd, length;

	home = getenv("HOME");
	if (home == NULL)
		return -1;

	path = (char *) xmalloc(strlen(home) + strlen(This->hostname) + sizeof("/.rdesktop/licence."));
	sprintf(path, "%s/.rdesktop/licence.%s", home, This->hostname);

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -1;

	if (fstat(fd, &st))
		return -1;

	*data = (uint8 *) xmalloc(st.st_size);
	length = read(fd, *data, st.st_size);
	close(fd);
	xfree(path);
	return length;
}

void
save_licence(RDPCLIENT * This, unsigned char *data, int length)
{
	char *home, *path, *tmppath;
	int fd;

	home = getenv("HOME");
	if (home == NULL)
		return;

	path = (char *) xmalloc(strlen(home) + strlen(This->hostname) + sizeof("/.rdesktop/licence."));

	sprintf(path, "%s/.rdesktop", home);
	if ((mkdir(path, 0700) == -1) && errno != EEXIST)
	{
		perror(path);
		return;
	}

	/* write licence to licence.hostname.new, then atomically rename to licence.hostname */

	sprintf(path, "%s/.rdesktop/licence.%s", home, This->hostname);
	tmppath = (char *) xmalloc(strlen(path) + sizeof(".new"));
	strcpy(tmppath, path);
	strcat(tmppath, ".new");

	fd = open(tmppath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd == -1)
	{
		perror(tmppath);
		return;
	}

	if (write(fd, data, length) != length)
	{
		perror(tmppath);
		unlink(tmppath);
	}
	else if (rename(tmppath, path) == -1)
	{
		perror(path);
		unlink(tmppath);
	}

	close(fd);
	xfree(tmppath);
	xfree(path);
}

/* Create the bitmap cache directory */
BOOL
rd_pstcache_mkdir(void)
{
	char *home;
	char bmpcache_dir[256];

	home = getenv("HOME");

	if (home == NULL)
		return False;

	sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop");

	if ((mkdir(bmpcache_dir, S_IRWXU) == -1) && errno != EEXIST)
	{
		perror(bmpcache_dir);
		return False;
	}

	sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop/cache");

	if ((mkdir(bmpcache_dir, S_IRWXU) == -1) && errno != EEXIST)
	{
		perror(bmpcache_dir);
		return False;
	}

	return True;
}

/* open a file in the .rdesktop directory */
int
rd_open_file(char *filename)
{
	char *home;
	char fn[256];
	int fd;

	home = getenv("HOME");
	if (home == NULL)
		return -1;
	sprintf(fn, "%s/.rdesktop/%s", home, filename);
	fd = open(fn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd == -1)
		perror(fn);
	return fd;
}

/* close file */
void
rd_close_file(int fd)
{
	close(fd);
}

/* read from file*/
int
rd_read_file(int fd, void *ptr, int len)
{
	return read(fd, ptr, len);
}

/* write to file */
int
rd_write_file(int fd, void *ptr, int len)
{
	return write(fd, ptr, len);
}

/* move file pointer */
int
rd_lseek_file(int fd, int offset)
{
	return lseek(fd, offset, SEEK_SET);
}

/* do a write lock on a file */
BOOL
rd_lock_file(int fd, int start, int len)
{
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = start;
	lock.l_len = len;
	if (fcntl(fd, F_SETLK, &lock) == -1)
		return False;
	return True;
}
