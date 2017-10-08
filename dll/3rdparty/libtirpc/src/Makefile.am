## Process this file with automake to create Makefile.in.

## NOTE: this file doesn't really try to be complete.  In particular
## `make dist' won't work at all.  We're just aiming to get the
## program built.  We also don't bother trying to assemble code, or
## anything like that.


INCLUDES = -I$(top_srcdir)/tirpc -DPORTMAP -DINET6 -DVERSION="\"$(VERSION)\"" \
		-D_GNU_SOURCE -Wall -pipe  

lib_LTLIBRARIES = libtirpc.la

#
# Set the library version information
#
# According to the libtool manual:
#
# "This flag accepts an argument of the form current[:revision[:age]].
#
# If either revision or age are omitted, they default to 0. Also note that
# age must be less than or equal to the current interface number.
#
# Here are a set of rules to help you update your library version information:
#
# 1.  Start with version information of 0:0:0 for each libtool library.
# 2.  Update the version information only immediately before a public
#     release of your software. More frequent updates are unnecessary,
#     and only guarantee that the current interface number gets larger faster.
# 3.  If the library source code has changed at all since the last update,
#     then increment revision (c:r:a becomes c:r+1:a).
# 4.  If any interfaces have been added, removed, or changed since the last
#     update, increment current, and set revision to 0.
# 5.  If any interfaces have been added since the last public release,
#     then increment age.
# 6.  If any interfaces have been removed since the last public release,
#     then set age to 0.
#
# _Never_ try to set the interface numbers so that they correspond to the
# release number of your package. This is an abuse that only fosters
# misunderstanding of the purpose of library versions."
#
libtirpc_la_LDFLAGS = -lnsl -lpthread -version-info 1:10:0

libtirpc_la_SOURCES = auth_none.c auth_unix.c authunix_prot.c bindresvport.c clnt_bcast.c \
        clnt_dg.c clnt_generic.c clnt_perror.c clnt_raw.c clnt_simple.c \
        clnt_vc.c rpc_dtablesize.c getnetconfig.c getnetpath.c getrpcent.c \
        getrpcport.c mt_misc.c pmap_clnt.c pmap_getmaps.c pmap_getport.c \
        pmap_prot.c pmap_prot2.c pmap_rmt.c rpc_prot.c rpc_commondata.c \
        rpc_callmsg.c rpc_generic.c rpc_soc.c rpcb_clnt.c rpcb_prot.c \
        rpcb_st_xdr.c svc.c svc_auth.c svc_dg.c svc_auth_unix.c svc_generic.c \
        svc_raw.c svc_run.c svc_simple.c svc_vc.c getpeereid.c \
        auth_time.c auth_des.c authdes_prot.c

## XDR
libtirpc_la_SOURCES += xdr.c xdr_rec.c xdr_array.c xdr_float.c xdr_mem.c xdr_reference.c xdr_stdio.c

## Secure-RPC
if GSS
    libtirpc_la_SOURCES += auth_gss.c authgss_prot.c svc_auth_gss.c \
		svc_auth_none.c
    libtirpc_la_LDFLAGS += $(GSSGLUE_LIBS)
    libtirpc_la_CFLAGS = -DHAVE_RPCSEC_GSS $(GSSGLUE_CFLAGS)
endif

## libtirpc_a_SOURCES += key_call.c key_prot_xdr.c getpublickey.c
## libtirpc_a_SOURCES += netname.c netnamer.c rpcdname.c \
## libtirpc_a_SOURCES += rtime.c \
## auth_time.c auth_des.c authdes_prot.c 

if GSS
$(libtirpc_la_OBJECTS) :auth_none.c auth_unix.c authunix_prot.c bindresvport.c clnt_bcast.c \
        clnt_dg.c clnt_generic.c clnt_perror.c clnt_raw.c clnt_simple.c \
        clnt_vc.c rpc_dtablesize.c getnetconfig.c getnetpath.c getrpcent.c \
        getrpcport.c mt_misc.c pmap_clnt.c pmap_getmaps.c pmap_getport.c \
        pmap_prot.c pmap_prot2.c pmap_rmt.c rpc_prot.c rpc_commondata.c \
        rpc_callmsg.c rpc_generic.c rpc_soc.c rpcb_clnt.c rpcb_prot.c \
        rpcb_st_xdr.c svc.c svc_auth.c svc_dg.c svc_auth_unix.c svc_generic.c \
        svc_raw.c svc_run.c svc_simple.c svc_vc.c \
	xdr.c xdr_rec.c xdr_array.c xdr_float.c xdr_mem.c xdr_reference.c xdr_stdio.c \
	auth_gss.c authgss_prot.c svc_auth_gss.c getpeereid.c \
	auth_time.c auth_des.c authdes_prot.c svc_auth_none.c
        
else
$(libtirpc_la_OBJECTS) :auth_none.c auth_unix.c authunix_prot.c bindresvport.c clnt_bcast.c \
        clnt_dg.c clnt_generic.c clnt_perror.c clnt_raw.c clnt_simple.c \
        clnt_vc.c rpc_dtablesize.c getnetconfig.c getnetpath.c getrpcent.c \
        getrpcport.c mt_misc.c pmap_clnt.c pmap_getmaps.c pmap_getport.c \
        pmap_prot.c pmap_prot2.c pmap_rmt.c rpc_prot.c rpc_commondata.c \
        rpc_callmsg.c rpc_generic.c rpc_soc.c rpcb_clnt.c rpcb_prot.c \
        rpcb_st_xdr.c svc.c svc_auth.c svc_dg.c svc_auth_unix.c svc_generic.c \
        svc_raw.c svc_run.c svc_simple.c svc_vc.c \
	xdr.c xdr_rec.c xdr_array.c xdr_float.c xdr_mem.c xdr_reference.c xdr_stdio.c \
	getpeereid.c auth_time.c auth_des.c authdes_prot.c
endif
