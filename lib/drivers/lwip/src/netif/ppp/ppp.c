/*****************************************************************************
* ppp.c - Network Point to Point Protocol program file.
*
* Copyright (c) 2003 by Marc Boucher, Services Informatiques (MBSI) inc.
* portions Copyright (c) 1997 by Global Election Systems Inc.
*
* The authors hereby grant permission to use, copy, modify, distribute,
* and license this software and its documentation for any purpose, provided
* that existing copyright notices are retained in all copies and that this
* notice and the following disclaimer are included verbatim in any 
* distributions. No written agreement, license, or royalty fee is required
* for any of the authorized uses.
*
* THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS *AS IS* AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
* IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
* REVISION HISTORY
*
* 03-01-01 Marc Boucher <marc@mbsi.ca>
*   Ported to lwIP.
* 97-11-05 Guy Lancaster <lancasterg@acm.org>, Global Election Systems Inc.
*   Original.
*****************************************************************************/

/*
 * ppp_defs.h - PPP definitions.
 *
 * if_pppvar.h - private structures and declarations for PPP.
 *
 * Copyright (c) 1994 The Australian National University.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, provided that the above copyright
 * notice appears in all copies.  This software is provided without any
 * warranty, express or implied. The Australian National University
 * makes no representations about the suitability of this software for
 * any purpose.
 *
 * IN NO EVENT SHALL THE AUSTRALIAN NATIONAL UNIVERSITY BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
 * THE AUSTRALIAN NATIONAL UNIVERSITY HAVE BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * THE AUSTRALIAN NATIONAL UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE AUSTRALIAN NATIONAL UNIVERSITY HAS NO
 * OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS,
 * OR MODIFICATIONS.
 */

/*
 * if_ppp.h - Point-to-Point Protocol definitions.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "lwip/opt.h"

#if PPP_SUPPORT /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip.h" /* for ip_input() */

#include "ppp.h"
#include "pppdebug.h"

#include "randm.h"
#include "fsm.h"
#if PAP_SUPPORT
#include "pap.h"
#endif /* PAP_SUPPORT */
#if CHAP_SUPPORT
#include "chap.h"
#endif /* CHAP_SUPPORT */
#include "ipcp.h"
#include "lcp.h"
#include "magic.h"
#include "auth.h"
#if VJ_SUPPORT
#include "vj.h"
#endif /* VJ_SUPPORT */
#if PPPOE_SUPPORT
#include "netif/ppp_oe.h"
#endif /* PPPOE_SUPPORT */

#include <string.h>

/*************************/
/*** LOCAL DEFINITIONS ***/
/*************************/

/*
 * The basic PPP frame.
 */
#define PPP_ADDRESS(p)  (((u_char *)(p))[0])
#define PPP_CONTROL(p)  (((u_char *)(p))[1])
#define PPP_PROTOCOL(p) ((((u_char *)(p))[2] << 8) + ((u_char *)(p))[3])

/* PPP packet parser states.  Current state indicates operation yet to be
 * completed. */
typedef enum {
  PDIDLE = 0,  /* Idle state - waiting. */
  PDSTART,     /* Process start flag. */
  PDADDRESS,   /* Process address field. */
  PDCONTROL,   /* Process control field. */
  PDPROTOCOL1, /* Process protocol field 1. */
  PDPROTOCOL2, /* Process protocol field 2. */
  PDDATA       /* Process data byte. */
} PPPDevStates;

#define ESCAPE_P(accm, c) ((accm)[(c) >> 3] & pppACCMMask[c & 0x07])

/************************/
/*** LOCAL DATA TYPES ***/
/************************/
/*
 * PPP interface control block.
 */
typedef struct PPPControl_s {
  char openFlag;                /* True when in use. */
#if PPPOE_SUPPORT
  struct netif *ethif;
  struct pppoe_softc *pppoe_sc;
#endif /* PPPOE_SUPPORT */
  int  if_up;                   /* True when the interface is up. */
  int  errCode;                 /* Code indicating why interface is down. */
#if PPPOS_SUPPORT
  sio_fd_t fd;                  /* File device ID of port. */
  int  kill_link;               /* Shut the link down. */
  int  sig_hup;                 /* Carrier lost. */
  struct pbuf *inHead, *inTail; /* The input packet. */
  PPPDevStates inState;         /* The input process state. */
  char inEscaped;               /* Escape next character. */
  u16_t inProtocol;             /* The input protocol code. */
  u16_t inFCS;                  /* Input Frame Check Sequence value. */
#endif /* PPPOS_SUPPORT */
  int  mtu;                     /* Peer's mru */
  int  pcomp;                   /* Does peer accept protocol compression? */
  int  accomp;                  /* Does peer accept addr/ctl compression? */
  u_long lastXMit;              /* Time of last transmission. */
  ext_accm inACCM;              /* Async-Ctl-Char-Map for input. */
  ext_accm outACCM;             /* Async-Ctl-Char-Map for output. */
#if PPPOS_SUPPORT && VJ_SUPPORT
  int  vjEnabled;               /* Flag indicating VJ compression enabled. */
  struct vjcompress vjComp;     /* Van Jacobson compression header. */
#endif /* PPPOS_SUPPORT && VJ_SUPPORT */

  struct netif netif;

  struct ppp_addrs addrs;

  void (*linkStatusCB)(void *ctx, int errCode, void *arg);
  void *linkStatusCtx;

} PPPControl;


/*
 * Ioctl definitions.
 */

struct npioctl {
  int         protocol; /* PPP procotol, e.g. PPP_IP */
  enum NPmode mode;
};



/***********************************/
/*** LOCAL FUNCTION DECLARATIONS ***/
/***********************************/
#if PPPOS_SUPPORT
static void pppMain(void *pd);
static void pppDrop(PPPControl *pc);
static void pppInProc(int pd, u_char *s, int l);
#endif /* PPPOS_SUPPORT */


/******************************/
/*** PUBLIC DATA STRUCTURES ***/
/******************************/
u_long subnetMask;

static PPPControl pppControl[NUM_PPP]; /* The PPP interface control blocks. */

/*
 * PPP Data Link Layer "protocol" table.
 * One entry per supported protocol.
 * The last entry must be NULL.
 */
struct protent *ppp_protocols[] = {
  &lcp_protent,
#if PAP_SUPPORT
  &pap_protent,
#endif /* PAP_SUPPORT */
#if CHAP_SUPPORT
  &chap_protent,
#endif /* CHAP_SUPPORT */
#if CBCP_SUPPORT
  &cbcp_protent,
#endif /* CBCP_SUPPORT */
  &ipcp_protent,
#if CCP_SUPPORT
  &ccp_protent,
#endif /* CCP_SUPPORT */
  NULL
};


/*
 * Buffers for outgoing packets.  This must be accessed only from the appropriate
 * PPP task so that it doesn't need to be protected to avoid collisions.
 */
u_char outpacket_buf[NUM_PPP][PPP_MRU+PPP_HDRLEN];


/*****************************/
/*** LOCAL DATA STRUCTURES ***/
/*****************************/

#if PPPOS_SUPPORT
/*
 * FCS lookup table as calculated by genfcstab.
 */
static const u_short fcstab[256] = {
  0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
  0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
  0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
  0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
  0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
  0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
  0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
  0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
  0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
  0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
  0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
  0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
  0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
  0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
  0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
  0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
  0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
  0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
  0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
  0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
  0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
  0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
  0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
  0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
  0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
  0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
  0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
  0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
  0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
  0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
  0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
  0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/* PPP's Asynchronous-Control-Character-Map.  The mask array is used
 * to select the specific bit for a character. */
static u_char pppACCMMask[] = {
  0x01,
  0x02,
  0x04,
  0x08,
  0x10,
  0x20,
  0x40,
  0x80
};


void
pppMainWakeup(int pd)
{
  PPPDEBUG((LOG_DEBUG, "pppMainWakeup: unit %d\n", pd));
  sio_read_abort(pppControl[pd].fd);
}
#endif /* PPPOS_SUPPORT */

void
pppLinkTerminated(int pd)
{
  PPPDEBUG((LOG_DEBUG, "pppLinkTerminated: unit %d\n", pd));

#if PPPOE_SUPPORT
  if(pppControl[pd].ethif) {
    pppoe_disconnect(pppControl[pd].pppoe_sc);
  } else
#endif /* PPPOE_SUPPORT */
  {
#if PPPOS_SUPPORT
    pppMainWakeup(pd);
#endif /* PPPOS_SUPPORT */
  }
}

void
pppLinkDown(int pd)
{
  PPPDEBUG((LOG_DEBUG, "pppLinkDown: unit %d\n", pd));

#if PPPOE_SUPPORT
  if(pppControl[pd].ethif) {
    pppoe_disconnect(pppControl[pd].pppoe_sc);
  } else
#endif /* PPPOE_SUPPORT */
  {
#if PPPOS_SUPPORT
    pppMainWakeup(pd);
#endif /* PPPOS_SUPPORT */
  }
}

/* these callbacks are necessary because lcp_* functions
   must be called in the same context as pppInput(),
   namely the tcpip_thread(), essentially because
   they manipulate timeouts which are thread-private
*/

static void
pppStartCB(void *arg)
{
  int pd = (int)arg;

  PPPDEBUG((LOG_DEBUG, "pppStartCB: unit %d\n", pd));
  lcp_lowerup(pd);
  lcp_open(pd); /* Start protocol */
}

static void
pppStopCB(void *arg)
{
  int pd = (int)arg;

  PPPDEBUG((LOG_DEBUG, "pppStopCB: unit %d\n", pd));
  lcp_close(pd, "User request");
}

static void
pppHupCB(void *arg)
{
  int pd = (int)arg;

  PPPDEBUG((LOG_DEBUG, "pppHupCB: unit %d\n", pd));
  lcp_lowerdown(pd);
  link_terminated(pd);
}

/***********************************/
/*** PUBLIC FUNCTION DEFINITIONS ***/
/***********************************/
/* Initialize the PPP subsystem. */

struct ppp_settings ppp_settings;

void
pppInit(void)
{
  struct protent *protp;
  int i, j;

  memset(&ppp_settings, 0, sizeof(ppp_settings));
  ppp_settings.usepeerdns = 1;
  pppSetAuth(PPPAUTHTYPE_NONE, NULL, NULL);

  magicInit();

  subnetMask = htonl(0xffffff00);

  for (i = 0; i < NUM_PPP; i++) {
    pppControl[i].openFlag = 0;

    /*
     * Initialize to the standard option set.
     */
    for (j = 0; (protp = ppp_protocols[j]) != NULL; ++j) {
      (*protp->init)(i);
    }
  }

#if PPPOE_SUPPORT
  pppoe_init();
#endif /* PPPOE_SUPPORT */
}

void
pppSetAuth(enum pppAuthType authType, const char *user, const char *passwd)
{
  switch(authType) {
    case PPPAUTHTYPE_NONE:
    default:
#ifdef LWIP_PPP_STRICT_PAP_REJECT
      ppp_settings.refuse_pap = 1;
#else  /* LWIP_PPP_STRICT_PAP_REJECT */
      /* some providers request pap and accept an empty login/pw */
      ppp_settings.refuse_pap = 0;
#endif /* LWIP_PPP_STRICT_PAP_REJECT */
      ppp_settings.refuse_chap = 1;
      break;

    case PPPAUTHTYPE_ANY:
      /* Warning: Using PPPAUTHTYPE_ANY might have security consequences.
       * RFC 1994 says:
       *
       * In practice, within or associated with each PPP server, there is a
       * database which associates "user" names with authentication
       * information ("secrets").  It is not anticipated that a particular
       * named user would be authenticated by multiple methods.  This would
       * make the user vulnerable to attacks which negotiate the least secure
       * method from among a set (such as PAP rather than CHAP).  If the same
       * secret was used, PAP would reveal the secret to be used later with
       * CHAP.
       *
       * Instead, for each user name there should be an indication of exactly
       * one method used to authenticate that user name.  If a user needs to
       * make use of different authentication methods under different
       * circumstances, then distinct user names SHOULD be employed, each of
       * which identifies exactly one authentication method.
       *
       */
      ppp_settings.refuse_pap = 0;
      ppp_settings.refuse_chap = 0;
      break;

    case PPPAUTHTYPE_PAP:
      ppp_settings.refuse_pap = 0;
      ppp_settings.refuse_chap = 1;
      break;

    case PPPAUTHTYPE_CHAP:
      ppp_settings.refuse_pap = 1;
      ppp_settings.refuse_chap = 0;
      break;
  }

  if(user) {
    strncpy(ppp_settings.user, user, sizeof(ppp_settings.user)-1);
    ppp_settings.user[sizeof(ppp_settings.user)-1] = '\0';
  } else {
    ppp_settings.user[0] = '\0';
  }

  if(passwd) {
    strncpy(ppp_settings.passwd, passwd, sizeof(ppp_settings.passwd)-1);
    ppp_settings.passwd[sizeof(ppp_settings.passwd)-1] = '\0';
  } else {
    ppp_settings.passwd[0] = '\0';
  }
}

#if PPPOS_SUPPORT
/* Open a new PPP connection using the given I/O device.
 * This initializes the PPP control block but does not
 * attempt to negotiate the LCP session.  If this port
 * connects to a modem, the modem connection must be
 * established before calling this.
 * Return a new PPP connection descriptor on success or
 * an error code (negative) on failure. */
int
pppOverSerialOpen(sio_fd_t fd, void (*linkStatusCB)(void *ctx, int errCode, void *arg), void *linkStatusCtx)
{
  PPPControl *pc;
  int pd;

  /* Find a free PPP session descriptor. Critical region? */
  for (pd = 0; pd < NUM_PPP && pppControl[pd].openFlag != 0; pd++);

  if (pd >= NUM_PPP) {
    pd = PPPERR_OPEN;
  } else {
    pppControl[pd].openFlag = !0;
  }

  /* Launch a deamon thread. */
  if (pd >= 0) {
    pppControl[pd].openFlag = 1;

    lcp_init(pd);
    pc = &pppControl[pd];
    pc->fd = fd;
#if PPPOE_SUPPORT
    pc->ethif= NULL;
#endif /* PPPOE_SUPPORT */
    pc->kill_link = 0;
    pc->sig_hup = 0;
    pc->if_up = 0;
    pc->errCode = 0;
    pc->inState = PDIDLE;
    pc->inHead = NULL;
    pc->inTail = NULL;
    pc->inEscaped = 0;
    pc->lastXMit = 0;

#if VJ_SUPPORT
    pc->vjEnabled = 0;
    vj_compress_init(&pc->vjComp);
#endif /* VJ_SUPPORT */

    /* 
     * Default the in and out accm so that escape and flag characters
     * are always escaped. 
     */
    memset(pc->inACCM, 0, sizeof(ext_accm));
    pc->inACCM[15] = 0x60;
    memset(pc->outACCM, 0, sizeof(ext_accm));
    pc->outACCM[15] = 0x60;

    pc->linkStatusCB = linkStatusCB;
    pc->linkStatusCtx = linkStatusCtx;

    sys_thread_new(PPP_THREAD_NAME, pppMain, (void*)pd, PPP_THREAD_STACKSIZE, PPP_THREAD_PRIO);
    if(!linkStatusCB) {
      while(pd >= 0 && !pc->if_up) {
        sys_msleep(500);
        if (lcp_phase[pd] == PHASE_DEAD) {
          pppClose(pd);
          if (pc->errCode) {
            pd = pc->errCode;
          } else {
            pd = PPPERR_CONNECT;
          }
        }
      }
    }
  }

  return pd;
}
#endif /* PPPOS_SUPPORT */

#if PPPOE_SUPPORT
static void pppOverEthernetLinkStatusCB(int pd, int up);

void
pppOverEthernetClose(int pd)
{
  PPPControl* pc = &pppControl[pd];

  /* *TJL* There's no lcp_deinit */
  lcp_close(pd, NULL);

  pppoe_destroy(&pc->netif);
}

int pppOverEthernetOpen(struct netif *ethif, const char *service_name, const char *concentrator_name, void (*linkStatusCB)(void *ctx, int errCode, void *arg), void *linkStatusCtx)
{
  PPPControl *pc;
  int pd;

  LWIP_UNUSED_ARG(service_name);
  LWIP_UNUSED_ARG(concentrator_name);

  /* Find a free PPP session descriptor. Critical region? */
  for (pd = 0; pd < NUM_PPP && pppControl[pd].openFlag != 0; pd++);
  if (pd >= NUM_PPP) {
    pd = PPPERR_OPEN;
  } else {
    pppControl[pd].openFlag = !0;
  }

  /* PPP session descriptor found, start PPPoE */
  if (pd >= 0) {

    pppControl[pd].openFlag = 1;

    lcp_init(pd);

    lcp_wantoptions[pd].mru = PPPOE_MAXMTU;
    lcp_wantoptions[pd].neg_asyncmap = 0;
    lcp_wantoptions[pd].neg_pcompression = 0;
    lcp_wantoptions[pd].neg_accompression = 0;

    lcp_allowoptions[pd].mru = PPPOE_MAXMTU;
    lcp_allowoptions[pd].neg_asyncmap = 0;
    lcp_allowoptions[pd].neg_pcompression = 0;
    lcp_allowoptions[pd].neg_accompression = 0;

    pc = &pppControl[pd];
    pc->if_up = 0;
    pc->errCode = 0;
    pc->lastXMit = 0;
#if PPPOS_SUPPORT
    pc->kill_link = 0;
    pc->sig_hup = 0;
    pc->inState = PDIDLE;
    pc->inHead = NULL;
    pc->inTail = NULL;
    pc->inEscaped = 0;
#if VJ_SUPPORT
    pc->vjEnabled = 0;
#endif /* VJ_SUPPORT */
#endif /* PPPOS_SUPPORT */
    pc->ethif= ethif;

    memset(pc->inACCM,  0, sizeof(ext_accm));
    memset(pc->outACCM, 0, sizeof(ext_accm));

    pc->linkStatusCB  = linkStatusCB;
    pc->linkStatusCtx = linkStatusCtx;

    if(pppoe_create(ethif, pd, pppOverEthernetLinkStatusCB, &pc->pppoe_sc) != ERR_OK) {
      pc->openFlag = 0;
      return PPPERR_OPEN;
    }

    pppoe_connect(pc->pppoe_sc);

    if(!linkStatusCB) {
      while(pd >= 0 && !pc->if_up) {
        sys_msleep(500);
        if (lcp_phase[pd] == PHASE_DEAD) {
          pppClose(pd);
          if (pc->errCode) {
            pd = pc->errCode;
          } else {
            pd = PPPERR_CONNECT;
          }
        }
      }
    }
  }

  return pd;
}
#endif /* PPPOE_SUPPORT */


/* Close a PPP connection and release the descriptor. 
 * Any outstanding packets in the queues are dropped.
 * Return 0 on success, an error code on failure. */
int
pppClose(int pd)
{
  PPPControl *pc = &pppControl[pd];
  int st = 0;

  /* Disconnect */
#if PPPOE_SUPPORT
  if(pc->ethif) {
    PPPDEBUG((LOG_DEBUG, "pppClose: unit %d kill_link -> pppStopCB\n", pd));
    pc->errCode = PPPERR_USER;
    /* This will leave us at PHASE_DEAD. */
    tcpip_callback(pppStopCB, (void*)pd);
  } else
#endif /* PPPOE_SUPPORT */
  {
#if PPPOS_SUPPORT
    pc->kill_link = !0;
    pppMainWakeup(pd);
#endif /* PPPOS_SUPPORT */
  }

  if(!pc->linkStatusCB) {
    while(st >= 0 && lcp_phase[pd] != PHASE_DEAD) {
      sys_msleep(500);
      break;
    }
  }

  return st;
}

/* This function is called when carrier is lost on the PPP channel. */
void
pppSigHUP(int pd)
{
  PPPControl *pc = &pppControl[pd];

#if PPPOE_SUPPORT
  if(pc->ethif) {
    PPPDEBUG((LOG_DEBUG, "pppSigHUP: unit %d sig_hup -> pppHupCB\n", pd));
    tcpip_callback(pppHupCB, (void*)pd);
  } else
#endif /* PPPOE_SUPPORT */
  {
#if PPPOS_SUPPORT
    pc->sig_hup = 1;
    pppMainWakeup(pd);
#endif /* PPPOS_SUPPORT */
  }
}

#if PPPOS_SUPPORT
static void
nPut(PPPControl *pc, struct pbuf *nb)
{
  struct pbuf *b;
  int c;

  for(b = nb; b != NULL; b = b->next) {
    if((c = sio_write(pc->fd, b->payload, b->len)) != b->len) {
      PPPDEBUG((LOG_WARNING,
               "PPP nPut: incomplete sio_write(%d,, %u) = %d\n", pc->fd, b->len, c));
      LINK_STATS_INC(link.err);
      pc->lastXMit = 0; /* prepend PPP_FLAG to next packet */
      break;
    }
  }

  pbuf_free(nb);
  LINK_STATS_INC(link.xmit);
}

/* 
 * pppAppend - append given character to end of given pbuf.  If outACCM
 * is not NULL and the character needs to be escaped, do so.
 * If pbuf is full, append another.
 * Return the current pbuf.
 */
static struct pbuf *
pppAppend(u_char c, struct pbuf *nb, ext_accm *outACCM)
{
  struct pbuf *tb = nb;
  
  /* Make sure there is room for the character and an escape code.
   * Sure we don't quite fill the buffer if the character doesn't
   * get escaped but is one character worth complicating this? */
  /* Note: We assume no packet header. */
  if (nb && (PBUF_POOL_BUFSIZE - nb->len) < 2) {
    tb = pbuf_alloc(PBUF_RAW, 0, PBUF_POOL);
    if (tb) {
      nb->next = tb;
    } else {
      LINK_STATS_INC(link.memerr);
    }
    nb = tb;
  }

  if (nb) {
    if (outACCM && ESCAPE_P(*outACCM, c)) {
      *((u_char*)nb->payload + nb->len++) = PPP_ESCAPE;
      *((u_char*)nb->payload + nb->len++) = c ^ PPP_TRANS;
    } else {
      *((u_char*)nb->payload + nb->len++) = c;
    }
  }

  return tb;
}
#endif /* PPPOS_SUPPORT */

#if PPPOE_SUPPORT
static err_t
pppifOutputOverEthernet(int pd, struct pbuf *p)
{
  PPPControl *pc = &pppControl[pd];
  struct pbuf *pb;
  u_short protocol = PPP_IP;
  int i=0;

  pb = pbuf_alloc(PBUF_LINK, pppoe_hdrlen + sizeof(protocol), PBUF_RAM);
  if(!pb) {
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.proterr);
    return ERR_MEM;
  }

  pbuf_header(pb, -pppoe_hdrlen);

  pc->lastXMit = sys_jiffies();

  if (!pc->pcomp || protocol > 0xFF) {
    *((u_char*)pb->payload + i++) = (protocol >> 8) & 0xFF;
  }
  *((u_char*)pb->payload + i) = protocol & 0xFF;

  pbuf_chain(pb, p);

  if(pppoe_xmit(pc->pppoe_sc, pb) != ERR_OK) {
    LINK_STATS_INC(link.err);
    return PPPERR_DEVICE;
  }

  LINK_STATS_INC(link.xmit);
  return ERR_OK;
}
#endif /* PPPOE_SUPPORT */

/* Send a packet on the given connection. */
static err_t
pppifOutput(struct netif *netif, struct pbuf *pb, struct ip_addr *ipaddr)
{
  int pd = (int)netif->state;
  PPPControl *pc = &pppControl[pd];
#if PPPOS_SUPPORT
  u_short protocol = PPP_IP;
  u_int fcsOut = PPP_INITFCS;
  struct pbuf *headMB = NULL, *tailMB = NULL, *p;
  u_char c;
#endif /* PPPOS_SUPPORT */

  LWIP_UNUSED_ARG(ipaddr);

  /* Validate parameters. */
  /* We let any protocol value go through - it can't hurt us
   * and the peer will just drop it if it's not accepting it. */
  if (pd < 0 || pd >= NUM_PPP || !pc->openFlag || !pb) {
    PPPDEBUG((LOG_WARNING, "pppifOutput[%d]: bad parms prot=%d pb=%p\n",
              pd, PPP_IP, pb));
    LINK_STATS_INC(link.opterr);
    LINK_STATS_INC(link.drop);
    return ERR_ARG;
  }

  /* Check that the link is up. */
  if (lcp_phase[pd] == PHASE_DEAD) {
    PPPDEBUG((LOG_ERR, "pppifOutput[%d]: link not up\n", pd));
    LINK_STATS_INC(link.rterr);
    LINK_STATS_INC(link.drop);
    return ERR_RTE;
  }

#if PPPOE_SUPPORT
  if(pc->ethif) {
    return pppifOutputOverEthernet(pd, pb);
  }
#endif /* PPPOE_SUPPORT */

#if PPPOS_SUPPORT
  /* Grab an output buffer. */
  headMB = pbuf_alloc(PBUF_RAW, 0, PBUF_POOL);
  if (headMB == NULL) {
    PPPDEBUG((LOG_WARNING, "pppifOutput[%d]: first alloc fail\n", pd));
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    return ERR_MEM;
  }

#if VJ_SUPPORT
  /* 
   * Attempt Van Jacobson header compression if VJ is configured and
   * this is an IP packet. 
   */
  if (protocol == PPP_IP && pc->vjEnabled) {
    switch (vj_compress_tcp(&pc->vjComp, pb)) {
      case TYPE_IP:
        /* No change...
           protocol = PPP_IP_PROTOCOL; */
        break;
      case TYPE_COMPRESSED_TCP:
        protocol = PPP_VJC_COMP;
        break;
      case TYPE_UNCOMPRESSED_TCP:
        protocol = PPP_VJC_UNCOMP;
        break;
      default:
        PPPDEBUG((LOG_WARNING, "pppifOutput[%d]: bad IP packet\n", pd));
        LINK_STATS_INC(link.proterr);
        LINK_STATS_INC(link.drop);
        pbuf_free(headMB);
        return ERR_VAL;
    }
  }
#endif /* VJ_SUPPORT */

  tailMB = headMB;

  /* Build the PPP header. */
  if ((sys_jiffies() - pc->lastXMit) >= PPP_MAXIDLEFLAG) {
    tailMB = pppAppend(PPP_FLAG, tailMB, NULL);
  }

  pc->lastXMit = sys_jiffies();
  if (!pc->accomp) {
    fcsOut = PPP_FCS(fcsOut, PPP_ALLSTATIONS);
    tailMB = pppAppend(PPP_ALLSTATIONS, tailMB, &pc->outACCM);
    fcsOut = PPP_FCS(fcsOut, PPP_UI);
    tailMB = pppAppend(PPP_UI, tailMB, &pc->outACCM);
  }
  if (!pc->pcomp || protocol > 0xFF) {
    c = (protocol >> 8) & 0xFF;
    fcsOut = PPP_FCS(fcsOut, c);
    tailMB = pppAppend(c, tailMB, &pc->outACCM);
  }
  c = protocol & 0xFF;
  fcsOut = PPP_FCS(fcsOut, c);
  tailMB = pppAppend(c, tailMB, &pc->outACCM);

  /* Load packet. */
  for(p = pb; p; p = p->next) {
    int n;
    u_char *sPtr;

    sPtr = (u_char*)p->payload;
    n = p->len;
    while (n-- > 0) {
      c = *sPtr++;

      /* Update FCS before checking for special characters. */
      fcsOut = PPP_FCS(fcsOut, c);
      
      /* Copy to output buffer escaping special characters. */
      tailMB = pppAppend(c, tailMB, &pc->outACCM);
    }
  }

  /* Add FCS and trailing flag. */
  c = ~fcsOut & 0xFF;
  tailMB = pppAppend(c, tailMB, &pc->outACCM);
  c = (~fcsOut >> 8) & 0xFF;
  tailMB = pppAppend(c, tailMB, &pc->outACCM);
  tailMB = pppAppend(PPP_FLAG, tailMB, NULL);

  /* If we failed to complete the packet, throw it away. */
  if (!tailMB) {
    PPPDEBUG((LOG_WARNING,
             "pppifOutput[%d]: Alloc err - dropping proto=%d\n", 
              pd, protocol));
    pbuf_free(headMB);
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    return ERR_MEM;
  }

  /* Send it. */
  PPPDEBUG((LOG_INFO, "pppifOutput[%d]: proto=0x%04X\n", pd, protocol));

  nPut(pc, headMB);
#endif /* PPPOS_SUPPORT */

  return ERR_OK;
}

/* Get and set parameters for the given connection.
 * Return 0 on success, an error code on failure. */
int
pppIOCtl(int pd, int cmd, void *arg)
{
  PPPControl *pc = &pppControl[pd];
  int st = 0;

  if (pd < 0 || pd >= NUM_PPP) {
    st = PPPERR_PARAM;
  } else {
    switch(cmd) {
    case PPPCTLG_UPSTATUS:      /* Get the PPP up status. */
      if (arg) {
        *(int *)arg = (int)(pc->if_up);
      } else {
        st = PPPERR_PARAM;
      }
      break;
    case PPPCTLS_ERRCODE:       /* Set the PPP error code. */
      if (arg) {
        pc->errCode = *(int *)arg;
      } else {
        st = PPPERR_PARAM;
      }
      break;
    case PPPCTLG_ERRCODE:       /* Get the PPP error code. */
      if (arg) {
        *(int *)arg = (int)(pc->errCode);
      } else {
        st = PPPERR_PARAM;
      }
      break;
#if PPPOS_SUPPORT
    case PPPCTLG_FD:
      if (arg) {
        *(sio_fd_t *)arg = pc->fd;
      } else {
        st = PPPERR_PARAM;
      }
      break;
#endif /* PPPOS_SUPPORT */
    default:
      st = PPPERR_PARAM;
      break;
    }
  }

  return st;
}

/*
 * Return the Maximum Transmission Unit for the given PPP connection.
 */
u_int
pppMTU(int pd)
{
  PPPControl *pc = &pppControl[pd];
  u_int st;

  /* Validate parameters. */
  if (pd < 0 || pd >= NUM_PPP || !pc->openFlag) {
    st = 0;
  } else {
    st = pc->mtu;
  }

  return st;
}

#if PPPOE_SUPPORT
int
pppWriteOverEthernet(int pd, const u_char *s, int n)
{
  PPPControl *pc = &pppControl[pd];
  struct pbuf *pb;

  /* skip address & flags */
  s += 2;
  n -= 2;

  pb = pbuf_alloc(PBUF_LINK, pppoe_hdrlen + n, PBUF_RAM);
  if(!pb) {
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.proterr);
    return PPPERR_ALLOC;
  }

  pbuf_header(pb, -pppoe_hdrlen);

  pc->lastXMit = sys_jiffies();

  MEMCPY(pb->payload, s, n);

  if(pppoe_xmit(pc->pppoe_sc, pb) != ERR_OK) {
    LINK_STATS_INC(link.err);
    return PPPERR_DEVICE;
  }

  LINK_STATS_INC(link.xmit);
  return PPPERR_NONE;
}
#endif /* PPPOE_SUPPORT */

/*
 * Write n characters to a ppp link.
 *  RETURN: >= 0 Number of characters written
 *           -1 Failed to write to device
 */
int
pppWrite(int pd, const u_char *s, int n)
{
  PPPControl *pc = &pppControl[pd];
#if PPPOS_SUPPORT
  u_char c;
  u_int fcsOut;
  struct pbuf *headMB, *tailMB;
#endif /* PPPOS_SUPPORT */

#if PPPOE_SUPPORT
  if(pc->ethif) {
    return pppWriteOverEthernet(pd, s, n);
  }
#endif /* PPPOE_SUPPORT */

#if PPPOS_SUPPORT
  headMB = pbuf_alloc(PBUF_RAW, 0, PBUF_POOL);
  if (headMB == NULL) {
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.proterr);
    return PPPERR_ALLOC;
  }

  tailMB = headMB;

  /* If the link has been idle, we'll send a fresh flag character to
   * flush any noise. */
  if ((sys_jiffies() - pc->lastXMit) >= PPP_MAXIDLEFLAG) {
    tailMB = pppAppend(PPP_FLAG, tailMB, NULL);
  }
  pc->lastXMit = sys_jiffies();

  fcsOut = PPP_INITFCS;
  /* Load output buffer. */
  while (n-- > 0) {
    c = *s++;

    /* Update FCS before checking for special characters. */
    fcsOut = PPP_FCS(fcsOut, c);

    /* Copy to output buffer escaping special characters. */
    tailMB = pppAppend(c, tailMB, &pc->outACCM);
  }
    
  /* Add FCS and trailing flag. */
  c = ~fcsOut & 0xFF;
  tailMB = pppAppend(c, tailMB, &pc->outACCM);
  c = (~fcsOut >> 8) & 0xFF;
  tailMB = pppAppend(c, tailMB, &pc->outACCM);
  tailMB = pppAppend(PPP_FLAG, tailMB, NULL);

  /* If we failed to complete the packet, throw it away.
   * Otherwise send it. */
  if (!tailMB) {
    PPPDEBUG((LOG_WARNING,
             "pppWrite[%d]: Alloc err - dropping pbuf len=%d\n", pd, headMB->len));
           /*"pppWrite[%d]: Alloc err - dropping %d:%.*H", pd, headMB->len, LWIP_MIN(headMB->len * 2, 40), headMB->payload)); */
    pbuf_free(headMB);
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.proterr);
    return PPPERR_ALLOC;
  }

  PPPDEBUG((LOG_INFO, "pppWrite[%d]: len=%d\n", pd, headMB->len));
                   /* "pppWrite[%d]: %d:%.*H", pd, headMB->len, LWIP_MIN(headMB->len * 2, 40), headMB->payload)); */
  nPut(pc, headMB);
#endif /* PPPOS_SUPPORT */

  return PPPERR_NONE;
}

/*
 * ppp_send_config - configure the transmit characteristics of
 * the ppp interface.
 */
void
ppp_send_config( int unit, int mtu, u32_t asyncmap, int pcomp, int accomp)
{
  PPPControl *pc = &pppControl[unit];
  int i;
  
  pc->mtu = mtu;
  pc->pcomp = pcomp;
  pc->accomp = accomp;
  
  /* Load the ACCM bits for the 32 control codes. */
  for (i = 0; i < 32/8; i++) {
    pc->outACCM[i] = (u_char)((asyncmap >> (8 * i)) & 0xFF);
  }
  PPPDEBUG((LOG_INFO, "ppp_send_config[%d]: outACCM=%X %X %X %X\n",
            unit,
            pc->outACCM[0], pc->outACCM[1], pc->outACCM[2], pc->outACCM[3]));
}


/*
 * ppp_set_xaccm - set the extended transmit ACCM for the interface.
 */
void
ppp_set_xaccm(int unit, ext_accm *accm)
{
  SMEMCPY(pppControl[unit].outACCM, accm, sizeof(ext_accm));
  PPPDEBUG((LOG_INFO, "ppp_set_xaccm[%d]: outACCM=%X %X %X %X\n",
            unit,
            pppControl[unit].outACCM[0],
            pppControl[unit].outACCM[1],
            pppControl[unit].outACCM[2],
            pppControl[unit].outACCM[3]));
}


/*
 * ppp_recv_config - configure the receive-side characteristics of
 * the ppp interface.
 */
void
ppp_recv_config( int unit, int mru, u32_t asyncmap, int pcomp, int accomp)
{
  PPPControl *pc = &pppControl[unit];
  int i;

  LWIP_UNUSED_ARG(accomp);
  LWIP_UNUSED_ARG(pcomp);
  LWIP_UNUSED_ARG(mru);

  /* Load the ACCM bits for the 32 control codes. */
  for (i = 0; i < 32 / 8; i++) {
    pc->inACCM[i] = (u_char)(asyncmap >> (i * 8));
  }
  PPPDEBUG((LOG_INFO, "ppp_recv_config[%d]: inACCM=%X %X %X %X\n",
            unit,
            pc->inACCM[0], pc->inACCM[1], pc->inACCM[2], pc->inACCM[3]));
}

#if 0
/*
 * ccp_test - ask kernel whether a given compression method
 * is acceptable for use.  Returns 1 if the method and parameters
 * are OK, 0 if the method is known but the parameters are not OK
 * (e.g. code size should be reduced), or -1 if the method is unknown.
 */
int
ccp_test( int unit, int opt_len,  int for_transmit, u_char *opt_ptr)
{
  return 0; /* XXX Currently no compression. */
}

/*
 * ccp_flags_set - inform kernel about the current state of CCP.
 */
void
ccp_flags_set(int unit, int isopen, int isup)
{
  /* XXX */
}

/*
 * ccp_fatal_error - returns 1 if decompression was disabled as a
 * result of an error detected after decompression of a packet,
 * 0 otherwise.  This is necessary because of patent nonsense.
 */
int
ccp_fatal_error(int unit)
{
  /* XXX */
  return 0;
}
#endif

/*
 * get_idle_time - return how long the link has been idle.
 */
int
get_idle_time(int u, struct ppp_idle *ip)
{
  /* XXX */
  LWIP_UNUSED_ARG(u);
  LWIP_UNUSED_ARG(ip);

  return 0;
}


/*
 * Return user specified netmask, modified by any mask we might determine
 * for address `addr' (in network byte order).
 * Here we scan through the system's list of interfaces, looking for
 * any non-point-to-point interfaces which might appear to be on the same
 * network as `addr'.  If we find any, we OR in their netmask to the
 * user-specified netmask.
 */
u32_t
GetMask(u32_t addr)
{
  u32_t mask, nmask;

  htonl(addr);
  if (IN_CLASSA(addr)) { /* determine network mask for address class */
    nmask = IN_CLASSA_NET;
  } else if (IN_CLASSB(addr)) {
    nmask = IN_CLASSB_NET;
  } else { 
    nmask = IN_CLASSC_NET;
  }

  /* class D nets are disallowed by bad_ip_adrs */
  mask = subnetMask | htonl(nmask);
  
  /* XXX
   * Scan through the system's network interfaces.
   * Get each netmask and OR them into our mask.
   */

  return mask;
}

/*
 * sifvjcomp - config tcp header compression
 */
int
sifvjcomp(int pd, int vjcomp, int cidcomp, int maxcid)
{
#if PPPOS_SUPPORT && VJ_SUPPORT
  PPPControl *pc = &pppControl[pd];
  
  pc->vjEnabled = vjcomp;
  pc->vjComp.compressSlot = cidcomp;
  pc->vjComp.maxSlotIndex = maxcid;
  PPPDEBUG((LOG_INFO, "sifvjcomp: VJ compress enable=%d slot=%d max slot=%d\n",
            vjcomp, cidcomp, maxcid));
#else /* PPPOS_SUPPORT && VJ_SUPPORT */
  LWIP_UNUSED_ARG(pd);
  LWIP_UNUSED_ARG(vjcomp);
  LWIP_UNUSED_ARG(cidcomp);
  LWIP_UNUSED_ARG(maxcid);
#endif /* PPPOS_SUPPORT && VJ_SUPPORT */

  return 0;
}

/*
 * pppifNetifInit - netif init callback
 */
static err_t
pppifNetifInit(struct netif *netif)
{
  netif->name[0] = 'p';
  netif->name[1] = 'p';
  netif->output = pppifOutput;
  netif->mtu = pppMTU((int)netif->state);
  return ERR_OK;
}


/*
 * sifup - Config the interface up and enable IP packets to pass.
 */
int
sifup(int pd)
{
  PPPControl *pc = &pppControl[pd];
  int st = 1;
  
  if (pd < 0 || pd >= NUM_PPP || !pc->openFlag) {
    st = 0;
    PPPDEBUG((LOG_WARNING, "sifup[%d]: bad parms\n", pd));
  } else {
    netif_remove(&pc->netif);
    if (netif_add(&pc->netif, &pc->addrs.our_ipaddr, &pc->addrs.netmask, &pc->addrs.his_ipaddr, (void *)pd, pppifNetifInit, ip_input)) {
      netif_set_up(&pc->netif);
      pc->if_up = 1;
      pc->errCode = PPPERR_NONE;

      PPPDEBUG((LOG_DEBUG, "sifup: unit %d: linkStatusCB=%lx errCode=%d\n", pd, pc->linkStatusCB, pc->errCode));
      if(pc->linkStatusCB) {
        pc->linkStatusCB(pc->linkStatusCtx, pc->errCode, &pc->addrs);
      }
    } else {
      st = 0;
      PPPDEBUG((LOG_ERR, "sifup[%d]: netif_add failed\n", pd));
    }
  }

  return st;
}

/*
 * sifnpmode - Set the mode for handling packets for a given NP.
 */
int
sifnpmode(int u, int proto, enum NPmode mode)
{
  LWIP_UNUSED_ARG(u);
  LWIP_UNUSED_ARG(proto);
  LWIP_UNUSED_ARG(mode);
  return 0;
}

/*
 * sifdown - Config the interface down and disable IP.
 */
int
sifdown(int pd)
{
  PPPControl *pc = &pppControl[pd];
  int st = 1;
  
  if (pd < 0 || pd >= NUM_PPP || !pc->openFlag) {
    st = 0;
    PPPDEBUG((LOG_WARNING, "sifdown[%d]: bad parms\n", pd));
  } else {
    pc->if_up = 0;
    /* make sure the netif status callback is called */
    netif_set_down(&pc->netif);
    netif_remove(&pc->netif);
    PPPDEBUG((LOG_DEBUG, "sifdown: unit %d: linkStatusCB=%lx errCode=%d\n", pd, pc->linkStatusCB, pc->errCode));
    if(pc->linkStatusCB) {
      pc->linkStatusCB(pc->linkStatusCtx, PPPERR_CONNECT, NULL);
    }
  }
  return st;
}

/**
 * sifaddr - Config the interface IP addresses and netmask.
 * @param pd Interface unit ???
 * @param o Our IP address ???
 * @param h His IP address ???
 * @param m IP subnet mask ???
 * @param ns1 Primary DNS
 * @param ns2 Secondary DNS
 */
int
sifaddr( int pd, u32_t o, u32_t h, u32_t m, u32_t ns1, u32_t ns2)
{
  PPPControl *pc = &pppControl[pd];
  int st = 1;
  
  if (pd < 0 || pd >= NUM_PPP || !pc->openFlag) {
    st = 0;
    PPPDEBUG((LOG_WARNING, "sifup[%d]: bad parms\n", pd));
  } else {
    SMEMCPY(&pc->addrs.our_ipaddr, &o, sizeof(o));
    SMEMCPY(&pc->addrs.his_ipaddr, &h, sizeof(h));
    SMEMCPY(&pc->addrs.netmask, &m, sizeof(m));
    SMEMCPY(&pc->addrs.dns1, &ns1, sizeof(ns1));
    SMEMCPY(&pc->addrs.dns2, &ns2, sizeof(ns2));
  }
  return st;
}

/**
 * cifaddr - Clear the interface IP addresses, and delete routes
 * through the interface if possible.
 * @param pd Interface unit ???
 * @param o Our IP address ???
 * @param h IP broadcast address ???
 */
int
cifaddr( int pd, u32_t o, u32_t h)
{
  PPPControl *pc = &pppControl[pd];
  int st = 1;
  
  LWIP_UNUSED_ARG(o);
  LWIP_UNUSED_ARG(h);
  if (pd < 0 || pd >= NUM_PPP || !pc->openFlag) {
    st = 0;
    PPPDEBUG((LOG_WARNING, "sifup[%d]: bad parms\n", pd));
  } else {
    IP4_ADDR(&pc->addrs.our_ipaddr, 0,0,0,0);
    IP4_ADDR(&pc->addrs.his_ipaddr, 0,0,0,0);
    IP4_ADDR(&pc->addrs.netmask, 255,255,255,0);
    IP4_ADDR(&pc->addrs.dns1, 0,0,0,0);
    IP4_ADDR(&pc->addrs.dns2, 0,0,0,0);
  }
  return st;
}

/*
 * sifdefaultroute - assign a default route through the address given.
 */
int
sifdefaultroute(int pd, u32_t l, u32_t g)
{
  PPPControl *pc = &pppControl[pd];
  int st = 1;

  LWIP_UNUSED_ARG(l);
  LWIP_UNUSED_ARG(g);

  if (pd < 0 || pd >= NUM_PPP || !pc->openFlag) {
    st = 0;
    PPPDEBUG((LOG_WARNING, "sifup[%d]: bad parms\n", pd));
  } else {
    netif_set_default(&pc->netif);
  }

  /* TODO: check how PPP handled the netMask, previously not set by ipSetDefault */

  return st;
}

/*
 * cifdefaultroute - delete a default route through the address given.
 */
int
cifdefaultroute(int pd, u32_t l, u32_t g)
{
  PPPControl *pc = &pppControl[pd];
  int st = 1;

  LWIP_UNUSED_ARG(l);
  LWIP_UNUSED_ARG(g);

  if (pd < 0 || pd >= NUM_PPP || !pc->openFlag) {
    st = 0;
    PPPDEBUG((LOG_WARNING, "sifup[%d]: bad parms\n", pd));
  } else {
    netif_set_default(NULL);
  }

  return st;
}

/**********************************/
/*** LOCAL FUNCTION DEFINITIONS ***/
/**********************************/

#if PPPOS_SUPPORT
/* The main PPP process function.  This implements the state machine according
 * to section 4 of RFC 1661: The Point-To-Point Protocol. */
static void
pppMain(void *arg)
{
  int pd = (int)arg;
  struct pbuf *p;
  PPPControl* pc;
  int c;

  pc = &pppControl[pd];

  p = pbuf_alloc(PBUF_RAW, PPP_MRU+PPP_HDRLEN, PBUF_RAM);
  if (!p) {
    LWIP_ASSERT("p != NULL", p);
    pc->errCode = PPPERR_ALLOC;
    goto out;
  }

  /*
   * Start the connection and handle incoming events (packet or timeout).
   */
  PPPDEBUG((LOG_INFO, "pppMain: unit %d: Connecting\n", pd));
  tcpip_callback(pppStartCB, arg);
  while (lcp_phase[pd] != PHASE_DEAD) {
    if (pc->kill_link) {
      PPPDEBUG((LOG_DEBUG, "pppMain: unit %d kill_link -> pppStopCB\n", pd));
      pc->errCode = PPPERR_USER;
      /* This will leave us at PHASE_DEAD. */
      tcpip_callback(pppStopCB, arg);
      pc->kill_link = 0;
    } else if (pc->sig_hup) {
      PPPDEBUG((LOG_DEBUG, "pppMain: unit %d sig_hup -> pppHupCB\n", pd));
      pc->sig_hup = 0;
      tcpip_callback(pppHupCB, arg);
    } else {
      c = sio_read(pc->fd, p->payload, p->len);
      if(c > 0) {
        pppInProc(pd, p->payload, c);
      } else {
        /* nothing received, give other tasks a chance to run */
        sys_msleep(1);
      }
    }
  }
  PPPDEBUG((LOG_INFO, "pppMain: unit %d: PHASE_DEAD\n", pd));
  pppDrop(pc); /* bug fix #17726 */
  pbuf_free(p);

out:
  PPPDEBUG((LOG_DEBUG, "pppMain: unit %d: linkStatusCB=%lx errCode=%d\n", pd, pc->linkStatusCB, pc->errCode));
  if(pc->linkStatusCB) {
    pc->linkStatusCB(pc->linkStatusCtx, pc->errCode ? pc->errCode : PPPERR_PROTOCOL, NULL);
  }

  pc->openFlag = 0;
}
#endif /* PPPOS_SUPPORT */

#if PPPOE_SUPPORT

void
pppOverEthernetInitFailed(void* arg)
{
  PPPControl* pc;
  int pd = (int)arg;

  pppHupCB(arg);
  pppStopCB(arg);

  pc = &pppControl[pd];
  pppoe_destroy(&pc->netif);
  pc->openFlag = 0;

  if(pc->linkStatusCB) {
    pc->linkStatusCB(pc->linkStatusCtx, pc->errCode ? pc->errCode : PPPERR_PROTOCOL, NULL);
  }
}

static void
pppOverEthernetLinkStatusCB(int pd, int up)
{
  if(up) {
    PPPDEBUG((LOG_INFO, "pppMain: unit %d: Connecting\n", pd));
    tcpip_callback(pppStartCB, (void*)pd);
  } else {
    PPPControl* pc;
    pc = &pppControl[pd];
    tcpip_callback(pppOverEthernetInitFailed, (void*)pd);
  }
}
#endif /* PPPOE_SUPPORT */

struct pbuf *
pppSingleBuf(struct pbuf *p)
{
  struct pbuf *q, *b;
  u_char *pl;

  if(p->tot_len == p->len) {
    return p;
  }

  q = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
  if(!q) {
    PPPDEBUG((LOG_ERR,
             "pppSingleBuf: unable to alloc new buf (%d)\n", p->tot_len));
    return p; /* live dangerously */
  }

  for(b = p, pl = q->payload; b != NULL; b = b->next) {
    MEMCPY(pl, b->payload, b->len);
    pl += b->len;
  }

  pbuf_free(p);

  return q;
}

struct pppInputHeader {
  int unit;
  u16_t proto;
};

/*
 * Pass the processed input packet to the appropriate handler.
 * This function and all handlers run in the context of the tcpip_thread
 */
static void
pppInput(void *arg)
{
  struct pbuf *nb = (struct pbuf *)arg;
  u16_t protocol;
  int pd;

  pd = ((struct pppInputHeader *)nb->payload)->unit;
  protocol = ((struct pppInputHeader *)nb->payload)->proto;
    
  if(pbuf_header(nb, -(int)sizeof(struct pppInputHeader))) {
    LWIP_ASSERT("pbuf_header failed\n", 0);
    goto drop;
  }

  LINK_STATS_INC(link.recv);

  /*
   * Toss all non-LCP packets unless LCP is OPEN.
   * Until we get past the authentication phase, toss all packets
   * except LCP, LQR and authentication packets.
   */
  if((lcp_phase[pd] <= PHASE_AUTHENTICATE) && (protocol != PPP_LCP)) {
    if(!((protocol == PPP_LQR) || (protocol == PPP_PAP) || (protocol == PPP_CHAP)) ||
        (lcp_phase[pd] != PHASE_AUTHENTICATE)) {
      PPPDEBUG((LOG_INFO, "pppInput: discarding proto 0x%04X in phase %d\n", protocol, lcp_phase[pd]));
      goto drop;
    }
  }

  switch(protocol) {
    case PPP_VJC_COMP:      /* VJ compressed TCP */
#if PPPOS_SUPPORT && VJ_SUPPORT
      PPPDEBUG((LOG_INFO, "pppInput[%d]: vj_comp in pbuf len=%d\n", pd, nb->len));
      /*
       * Clip off the VJ header and prepend the rebuilt TCP/IP header and
       * pass the result to IP.
       */
      if ((vj_uncompress_tcp(&nb, &pppControl[pd].vjComp) >= 0) && (pppControl[pd].netif.input)) {
        pppControl[pd].netif.input(nb, &pppControl[pd].netif);
        return;
      }
      /* Something's wrong so drop it. */
      PPPDEBUG((LOG_WARNING, "pppInput[%d]: Dropping VJ compressed\n", pd));
#else  /* PPPOS_SUPPORT && VJ_SUPPORT */
      /* No handler for this protocol so drop the packet. */
      PPPDEBUG((LOG_INFO, "pppInput[%d]: drop VJ Comp in %d:%s\n", pd, nb->len, nb->payload));
#endif /* PPPOS_SUPPORT && VJ_SUPPORT */
      break;

    case PPP_VJC_UNCOMP:    /* VJ uncompressed TCP */
#if PPPOS_SUPPORT && VJ_SUPPORT
      PPPDEBUG((LOG_INFO, "pppInput[%d]: vj_un in pbuf len=%d\n", pd, nb->len));
      /*
       * Process the TCP/IP header for VJ header compression and then pass
       * the packet to IP.
       */
      if ((vj_uncompress_uncomp(nb, &pppControl[pd].vjComp) >= 0) && pppControl[pd].netif.input) {
        pppControl[pd].netif.input(nb, &pppControl[pd].netif);
        return;
      }
      /* Something's wrong so drop it. */
      PPPDEBUG((LOG_WARNING, "pppInput[%d]: Dropping VJ uncompressed\n", pd));
#else  /* PPPOS_SUPPORT && VJ_SUPPORT */
      /* No handler for this protocol so drop the packet. */
      PPPDEBUG((LOG_INFO,
               "pppInput[%d]: drop VJ UnComp in %d:.*H\n", 
                pd, nb->len, LWIP_MIN(nb->len * 2, 40), nb->payload));
#endif /* PPPOS_SUPPORT && VJ_SUPPORT */
      break;

    case PPP_IP:            /* Internet Protocol */
      PPPDEBUG((LOG_INFO, "pppInput[%d]: ip in pbuf len=%d\n", pd, nb->len));
      if (pppControl[pd].netif.input) {
        pppControl[pd].netif.input(nb, &pppControl[pd].netif);
        return;
      }
      break;

    default: {
      struct protent *protp;
      int i;

      /*
       * Upcall the proper protocol input routine.
       */
      for (i = 0; (protp = ppp_protocols[i]) != NULL; ++i) {
        if (protp->protocol == protocol && protp->enabled_flag) {
          PPPDEBUG((LOG_INFO, "pppInput[%d]: %s len=%d\n", pd, protp->name, nb->len));
          nb = pppSingleBuf(nb);
          (*protp->input)(pd, nb->payload, nb->len);
          goto out;
        }
      }

      /* No handler for this protocol so reject the packet. */
      PPPDEBUG((LOG_INFO, "pppInput[%d]: rejecting unsupported proto 0x%04X len=%d\n", pd, protocol, nb->len));
      if (pbuf_header(nb, sizeof(protocol))) {
        LWIP_ASSERT("pbuf_header failed\n", 0);
        goto drop;
      }
#if BYTE_ORDER == LITTLE_ENDIAN
      protocol = htons(protocol);
      SMEMCPY(nb->payload, &protocol, sizeof(protocol));
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
      lcp_sprotrej(pd, nb->payload, nb->len);
    }
    break;
  }

drop:
  LINK_STATS_INC(link.drop);

out:
  pbuf_free(nb);
  return;
}

#if PPPOS_SUPPORT
/*
 * Drop the input packet.
 */
static void
pppDrop(PPPControl *pc)
{
  if (pc->inHead != NULL) {
#if 0
    PPPDEBUG((LOG_INFO, "pppDrop: %d:%.*H\n", pc->inHead->len, min(60, pc->inHead->len * 2), pc->inHead->payload));
#endif
    PPPDEBUG((LOG_INFO, "pppDrop: pbuf len=%d\n", pc->inHead->len));
    if (pc->inTail && (pc->inTail != pc->inHead)) {
      pbuf_free(pc->inTail);
    }
    pbuf_free(pc->inHead);
    pc->inHead = NULL;
    pc->inTail = NULL;
  }
#if VJ_SUPPORT
  vj_uncompress_err(&pc->vjComp);
#endif /* VJ_SUPPORT */

  LINK_STATS_INC(link.drop);
}

/**
 * Process a received octet string.
 */
static void
pppInProc(int pd, u_char *s, int l)
{
  PPPControl *pc = &pppControl[pd];
  struct pbuf *nextNBuf;
  u_char curChar;

  PPPDEBUG((LOG_DEBUG, "pppInProc[%d]: got %d bytes\n", pd, l));
  while (l-- > 0) {
    curChar = *s++;
    
    /* Handle special characters. */
    if (ESCAPE_P(pc->inACCM, curChar)) {
      /* Check for escape sequences. */
      /* XXX Note that this does not handle an escaped 0x5d character which
       * would appear as an escape character.  Since this is an ASCII ']'
       * and there is no reason that I know of to escape it, I won't complicate
       * the code to handle this case. GLL */
      if (curChar == PPP_ESCAPE) {
        pc->inEscaped = 1;
      /* Check for the flag character. */
      } else if (curChar == PPP_FLAG) {
         /* If this is just an extra flag character, ignore it. */
         if (pc->inState <= PDADDRESS) {
           /* ignore it */;
         /* If we haven't received the packet header, drop what has come in. */
         } else if (pc->inState < PDDATA) {
           PPPDEBUG((LOG_WARNING,
                    "pppInProc[%d]: Dropping incomplete packet %d\n", 
                     pd, pc->inState));
           LINK_STATS_INC(link.lenerr);
           pppDrop(pc);
         /* If the fcs is invalid, drop the packet. */
         } else if (pc->inFCS != PPP_GOODFCS) {
           PPPDEBUG((LOG_INFO,
                    "pppInProc[%d]: Dropping bad fcs 0x%04X proto=0x%04X\n", 
                     pd, pc->inFCS, pc->inProtocol));
           LINK_STATS_INC(link.chkerr);
           pppDrop(pc);
         /* Otherwise it's a good packet so pass it on. */
         } else {
           /* Trim off the checksum. */
           if(pc->inTail->len >= 2) {
             pc->inTail->len -= 2;

             pc->inTail->tot_len = pc->inTail->len;
             if (pc->inTail != pc->inHead) {
               pbuf_cat(pc->inHead, pc->inTail);
             }
           } else {
             pc->inTail->tot_len = pc->inTail->len;
             if (pc->inTail != pc->inHead) {
               pbuf_cat(pc->inHead, pc->inTail);
             }

             pbuf_realloc(pc->inHead, pc->inHead->tot_len - 2);
           }

           /* Dispatch the packet thereby consuming it. */
           if(tcpip_callback(pppInput, pc->inHead) != ERR_OK) {
             PPPDEBUG((LOG_ERR, "pppInProc[%d]: tcpip_callback() failed, dropping packet\n", pd));
             pbuf_free(pc->inHead);
             LINK_STATS_INC(link.drop);
           }
           pc->inHead = NULL;
           pc->inTail = NULL;
         }

         /* Prepare for a new packet. */
         pc->inFCS = PPP_INITFCS;
         pc->inState = PDADDRESS;
         pc->inEscaped = 0;
      /* Other characters are usually control characters that may have
       * been inserted by the physical layer so here we just drop them. */
      } else {
        PPPDEBUG((LOG_WARNING,
                 "pppInProc[%d]: Dropping ACCM char <%d>\n", pd, curChar));
      }
    /* Process other characters. */
    } else {
      /* Unencode escaped characters. */
      if (pc->inEscaped) {
        pc->inEscaped = 0;
        curChar ^= PPP_TRANS;
      }

      /* Process character relative to current state. */
      switch(pc->inState) {
        case PDIDLE:                    /* Idle state - waiting. */
          /* Drop the character if it's not 0xff
           * we would have processed a flag character above. */
          if (curChar != PPP_ALLSTATIONS) {
            break;
          }

        /* Fall through */
        case PDSTART:                   /* Process start flag. */
          /* Prepare for a new packet. */
          pc->inFCS = PPP_INITFCS;

        /* Fall through */
        case PDADDRESS:                 /* Process address field. */
          if (curChar == PPP_ALLSTATIONS) {
            pc->inState = PDCONTROL;
            break;
          }
          /* Else assume compressed address and control fields so
           * fall through to get the protocol... */
        case PDCONTROL:                 /* Process control field. */
          /* If we don't get a valid control code, restart. */
          if (curChar == PPP_UI) {
            pc->inState = PDPROTOCOL1;
            break;
          }
#if 0
          else {
            PPPDEBUG((LOG_WARNING,
                     "pppInProc[%d]: Invalid control <%d>\n", pd, curChar));
                      pc->inState = PDSTART;
          }
#endif
        case PDPROTOCOL1:               /* Process protocol field 1. */
          /* If the lower bit is set, this is the end of the protocol
           * field. */
          if (curChar & 1) {
            pc->inProtocol = curChar;
            pc->inState = PDDATA;
          } else {
            pc->inProtocol = (u_int)curChar << 8;
            pc->inState = PDPROTOCOL2;
          }
          break;
        case PDPROTOCOL2:               /* Process protocol field 2. */
          pc->inProtocol |= curChar;
          pc->inState = PDDATA;
          break;
        case PDDATA:                    /* Process data byte. */
          /* Make space to receive processed data. */
          if (pc->inTail == NULL || pc->inTail->len == PBUF_POOL_BUFSIZE) {
            if(pc->inTail) {
              pc->inTail->tot_len = pc->inTail->len;
              if (pc->inTail != pc->inHead) {
                pbuf_cat(pc->inHead, pc->inTail);
              }
            }
            /* If we haven't started a packet, we need a packet header. */
            nextNBuf = pbuf_alloc(PBUF_RAW, 0, PBUF_POOL);
            if (nextNBuf == NULL) {
              /* No free buffers.  Drop the input packet and let the
               * higher layers deal with it.  Continue processing
               * the received pbuf chain in case a new packet starts. */
              PPPDEBUG((LOG_ERR, "pppInProc[%d]: NO FREE MBUFS!\n", pd));
              LINK_STATS_INC(link.memerr);
              pppDrop(pc);
              pc->inState = PDSTART;  /* Wait for flag sequence. */
              break;
            }
            if (pc->inHead == NULL) {
              struct pppInputHeader *pih = nextNBuf->payload;

              pih->unit = pd;
              pih->proto = pc->inProtocol;

              nextNBuf->len += sizeof(*pih);

              pc->inHead = nextNBuf;
            }
            pc->inTail = nextNBuf;
          }
          /* Load character into buffer. */
          ((u_char*)pc->inTail->payload)[pc->inTail->len++] = curChar;
          break;
      }

      /* update the frame check sequence number. */
      pc->inFCS = PPP_FCS(pc->inFCS, curChar);
    }
  }

  avRandomize();
}
#endif /* PPPOS_SUPPORT */

#if PPPOE_SUPPORT
void
pppInProcOverEthernet(int pd, struct pbuf *pb)
{
  struct pppInputHeader *pih;
  u16_t inProtocol;

  if(pb->len < sizeof(inProtocol)) {
    PPPDEBUG((LOG_ERR, "pppInProcOverEthernet: too small for protocol field\n"));
    goto drop;
  }

  inProtocol = (((u8_t *)pb->payload)[0] << 8) | ((u8_t*)pb->payload)[1];

  /* make room for pppInputHeader - should not fail */
  if (pbuf_header(pb, sizeof(*pih) - sizeof(inProtocol)) != 0) {
    PPPDEBUG((LOG_ERR, "pppInProcOverEthernet: could not allocate room for header\n"));
    goto drop;
  }

  pih = pb->payload;

  pih->unit = pd;
  pih->proto = inProtocol;

  /* Dispatch the packet thereby consuming it. */
  if(tcpip_callback(pppInput, pb) != ERR_OK) {
    PPPDEBUG((LOG_ERR, "pppInProcOverEthernet[%d]: tcpip_callback() failed, dropping packet\n", pd));
    goto drop;
  }

  return;

drop:
  LINK_STATS_INC(link.drop);
  pbuf_free(pb);
  return;
}
#endif /* PPPOE_SUPPORT */

#endif /* PPP_SUPPORT */
