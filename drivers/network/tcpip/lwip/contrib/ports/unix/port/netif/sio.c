/* Author: Magnus Ivarsson <magnus.ivarsson@volvo.com> */

/* to get rid of implicit function declarations */
#define _XOPEN_SOURCE 600
#define _GNU_SOURCE

/* build with Darwin C extensions not part of POSIX, i.e. FASYNC, SIGIO.
   we can't use LWIP_UNIX_MACH because extensions need to be turned
   on before any system headers (which are pulled in through cc.h)
   are included */
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif

#include "netif/sio.h"
#include "netif/fifo.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/arch.h"
#include "lwip/sio.h"
#include "netif/ppp/ppp_opts.h"

/* Following #undefs are here to keep compiler from issuing warnings
   about them being double defined. (They are defined in lwip/inet.h
   as well as the Unix #includes below.) */
#undef htonl
#undef ntohl
#undef htons
#undef ntohs
#undef HTONL
#undef NTOHL
#undef HTONS
#undef NTOHS

#include <stdlib.h>
#include <stdio.h>
#if defined(LWIP_UNIX_OPENBSD)
#include <util.h>
#endif
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>

#ifndef LWIP_HAVE_SLIPIF
#define LWIP_HAVE_SLIPIF 0
#endif

#if (PPP_SUPPORT || LWIP_HAVE_SLIPIF) && defined(LWIP_UNIX_LINUX)
#include <pty.h>
#endif

/*#define BAUDRATE B19200 */
/*#define BAUDRATE B57600 */
#define BAUDRATE B115200

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* for all of you who don't define SIO_DEBUG in debug.h */
#ifndef SIO_DEBUG
#define SIO_DEBUG 0
#endif


/*  typedef struct siostruct_t */
/*  {  */
/*  	sio_status_t *sio; */
/*  } siostruct_t; */

/** array of ((siostruct*)netif->state)->sio structs */
static sio_status_t statusar[4];

#if ! (PPP_SUPPORT || LWIP_HAVE_SLIPIF)
/* --private-functions----------------------------------------------------------------- */
/**
 * Signal handler for ttyXX0 to indicate bytes received
 * one per interface is needed since we cannot send a instance number / pointer as callback argument (?)
 */
static void	signal_handler_IO_0( int status )
{
	LWIP_UNUSED_ARG(status);
	LWIP_DEBUGF(SIO_DEBUG, ("SigHand: rxSignal channel 0\n"));
	fifoPut( &statusar[0].myfifo, statusar[0].fd );
}

/**
 * Signal handler for ttyXX1 to indicate bytes received
 * one per interface is needed since we cannot send a instance number / pointer as callback argument (?)
 */
static void signal_handler_IO_1( int status )
{
	LWIP_UNUSED_ARG(status);
	LWIP_DEBUGF(SIO_DEBUG, ("SigHand: rxSignal channel 1\n"));
	fifoPut( &statusar[1].myfifo, statusar[1].fd );
}
#endif /* ! (PPP_SUPPORT || LWIP_HAVE_SLIPIF) */

/**
* Initiation of serial device
* @param device string with the device name and path, eg. "/dev/ttyS0"
* @param devnum device number
* @param siostat status
* @return file handle to serial dev.
*/
static int sio_init( char * device, int devnum, sio_status_t * siostat )
{
	struct termios oldtio,newtio;
#if ! (PPP_SUPPORT || LWIP_HAVE_SLIPIF)
	struct sigaction saio;           /* definition of signal action */
#endif
	int fd;
	LWIP_UNUSED_ARG(siostat);
	LWIP_UNUSED_ARG(devnum);

	/* open the device to be non-blocking (read will return immediately) */
	fd = open( device, O_RDWR | O_NOCTTY | O_NONBLOCK );
	if ( fd < 0 )
	{
		perror( device );
		exit( -1 );
	}

#if ! (PPP_SUPPORT || LWIP_HAVE_SLIPIF)
	memset(&saio, 0, sizeof(struct sigaction));
	/* install the signal handler before making the device asynchronous */
	switch ( devnum )
	{
		case 0:
			LWIP_DEBUGF( SIO_DEBUG, ("sioinit, signal_handler_IO_0\n") );
			saio.sa_handler = signal_handler_IO_0;
			break;
		case 1:
			LWIP_DEBUGF( SIO_DEBUG, ("sioinit, signal_handler_IO_1\n") );
			saio.sa_handler = signal_handler_IO_1;
			break;
		default:
			LWIP_DEBUGF( SIO_DEBUG,("sioinit, devnum not allowed\n") );
			break;
	}

	sigaction( SIGIO,&saio,NULL );

	/* allow the process to receive SIGIO */
       	if ( fcntl( fd, F_SETOWN, getpid( ) ) != 0)
	{
		perror( device );
		exit( -1 );
	}
	/* Make the file descriptor asynchronous (the manual page says only
	O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
       	if ( fcntl( fd, F_SETFL, FASYNC ) != 0)
	{
		perror( device );
		exit( -1 );
	}
#else
       	if ( fcntl( fd, F_SETFL, 0 ) != 0)
	{
		perror( device );
		exit( -1 );
	}

#endif /* ! (PPP_SUPPORT || LWIP_HAVE_SLIPIF) */

	tcgetattr( fd,&oldtio ); /* save current port settings */
	/* set new port settings */
	/* see 'man termios' for further settings */
        memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD | CRTSCTS;
	newtio.c_iflag = 0;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0; /*ECHO; */
	newtio.c_cc[VMIN] = 1; /* Read 1 byte at a time, no timer */
	newtio.c_cc[VTIME] = 0;

	tcsetattr( fd,TCSANOW,&newtio );
	tcflush( fd, TCIOFLUSH );

	return fd;
}

/**
*
*/
static void sio_speed( int fd, int speed )
{
	struct termios oldtio,newtio;
	/*  int fd; */

	LWIP_DEBUGF(SIO_DEBUG, ("sio_speed[%d]: baudcode:%d enter\n", fd, speed));

	if ( fd < 0 )
	{
		LWIP_DEBUGF(SIO_DEBUG, ("sio_speed[%d]: fd ERROR\n", fd));
		exit( -1 );
	}

	tcgetattr( fd,&oldtio ); /* get current port settings */

	/* set new port settings
	* see 'man termios' for further settings */
        memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = speed | CS8 | CLOCAL | CREAD; /* | CRTSCTS; */
	newtio.c_iflag = 0;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0; /*ECHO; */
	newtio.c_cc[VMIN] = 1; /* Read 1 byte at a time, no timer */
	newtio.c_cc[VTIME] = 0;

	tcsetattr( fd,TCSANOW,&newtio );
	tcflush( fd, TCIOFLUSH );

	LWIP_DEBUGF(SIO_DEBUG, ("sio_speed[%d]: leave\n", fd));
}

/* --public-functions----------------------------------------------------------------------------- */
void sio_send( u8_t c, sio_status_t * siostat )
{
    /*	sio_status_t * siostat = ((siostruct_t*)netif->state)->sio; */

	if ( write( siostat->fd, &c, 1 ) <= 0 )
	{
		LWIP_DEBUGF(SIO_DEBUG, ("sio_send[%d]: write refused\n", siostat->fd));
	}
}

void sio_send_string( u8_t *str, sio_status_t * siostat )
{
    /*	sio_status_t * siostat = ((siostruct_t*)netif->state)->sio; */
	int len = strlen( (const char *)str );

	if ( write( siostat->fd, str, len ) <= 0 )
	{
		LWIP_DEBUGF(SIO_DEBUG, ("sio_send_string[%d]: write refused\n", siostat->fd));
	}
	LWIP_DEBUGF(SIO_DEBUG, ("sio_send_string[%d]: sent: %s\n", siostat->fd, str));
}


void sio_flush( sio_status_t * siostat )
{
	LWIP_UNUSED_ARG(siostat);
	/* not implemented in unix as it is not needed */
 	/*sio_status_t * siostat = ((siostruct_t*)netif->state)->sio; */
}


#if ! (PPP_SUPPORT || LWIP_HAVE_SLIPIF)
/*u8_t sio_recv( struct netif * netif )*/
u8_t sio_recv( sio_status_t * siostat )
{
    /*	sio_status_t * siostat = ((siostruct_t*)netif->state)->sio; */
	return fifoGet( &(siostat->myfifo) );
}

s16_t sio_poll(sio_status_t * siostat)
{
    /*	sio_status_t * siostat = ((siostruct_t*)netif->state)->sio;*/
	return fifoGetNonBlock( &(siostat->myfifo) );
}


void sio_expect_string( u8_t *str, sio_status_t * siostat )
{
    /*	sio_status_t * siostat = ((siostruct_t*)netif->state)->sio;*/
	u8_t c;
 	int finger=0;

	LWIP_DEBUGF(SIO_DEBUG, ("sio_expect_string[%d]: %s\n", siostat->fd, str));
	while ( 1 )
	{
		c=fifoGet( &(siostat->myfifo) );
		LWIP_DEBUGF(SIO_DEBUG, ("_%c", c));
		if ( c==str[finger] )
		{
			finger++;
		} else if ( finger > 0 )
		{
                    /*it might fit in the beginning? */
			if ( str[0] == c )
			{
				finger = 1;
			}
		}
		if ( 0 == str[finger] )
                    break;	/* done, we have a match */
	}
	LWIP_DEBUGF(SIO_DEBUG, ("sio_expect_string[%d]: [match]\n", siostat->fd));
}
#endif /* ! (PPP_SUPPORT || LWIP_HAVE_SLIPIF) */

#if (PPP_SUPPORT || LWIP_HAVE_SLIPIF)
u32_t sio_write(sio_status_t * siostat, const u8_t *buf, u32_t size)
{
    ssize_t wsz = write( siostat->fd, buf, size );
    return wsz < 0 ? 0 : wsz;
}

u32_t sio_read(sio_status_t * siostat, u8_t *buf, u32_t size)
{
    ssize_t rsz = read( siostat->fd, buf, size );
    return rsz < 0 ? 0 : rsz;
}

void sio_read_abort(sio_status_t * siostat)
{
    LWIP_UNUSED_ARG(siostat);
    printf("sio_read_abort[%d]: not yet implemented for unix\n", siostat->fd);
}
#endif /* (PPP_SUPPORT || LWIP_HAVE_SLIPIF) */

sio_fd_t sio_open(u8_t devnum)
{
	char dev[20];

	/* would be nice with dynamic memory alloc */
	sio_status_t * siostate = &statusar[ devnum ];
/* 	siostruct_t * tmp; */


/* 	tmp = (siostruct_t*)(netif->state); */
/* 	tmp->sio = siostate; */

/* 	tmp = (siostruct_t*)(netif->state); */

/* 	((sio_status_t*)(tmp->sio))->fd = 0; */

	LWIP_DEBUGF(SIO_DEBUG, ("sio_open: for devnum %d\n", devnum));

#if ! (PPP_SUPPORT || LWIP_HAVE_SLIPIF)
	fifoInit( &siostate->myfifo );
#endif /* ! PPP_SUPPORT */

	snprintf( dev, sizeof(dev), "/dev/ttyS%d", devnum );

	if ( (devnum == 1) || (devnum == 0) )
	{
		if ( ( siostate->fd = sio_init( dev, devnum, siostate ) ) == 0 )
		{
			LWIP_DEBUGF(SIO_DEBUG, ("sio_open: ERROR opening serial device dev=%s\n", dev));
			abort( );
			return NULL;
		}
		LWIP_DEBUGF(SIO_DEBUG, ("sio_open[%d]: dev=%s open.\n", siostate->fd, dev));
	}
#if PPP_SUPPORT
	else if (devnum == 2) {
	    pid_t childpid;
	    char name[256];
	    childpid = forkpty(&siostate->fd, name, NULL, NULL);
	    if(childpid < 0) {
		perror("forkpty");
		exit (1);
	    }
	    if(childpid == 0) {
		execl("/usr/sbin/pppd", "pppd",
			"ms-dns", "198.168.100.7",
			"local", "crtscts",
			"debug",
#ifdef LWIP_PPP_CHAP_TEST
			"auth",
			"require-chap",
			"remotename", "lwip",
#else
			"noauth",
#endif
#if LWIP_IPV6
			"+ipv6",
#endif
			"192.168.1.1:192.168.1.2",
			NULL);
		perror("execl pppd");
		exit (1);
	    } else {
		LWIP_DEBUGF(SIO_DEBUG, ("sio_open[%d]: spawned pppd pid %d on %s\n",
			siostate->fd, childpid, name));
	    }

	}
#endif
#if LWIP_HAVE_SLIPIF
	else if (devnum == 3) {
	    pid_t childpid;
	    /* create PTY pair */
	    siostate->fd = posix_openpt(O_RDWR | O_NOCTTY);
	    if (siostate->fd < 0) {
		perror("open pty master");
		exit (1);
	    }
	    if (grantpt(siostate->fd) != 0) {
		perror("grant pty master");
		exit (1);
	    }
	    if (unlockpt(siostate->fd) != 0) {
		perror("unlock pty master");
		exit (1);
	    }
	    LWIP_DEBUGF(SIO_DEBUG, ("sio_open[%d]: for %s\n",
		    siostate->fd, ptsname(siostate->fd)));
	    /* fork for slattach */
	    childpid = fork();
	    if(childpid < 0) {
		perror("fork");
		exit (1);
	    }
	    if(childpid == 0) {
		/* esteblish SLIP interface on host side connected to PTY slave */
		execl("/sbin/slattach", "slattach",
			"-d", "-v", "-L", "-p", "slip",
			ptsname(siostate->fd),
			NULL);
		perror("execl slattach");
		exit (1);
	    } else {
		int ret;
		char buf[1024];
		LWIP_DEBUGF(SIO_DEBUG, ("sio_open[%d]: spawned slattach pid %d on %s\n",
			siostate->fd, childpid, ptsname(siostate->fd)));
		/* wait a moment for slattach startup */
		sleep(1);
		/* configure SLIP interface on host side as P2P interface */
		snprintf(buf, sizeof(buf),
			"/sbin/ifconfig sl0 mtu %d %s pointopoint %s up",
			SLIP_MAX_SIZE, "192.168.2.1", "192.168.2.2");
		LWIP_DEBUGF(SIO_DEBUG, ("sio_open[%d]: system(\"%s\");\n", siostate->fd, buf));
		ret = system(buf);
		if (ret < 0) {
		    perror("ifconfig failed");
		    exit(1);
		}
	    }
	}
#endif /* LWIP_HAVE_SLIPIF */
	else
	{
		LWIP_DEBUGF(SIO_DEBUG, ("sio_open: device %s (%d) is not supported\n", dev, devnum));
		return NULL;
	}

	return siostate;
}

/**
*
*/
void sio_change_baud( sioBaudrates baud, sio_status_t * siostat )
{
    /*	sio_status_t * siostat = ((siostruct_t*)netif->state)->sio;*/

	LWIP_DEBUGF(SIO_DEBUG, ("sio_change_baud[%d]\n", siostat->fd));

	switch ( baud )
	{
		case SIO_BAUD_9600:
			sio_speed( siostat->fd, B9600 );
			break;
		case SIO_BAUD_19200:
			sio_speed( siostat->fd, B19200 );
			break;
		case SIO_BAUD_38400:
			sio_speed( siostat->fd, B38400 );
			break;
		case SIO_BAUD_57600:
			sio_speed( siostat->fd, B57600 );
			break;
		case SIO_BAUD_115200:
			sio_speed( siostat->fd, B115200 );
			break;

		default:
			LWIP_DEBUGF(SIO_DEBUG, ("sio_change_baud[%d]: Unknown baudrate, code:%d\n",
					siostat->fd, baud));
			break;
	}
}
