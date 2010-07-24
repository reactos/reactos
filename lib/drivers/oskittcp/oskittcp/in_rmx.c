/*
 * Copyright 1994, 1995 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that both the above copyright notice and this
 * permission notice appear in all copies, that both the above
 * copyright notice and this permission notice appear in all
 * supporting documentation, and that the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  M.I.T. makes
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THIS SOFTWARE IS PROVIDED BY M.I.T. ``AS IS''.  M.I.T. DISCLAIMS
 * ALL EXPRESS OR IMPLIED WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
 * SHALL M.I.T. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * This code does two things necessary for the enhanced TCP metrics to
 * function in a useful manner:
 *  1) It marks all non-host routes as `cloning', thus ensuring that
 *     every actual reference to such a route actually gets turned
 *     into a reference to a host route to the specific destination
 *     requested.
 *  2) When such routes lose all their references, it arranges for them
 *     to be deleted in some random collection of circumstances, so that
 *     a large quantity of stale routing data is not kept in kernel memory
 *     indefinitely.  See in_rtqtimo() below for the exact mechanism.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>

#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>

/*
 * Initialize our routing tree.
 */
int
in_inithead(void **head, int off)
{
	struct radix_node_head *rnh;

	if(!rn_inithead(head, off))
		return 0;

	if(head != (void **)&rt_tables[AF_INET]) /* BOGUS! */
		return 1;	/* only do this for the real routing table */

	rnh = *head;
#if 0
	rnh->rnh_addaddr = in_addroute;
	rnh->rnh_matchaddr = in_matroute;
	rnh->rnh_close = in_clsroute;
	in_rtqtimo(rnh);	/* kick off timeout first time */
#endif
	return 1;
}

