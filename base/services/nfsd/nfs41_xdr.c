/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#include <windows.h>
#include <strsafe.h>

#include "nfs41_compound.h"
#include "nfs41_ops.h"
#include "nfs41_xdr.h"
#include "util.h"
#include "daemon_debug.h"
#include "rpc/rpc.h"

static bool_t encode_file_attrs(
    fattr4 *attrs,
    nfs41_file_info *info);

static __inline int unexpected_op(uint32_t op, uint32_t expected)
{
    if (op == expected)
        return 0;

    eprintf("Op table mismatch. Got %s (%d), expected %s (%d).\n",
        nfs_opnum_to_string(op), op,
        nfs_opnum_to_string(expected), expected);
    return 1;
}

/* typedef uint32_t bitmap4<> */
bool_t xdr_bitmap4(
    XDR *xdr,
    bitmap4 *bitmap)
{
    uint32_t i;

    if (xdr->x_op == XDR_ENCODE) {
        if (bitmap->count > 3) {
            eprintf("encode_bitmap4: count (%d) must be <= 3\n",
                bitmap->count);
            return FALSE;
        }
        if (!xdr_u_int32_t(xdr, &bitmap->count))
            return FALSE;

        for (i = 0; i < bitmap->count; i++)
            if (!xdr_u_int32_t(xdr, &bitmap->arr[i]))
                return FALSE;

    } else if (xdr->x_op == XDR_DECODE) {
        if (!xdr_u_int32_t(xdr, &bitmap->count))
            return FALSE;
        if (bitmap->count > 3) {
            eprintf("decode_bitmap4: count (%d) must be <= 3\n",
                bitmap->count);
            return FALSE;
        }

        for (i = 0; i < bitmap->count; i++)
            if (!xdr_u_int32_t(xdr, &bitmap->arr[i]))
                return FALSE;
    } else 
        return FALSE;

    return TRUE;
}

/* nfstime4 */
static bool_t xdr_nfstime4(
    XDR *xdr,
    nfstime4 *nt)
{
    if (!xdr_hyper(xdr, &nt->seconds))
        return FALSE;

    return xdr_u_int32_t(xdr, &nt->nseconds);
}


/* settime4 */
static uint32_t settime_how(
    nfstime4 *newtime,
    const nfstime4 *time_delta)
{
    nfstime4 current;
    get_nfs_time(&current);
    /* get the absolute difference between current and newtime */
    nfstime_diff(&current, newtime, &current);
    nfstime_abs(&current, &current);
    /* compare the difference with time_delta */
    nfstime_diff(time_delta, &current, &current);
    /* use client time if diff > delta (i.e. time_delta - current < 0) */
    return current.seconds < 0 ? SET_TO_CLIENT_TIME4 : SET_TO_SERVER_TIME4;
}

static bool_t xdr_settime4(
    XDR *xdr,
    nfstime4 *nt,
    const nfstime4 *time_delta)
{
    uint32_t how = settime_how(nt, time_delta);

    if (xdr->x_op != XDR_ENCODE) /* not used for decode */
        return FALSE;

    if (!xdr_u_int32_t(xdr, &how))
        return FALSE;

    if (how == SET_TO_CLIENT_TIME4)
        return xdr_nfstime4(xdr, nt);

    return TRUE;
}

/* stateid4 */
static bool_t xdr_stateid4(
    XDR *xdr,
    stateid4 *si)
{
    if (!xdr_u_int32_t(xdr, &si->seqid))
        return FALSE;

    return xdr_opaque(xdr, (char *)si->other, NFS4_STATEID_OTHER);
}

/* fattr4 */
bool_t xdr_fattr4(
    XDR *xdr,
    fattr4 *fattr)
{
    unsigned char *attr_vals = fattr->attr_vals;

    if (!xdr_bitmap4(xdr, &fattr->attrmask))
        return FALSE;

    return xdr_bytes(xdr, (char **)&attr_vals, &fattr->attr_vals_len, NFS4_OPAQUE_LIMIT);
}

/* nfs41_fh */
static bool_t xdr_fh(
    XDR *xdr,
    nfs41_fh *fh)
{
    unsigned char *pfh = fh->fh;
    return xdr_bytes(xdr, (char **)&pfh, &fh->len, NFS4_FHSIZE);
}

/* nfs41_fsid */
static bool_t xdr_fsid(
    XDR *xdr,
    nfs41_fsid *fsid)
{
    if (!xdr_u_hyper(xdr, &fsid->major))
        return FALSE;

    return xdr_u_hyper(xdr, &fsid->minor);
}


/* nfs41_component */
static bool_t encode_component(
    XDR *xdr,
    const nfs41_component *component)
{
    uint32_t len = component->len;
    return xdr_bytes(xdr, (char **)&component->name, &len, NFS4_OPAQUE_LIMIT);
}

static bool_t decode_component(
    XDR *xdr,
    nfs41_component *component)
{
    bool_t result;
    uint32_t len;

    result = xdr_bytes(xdr, (char **)&component->name, &len, NFS4_OPAQUE_LIMIT);
    component->len = (result == FALSE) ? 0 : (unsigned short)len;
    return result;
}


/* state_owner4 */
static bool_t xdr_state_owner4(
    XDR *xdr,
    state_owner4 *so)
{
    u_quad_t clientid = 0;
    unsigned char *owner = so->owner;

    /* 18.16.3. "The client can set the clientid field to any value and
     * the server MUST ignore it.  Instead the server MUST derive the
     * client ID from the session ID of the SEQUENCE operation of the
     * COMPOUND request. */
    if (xdr->x_op == XDR_ENCODE) {
        if (!xdr_u_hyper(xdr, &clientid)) /* clientid = 0 */
            return FALSE;
    } else if (xdr->x_op == XDR_DECODE) {
        if (!xdr_u_hyper(xdr, &clientid))
            return FALSE;
    } else return FALSE;

    return xdr_bytes(xdr, (char **)&owner, &so->owner_len, NFS4_OPAQUE_LIMIT);
}

static bool_t xdr_layout_types(
    XDR *xdr,
    uint32_t *layout_type)
{
    u_int32_t i, count, type;

    if (xdr->x_op != XDR_DECODE) {
        eprintf("xdr_layout_types: xdr->x_op is not XDR_DECODE! "
            "x_op %d not supported.\n", xdr->x_op);
        return FALSE;
    }

    *layout_type = 0;

    if (!xdr_u_int32_t(xdr, &count))
        return FALSE;

    for (i = 0; i < count; i++) {
        if (!xdr_u_int32_t(xdr, &type))
            return FALSE;

        *layout_type |= 1 << (type - 1);
    }
    return TRUE;
}

static bool_t xdr_threshold_item(
    XDR *xdr,
    threshold_item4 *item)
{
    bitmap4 bitmap;

    if (!xdr_u_int32_t(xdr, &item->type))
        return FALSE;

    if (!xdr_bitmap4(xdr, &bitmap))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &bitmap.count))
        return FALSE;

    if (bitmap.count) {
        if (bitmap.arr[0] & 0x1 && !xdr_u_hyper(xdr, &item->hints[0]))
            return FALSE;
        if (bitmap.arr[0] & 0x2 && !xdr_u_hyper(xdr, &item->hints[1]))
            return FALSE;
        if (bitmap.arr[0] & 0x4 && !xdr_u_hyper(xdr, &item->hints[2]))
            return FALSE;
        if (bitmap.arr[0] & 0x8 && !xdr_u_hyper(xdr, &item->hints[3]))
            return FALSE;
    }
    return TRUE;
}

static bool_t xdr_mdsthreshold(
    XDR *xdr,
    mdsthreshold4 *mdsthreshold)
{
    uint32_t i;

    if (!xdr_u_int32_t(xdr, &mdsthreshold->count))
        return FALSE;

    if (mdsthreshold->count > MAX_MDSTHRESHOLD_ITEMS)
        return FALSE;

    for (i = 0; i < mdsthreshold->count; i++)
        if (!xdr_threshold_item(xdr, &mdsthreshold->items[i]))
            return FALSE;
    return TRUE;
}

static bool_t xdr_nfsace4(
    XDR *xdr,
    nfsace4 *ace)
{
    char *who = ace->who;

    if (!xdr_u_int32_t(xdr, &ace->acetype))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &ace->aceflag))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &ace->acemask))
        return FALSE;

    /* 'who' is a static array, so don't try to free it */
    if (xdr->x_op == XDR_FREE)
        return TRUE;

    return xdr_string(xdr, &who, NFS4_OPAQUE_LIMIT);
}

static bool_t xdr_nfsdacl41(
    XDR *xdr,
    nfsacl41 *acl)
{
    if (!xdr_u_int32_t(xdr, &acl->flag))
        return FALSE;

    return xdr_array(xdr, (char**)&acl->aces, &acl->count,
        32, sizeof(nfsace4), (xdrproc_t)xdr_nfsace4);
}

static bool_t xdr_nfsacl41(
    XDR *xdr,
    nfsacl41 *acl)
{
    return xdr_array(xdr, (char**)&acl->aces, &acl->count,
        32, sizeof(nfsace4), (xdrproc_t)xdr_nfsace4);
}

void nfsacl41_free(nfsacl41 *acl)
{
    XDR xdr = { XDR_FREE };
    xdr_nfsacl41(&xdr, acl);
}

/* pathname4
 * decode a variable array of components into a nfs41_abs_path */
static bool_t decode_pathname4(
    XDR *xdr,
    nfs41_abs_path *path)
{
    char *pos;
    u_int32_t i, count, len, remaining;

    /* decode the number of components */
    if (!xdr_u_int32_t(xdr, &count))
        return FALSE;

    pos = (char *)path->path;
    remaining = NFS41_MAX_PATH_LEN;

    /* decode each component */
    for (i = 0; i < count; i++) {
        len = remaining;
        if (!xdr_bytes(xdr, (char **)&pos, &len, NFS41_MAX_PATH_LEN))
            return FALSE;
        remaining -= len;
        pos += len;

        if (i < count-1) { /* add a \ between components */
            if (remaining < 1)
                return FALSE;
            *pos++ = '\\';
            remaining--;
        }
    }
    path->len = (unsigned short)(NFS41_MAX_PATH_LEN - remaining);
    return TRUE;
}

/* fs_location4 */
static bool_t decode_fs_location4(
    XDR *xdr,
    fs_location4 *location)
{
    fs_location_server *arr;
    char *address;
    u_int32_t i, count, len;

    /* decode the number of servers */
    if (!xdr_u_int32_t(xdr, &count))
        return FALSE;

    /* allocate the fs_location_server array */
    if (count == 0) {
        free(location->servers);
        arr = NULL;
    } else if (count != location->server_count) {
        arr = realloc(location->servers, count * sizeof(fs_location_server));
        if (arr == NULL)
            return FALSE;
        ZeroMemory(arr, count * sizeof(fs_location_server));
    } else {
        arr = location->servers;
    }

    location->servers = arr;
    location->server_count = count;

    for (i = 0; i < count; i++) {
        len = NFS41_HOSTNAME_LEN;
        address = arr[i].address;
        if (!xdr_bytes(xdr, &address, &len, NFS41_HOSTNAME_LEN)) {
            free(arr);
            return FALSE;
        }
        arr[i].address[len] = '\0';
    }

    return decode_pathname4(xdr, &location->path);
}

/* fs_locations4 */
static bool_t decode_fs_locations4(
    XDR *xdr,
    fs_locations4 *locations)
{
    u_int32_t i, count;
    fs_location4 *arr;

    if (!decode_pathname4(xdr, &locations->path))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &count))
        return FALSE;

    /* allocate the fs_location array */
    if (count == 0) {
        free(locations->locations);
        arr = NULL;
    } else if (count != locations->location_count) {
        arr = realloc(locations->locations, count * sizeof(fs_location4));
        if (arr == NULL)
            return FALSE;
        ZeroMemory(arr, count * sizeof(fs_location4));
    } else {
        arr = locations->locations;
    }

    locations->locations = arr;
    locations->location_count = count;

    for (i = 0; i < count; i++) {
        if (!decode_fs_location4(xdr, &arr[i])) {
            free(arr);
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * OP_EXCHANGE_ID
 */
static bool_t xdr_client_owner4(
    XDR *xdr,
    client_owner4 *co)
{
    unsigned char *co_ownerid = co->co_ownerid;
    if (!xdr_opaque(xdr, (char *)&co->co_verifier[0], NFS4_VERIFIER_SIZE))
        return FALSE;

    return xdr_bytes(xdr, (char **)&co_ownerid, &co->co_ownerid_len, NFS4_OPAQUE_LIMIT);
}

#if 0
static bool_t encode_state_protect_ops4(
    XDR *xdr,
    state_protect_ops4 *spo)
{
    if (!xdr_bitmap4(xdr, &spo->spo_must_enforce))
        return FALSE;

    return xdr_bitmap4(xdr, &spo->spo_must_allow);
}

static bool_t encode_ssv_sp_parms4(
    XDR *xdr,
    ssv_sp_parms4 *spp)
{
    if (!encode_state_protect_ops4(xdr, &spp->ssp_ops))
        return FALSE;

    if (!xdr_bytes(xdr, &spp->ssp_hash_algs,
        &spp->ssp_hash_algs_len, NFS4_OPAQUE_LIMIT))
        return FALSE;

    if (!xdr_bytes(xdr, &spp->ssp_encr_algs,
        &spp->ssp_encr_algs_len, NFS4_OPAQUE_LIMIT))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &spp->ssp_window))
        return FALSE;

    return xdr_u_int32_t(xdr, &spp->ssp_num_gss_handles);
}
#endif

static bool_t xdr_state_protect4_a(
    XDR *xdr,
    state_protect4_a *spa)
{
    bool_t result = TRUE;

    if (!xdr_u_int32_t(xdr, (u_int32_t *)&spa->spa_how))
        return FALSE;

    switch (spa->spa_how)
    {
    case SP4_NONE:
        break;
#if 0
    case SP4_MACH_CRED:
        result = xdr_state_protect_ops4(xdr, &spa->u.spa_mach_ops);
        break;
    case SP4_SSV:
        result = xdr_ssv_sp_parms4(xdr, &spa->u.spa_ssv_parms);
        break;
#endif
    default:
        eprintf("encode_state_protect4_a: state protect "
            "type %d not supported.\n", spa->spa_how);
        result = FALSE;
        break;
    }
    return result;
}

static bool_t xdr_nfs_impl_id4(
    XDR *xdr,
    nfs_impl_id4 *nii)
{
    unsigned char *nii_domain = nii->nii_domain;
    unsigned char *nii_name = nii->nii_name;

    if (!xdr_bytes(xdr, (char **)&nii_domain, &nii->nii_domain_len, NFS4_OPAQUE_LIMIT))
        return FALSE;

    if (!xdr_bytes(xdr, (char **)&nii_name, &nii->nii_name_len, NFS4_OPAQUE_LIMIT))
        return FALSE;

    return xdr_nfstime4(xdr, &nii->nii_date);
}


static bool_t encode_op_exchange_id(
    XDR *xdr,
    nfs_argop4 *argop)
{
    uint32_t zero = 0;
    uint32_t one = 1;

    nfs41_exchange_id_args *args = (nfs41_exchange_id_args*)argop->arg;

    if (unexpected_op(argop->op, OP_EXCHANGE_ID))
        return FALSE;

    if (!xdr_client_owner4(xdr, args->eia_clientowner))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->eia_flags))
        return FALSE;

    if (!xdr_state_protect4_a(xdr, &args->eia_state_protect))
        return FALSE;

    if (args->eia_client_impl_id)
    {
        if (!xdr_u_int32_t(xdr, &one))
            return FALSE;
        return xdr_nfs_impl_id4(xdr, args->eia_client_impl_id);
    }
    else
        return xdr_u_int32_t(xdr, &zero);
}

#if 0

static bool_t decode_state_protect_ops4(
    XDR *xdr,
    state_protect_ops4 *spo)
{
    if (!xdr_bitmap4(xdr, &spo->spo_must_enforce))
        return FALSE;

    return xdr_bitmap4(xdr, &spo->spo_must_allow);
}

static bool_t decode_ssv_prot_info4(
    XDR *xdr,
    ssv_prot_info4 *spi)
{
/*  uint32_t i; */

    if (!decode_state_protect_ops4(xdr, &spi->spi_ops))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &spi->spi_hash_alg))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &spi->spi_encr_alg))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &spi->spi_ssv_len))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &spi->spi_window))
        return FALSE;

/* TODO: spi->spi_handles */
    return xdr_u_int32_t(xdr, 0);
    /*
    if (!xdr_u_int32_t(xdr, &spi->spi_handles.count))
        return FALSE;

    for (i = 0; i < spi->spi_handles.count; i++)
        if (!xdr_opaque(xdr, &spi->spi_handles.arr[i])
            return FALSE;
*/
    return TRUE;
}
#endif

static bool_t xdr_state_protect4_r(
    XDR *xdr,
    state_protect4_r *spr)
{
    bool_t result = TRUE;

    if (!xdr_u_int32_t(xdr, (uint32_t *)&spr->spr_how))
        return FALSE;

    switch (spr->spr_how)
    {
    case SP4_NONE:
        break;
#if 0
    case SP4_MACH_CRED:
        result = decode_state_protect_ops4(xdr, &spr->u.spr_mach_ops);
        break;
    case SP4_SSV:
        result = decode_ssv_prot_info4(xdr, &spr->u.spr_ssv_info);
        break;
#endif
    default:
        eprintf("decode_state_protect4_r: state protect "
            "type %d not supported.\n", spr->spr_how);
        result = FALSE;
        break;
    }
    return result;
}

static bool_t xdr_server_owner4(
    XDR *xdr,
    server_owner4 *so)
{
    char *so_major_id = so->so_major_id;

    if (!xdr_u_hyper(xdr, &so->so_minor_id))
        return FALSE;

    return xdr_bytes(xdr, (char **)&so_major_id,
        &so->so_major_id_len, NFS4_OPAQUE_LIMIT);
}

static bool_t decode_op_exchange_id(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_exchange_id_res *res = (nfs41_exchange_id_res*)resop->res;
    char *server_scope = (char *)res->server_scope;

    if (unexpected_op(resop->op, OP_EXCHANGE_ID))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status != NFS4_OK)
        return TRUE;

    if (!xdr_u_hyper(xdr, &res->clientid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->sequenceid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->flags))
        return FALSE;

    if (!xdr_state_protect4_r(xdr, &res->state_protect))
        return FALSE;

    if (!xdr_server_owner4(xdr, &res->server_owner))
        return FALSE;

    return xdr_bytes(xdr, &server_scope,
        &res->server_scope_len, NFS4_OPAQUE_LIMIT);
}

/*
 * OP_CREATE_SESSION
 */
static bool_t xdr_channel_attrs4(
    XDR *xdr,
    nfs41_channel_attrs *attrs)
{
    uint32_t zero = 0;
    uint32_t one = 1;

    /* count4 ca_headerpadsize */
    if (!xdr_u_int32_t(xdr, &attrs->ca_headerpadsize))
        return FALSE;

    /* count4 ca_maxrequestsize */
    if (!xdr_u_int32_t(xdr, &attrs->ca_maxrequestsize))
        return FALSE;

    /* count4 ca_maxresponsesize */
    if (!xdr_u_int32_t(xdr, &attrs->ca_maxresponsesize))
        return FALSE;

    /* count4 ca_maxresponsesize_cached */
    if (!xdr_u_int32_t(xdr, &attrs->ca_maxresponsesize_cached))
        return FALSE;

    /* count4 ca_maxoperations */
    if (!xdr_u_int32_t(xdr, &attrs->ca_maxoperations))
        return FALSE;

    /* count4 ca_maxrequests */
    if (!xdr_u_int32_t(xdr, &attrs->ca_maxrequests))
        return FALSE;

    if (xdr->x_op == XDR_ENCODE) {
        /* uint32_t ca_rdma_ird<1> */
        if (attrs->ca_rdma_ird)
        {
            if (!xdr_u_int32_t(xdr, &one))
                return FALSE;
            return xdr_u_int32_t(xdr, attrs->ca_rdma_ird);
        }
        else {
            return xdr_u_int32_t(xdr, &zero);
        }
    }
    else if (xdr->x_op == XDR_DECODE) {
#if 0
        u_int32_t count;
        /* uint32_t ca_rdma_ird<1> */
        if (!xdr_u_int32_t(xdr, &count))
            return FALSE;
        if (count > 1)
            return FALSE;
        if (count)
            return xdr_u_int32_t(xdr, attrs->ca_rdma_ird);
        else
#endif
            return TRUE;
    }
    else {
        eprintf("%s: xdr->x_op %d not supported.\n",
            "xdr_channel_attrs4", xdr->x_op);
        return FALSE;
    }
}

static bool_t encode_backchannel_sec_parms(
    XDR *xdr,
    nfs41_callback_secparms *args)
{
    uint32_t zero = 0;

    if (!xdr_u_int32_t(xdr, &args->type))
        return FALSE;

    switch (args->type)  {
    case AUTH_NONE: return TRUE;
    case AUTH_SYS:
        if (!xdr_u_int32_t(xdr, &args->u.auth_sys.stamp))
            return FALSE;
        if (!xdr_string(xdr, &args->u.auth_sys.machinename, NI_MAXHOST))
            return FALSE;
        return xdr_u_int32_t(xdr, &zero) && xdr_u_int32_t(xdr, &zero) && 
                xdr_u_int32_t(xdr, &zero);
    case RPCSEC_GSS:
    default:
        return FALSE;
    }
}

static bool_t encode_op_create_session(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_create_session_args *args = (nfs41_create_session_args*)argop->arg;
    nfs41_callback_secparms *cb_secparams = args->csa_cb_secparams;
    uint32_t cb_count = 2;

    if (unexpected_op(argop->op, OP_CREATE_SESSION))
        return FALSE;

    /* clientid4 csa_clientid */
    if (!xdr_u_hyper(xdr, &args->csa_clientid))
        return FALSE;

    /* sequenceid4 csa_sequence */
    if (!xdr_u_int32_t(xdr, &args->csa_sequence))
        return FALSE;

    /* TODO: uint32_t csa_flags = 0 */
    if (!xdr_u_int32_t(xdr, &args->csa_flags))
        return FALSE;

    /* channel_attrs4 csa_fore_chan_attrs */
    if (!xdr_channel_attrs4(xdr, &args->csa_fore_chan_attrs))
        return FALSE;

    /* channel_attrs4 csa_back_chan_attrs */
    if (!xdr_channel_attrs4(xdr, &args->csa_back_chan_attrs))
        return FALSE;

    /* TODO: uint32_t csa_cb_program = 1234 */
    if (!xdr_u_int32_t(xdr, &args->csa_cb_program))
        return FALSE;

    return xdr_array(xdr, (char **)&cb_secparams, &cb_count,
        3, sizeof(nfs41_callback_secparms), (xdrproc_t) encode_backchannel_sec_parms);
}

static bool_t decode_op_create_session(
    XDR *xdr,
    nfs_resop4 *resop)
{
    uint32_t opstatus;
    nfs41_create_session_res *res = (nfs41_create_session_res*)resop->res;

    if (unexpected_op(resop->op, OP_CREATE_SESSION))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &opstatus))
        return FALSE;

    if (opstatus != NFS4_OK)
        return TRUE;

    if (!xdr_opaque(xdr, (char *)res->csr_sessionid, NFS4_SESSIONID_SIZE))
        return FALSE;

    /* sequenceid4 csr_sequence */
    if (!xdr_u_int32_t(xdr, &res->csr_sequence))
        return FALSE;

    /* uint32_t csr_flags */
    if (!xdr_u_int32_t(xdr, &res->csr_flags))
        return FALSE;

    /* channel_attrs4 csr_fore_chan_attrs */
    if (!xdr_channel_attrs4(xdr, res->csr_fore_chan_attrs))
        return FALSE;

    /* channel_attrs4 csr_back_chan_attrs */
    return xdr_channel_attrs4(xdr, res->csr_back_chan_attrs);
}


/*
 * OP_BIND_CONN_TO_SESSION
 */
static bool_t encode_op_bind_conn_to_session(
    XDR *xdr,
    nfs_argop4 *argop)
{
    uint32_t zero = 0;

    nfs41_bind_conn_to_session_args *args =
        (nfs41_bind_conn_to_session_args*)argop->arg;

    if (unexpected_op(argop->op, OP_BIND_CONN_TO_SESSION))
        return FALSE;

    if (!xdr_opaque(xdr, (char *)args->sessionid, NFS4_SESSIONID_SIZE))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&args->dir))
        return FALSE;

    return xdr_u_int32_t(xdr, &zero); /* bctsa_use_conn_in_rdma_mode = false */
}

static bool_t decode_op_bind_conn_to_session(
    XDR *xdr,
    nfs_resop4 *resop)
{
    unsigned char sessionid_ignored[NFS4_SESSIONID_SIZE];
    nfs41_bind_conn_to_session_res *res =
        (nfs41_bind_conn_to_session_res*)resop->res;
    bool_t use_rdma_ignored;

    if (unexpected_op(resop->op, OP_BIND_CONN_TO_SESSION))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&res->status))
        return FALSE;

    if (res->status == NFS4_OK) {
        if (!xdr_opaque(xdr, (char *)&sessionid_ignored, NFS4_SESSIONID_SIZE))
            return FALSE;

        if (!xdr_enum(xdr, (enum_t *)&res->dir))
            return FALSE;

        return xdr_bool(xdr, &use_rdma_ignored);
    }
    return TRUE;
}


/*
 * OP_DESTROY_SESSION
 */
static bool_t encode_op_destroy_session(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_destroy_session_args *args = (nfs41_destroy_session_args*)argop->arg;

    if (unexpected_op(argop->op, OP_DESTROY_SESSION))
        return FALSE;

    return xdr_opaque(xdr, (char *)args->dsa_sessionid, NFS4_SESSIONID_SIZE);
}

static bool_t decode_op_destroy_session(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_destroy_session_res *res = (nfs41_destroy_session_res*)resop->res;

    if (unexpected_op(resop->op, OP_DESTROY_SESSION))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->dsr_status);
}

/*
 * OP_DESTROY_CLIENTID
 */
static bool_t encode_op_destroy_clientid(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_destroy_clientid_args *args = (nfs41_destroy_clientid_args*)argop->arg;

    if (unexpected_op(argop->op, OP_DESTROY_CLIENTID))
        return FALSE;

    return xdr_u_hyper(xdr, &args->dca_clientid);
}

static bool_t decode_op_destroy_clientid(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_destroy_clientid_res *res = (nfs41_destroy_clientid_res*)resop->res;

    if (unexpected_op(resop->op, OP_DESTROY_CLIENTID))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->dcr_status);
}


/*
 * OP_SEQUENCE
 */
static bool_t encode_op_sequence(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_sequence_args *args = (nfs41_sequence_args*)argop->arg;

    if (unexpected_op(argop->op, OP_SEQUENCE))
        return FALSE;

    if (!xdr_opaque(xdr, (char *)args->sa_sessionid, NFS4_SESSIONID_SIZE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->sa_sequenceid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->sa_slotid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->sa_highest_slotid))
        return FALSE;

    return xdr_bool(xdr, &args->sa_cachethis);
}

static bool_t xdr_sequence_res_ok(
    XDR *xdr,
    nfs41_sequence_res_ok *res)
{
    if (!xdr_opaque(xdr, (char *)res->sr_sessionid, NFS4_SESSIONID_SIZE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->sr_sequenceid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->sr_slotid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->sr_highest_slotid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->sr_target_highest_slotid))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->sr_status_flags);
}

static bool_t decode_op_sequence(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_sequence_res *res = (nfs41_sequence_res*)resop->res;

    if (unexpected_op(resop->op, OP_SEQUENCE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->sr_status))
        return FALSE;

    if (res->sr_status == NFS4_OK)
        return xdr_sequence_res_ok(xdr, &res->sr_resok4);

    return TRUE;
}


/*
 * OP_RECLAIM_COMPLETE
 */
static bool_t encode_op_reclaim_complete(
    XDR *xdr,
    nfs_argop4 *argop)
{
    bool_t zero = FALSE;

    if (unexpected_op(argop->op, OP_RECLAIM_COMPLETE))
        return FALSE;

    /* rca_one_fs = 0 indicates that the reclaim applies to all filesystems */
    return xdr_bool(xdr, &zero);
}

static bool_t decode_op_reclaim_complete(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_reclaim_complete_res *res = (nfs41_reclaim_complete_res*)resop->res;

    if (unexpected_op(resop->op, OP_RECLAIM_COMPLETE))
        return FALSE;

    return xdr_enum(xdr, (enum_t *)&res->status);
}


/*
 * OP_PUTFH
 */
static bool_t encode_op_putfh(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_putfh_args *args = (nfs41_putfh_args*)argop->arg;

    if (unexpected_op(argop->op, OP_PUTFH))
        return FALSE;

    return xdr_fh(xdr, &args->file->fh);
}

static bool_t decode_op_putfh(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_putfh_res *res = (nfs41_putfh_res*)resop->res;

    if (unexpected_op(resop->op, OP_PUTFH))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_PUTROOTFH
 */
static bool_t encode_op_putrootfh(
    XDR *xdr,
    nfs_argop4* argop)
{
    if (unexpected_op(argop->op, OP_PUTROOTFH))
        return FALSE;
    /* void */
    return TRUE;
}

static bool_t decode_op_putrootfh(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_putrootfh_res *res = (nfs41_putrootfh_res*)resop->res;

    if (unexpected_op(resop->op, OP_PUTROOTFH))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_GETFH
 */
static bool_t encode_op_getfh(
    XDR *xdr,
    nfs_argop4 *argop)
{
    if (unexpected_op(argop->op, OP_GETFH))
        return FALSE;

    /* void */
    return TRUE;
}

static bool_t decode_op_getfh(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_getfh_res *res = (nfs41_getfh_res*)resop->res;

    if (unexpected_op(resop->op, OP_GETFH))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_fh(xdr, res->fh);

    return TRUE;
}


/*
 * OP_LOOKUP
 */
static bool_t encode_op_lookup(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_lookup_args *args = (nfs41_lookup_args*)argop->arg;

    if (unexpected_op(argop->op, OP_LOOKUP))
        return FALSE;

    return encode_component(xdr, args->name);
}

static bool_t decode_op_lookup(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_lookup_res *res = (nfs41_lookup_res*)resop->res;

    if (unexpected_op(resop->op, OP_LOOKUP))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_ACCESS
 */
static bool_t encode_op_access(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_access_args *args = (nfs41_access_args*)argop->arg;

    if (unexpected_op(argop->op, OP_ACCESS))
        return FALSE;

    return xdr_u_int32_t(xdr, &args->access);
}

static bool_t decode_op_access(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_access_res *res = (nfs41_access_res*)resop->res;

    if (unexpected_op(resop->op, OP_ACCESS))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
    {
        if (!xdr_u_int32_t(xdr, &res->supported))
            return FALSE;

        return xdr_u_int32_t(xdr, &res->access);
    }
    return TRUE;
}


/*
 * OP_CLOSE
 */
static bool_t encode_op_close(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_op_close_args *args = (nfs41_op_close_args*)argop->arg;
    uint32_t zero = 0;

    if (unexpected_op(argop->op, OP_CLOSE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &zero)) // This should be ignored by server
        return FALSE;

    return xdr_stateid4(xdr, &args->stateid->stateid);
}

static bool_t decode_op_close(
    XDR *xdr,
    nfs_resop4 *resop)
{
    stateid4 ignored;
    nfs41_op_close_res *res = (nfs41_op_close_res*)resop->res;

    if (unexpected_op(resop->op, OP_CLOSE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_stateid4(xdr, &ignored);

    return TRUE;
}


/*
 * OP_COMMIT
 */
static bool_t encode_op_commit(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_commit_args *args = (nfs41_commit_args*)argop->arg;

    if (unexpected_op(argop->op, OP_COMMIT))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->offset))
        return FALSE;

    return xdr_u_int32_t(xdr, &args->count);
}

static bool_t decode_op_commit(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_commit_res *res = (nfs41_commit_res*)resop->res;

    if (unexpected_op(resop->op, OP_COMMIT))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_opaque(xdr, (char *)res->verf->verf, NFS4_VERIFIER_SIZE);

    return TRUE;
}


/*
 * OP_CREATE
 */
static bool_t encode_createtype4(
    XDR *xdr,
    createtype4 *ct)
{
    bool_t result = TRUE;
    const char *linkdata;

    if (!xdr_u_int32_t(xdr, &ct->type))
        return FALSE;

    switch (ct->type)
    {
    case NF4LNK:
        linkdata = ct->u.lnk.linkdata;
        result = xdr_bytes(xdr, (char**)&linkdata, &ct->u.lnk.linkdata_len,
            NFS4_OPAQUE_LIMIT);
        break;
    case NF4BLK:
    case NF4CHR:
        result = xdr_u_int32_t(xdr, &ct->u.devdata.specdata1);
        if (result == TRUE)
            result = xdr_u_int32_t(xdr, &ct->u.devdata.specdata2);
        break;
    default:
        // Some types need no further action
        break;
    }
    return result;
}

static bool_t encode_createattrs4(
    XDR *xdr,
    nfs41_file_info* createattrs)
{
    fattr4 attrs;

    /* encode attribute values from createattrs->info into attrs.attr_vals */
    attrs.attr_vals_len = NFS4_OPAQUE_LIMIT;
    if (!encode_file_attrs(&attrs, createattrs))
        return FALSE;

    return xdr_fattr4(xdr, &attrs);
}

static bool_t encode_op_create(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_create_args *args = (nfs41_create_args*)argop->arg;

    if (unexpected_op(argop->op, OP_CREATE))
        return FALSE;

    if (!encode_createtype4(xdr, &args->objtype))
        return FALSE;

    if (!encode_component(xdr, args->name))
        return FALSE;

    return encode_createattrs4(xdr, args->createattrs);
}

static bool_t xdr_change_info4(
    XDR *xdr,
    change_info4 *cinfo)
{
    if (!xdr_bool(xdr, &cinfo->atomic))
        return FALSE;

    if (!xdr_u_hyper(xdr, &cinfo->before))
        return FALSE;

    return xdr_u_hyper(xdr, &cinfo->after);
}

static bool_t decode_op_create(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_create_res *res = (nfs41_create_res*)resop->res;

    if (unexpected_op(resop->op, OP_CREATE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
    {
        if (!xdr_change_info4(xdr, &res->cinfo))
            return FALSE;
        return xdr_bitmap4(xdr, &res->attrset);
    }
    return TRUE;
}


/*
 * OP_LINK
 */
static bool_t encode_op_link(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_link_args *args = (nfs41_link_args*)argop->arg;

    if (unexpected_op(argop->op, OP_LINK))
        return FALSE;

    return encode_component(xdr, args->newname);
}

static bool_t decode_op_link(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_link_res *res = (nfs41_link_res*)resop->res;

    if (unexpected_op(resop->op, OP_LINK))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_change_info4(xdr, &res->cinfo);

    return TRUE;
}


/*
 * OP_LOCK
 */
static bool_t xdr_locker4(
    XDR *xdr,
    locker4 *locker)
{
    if (xdr->x_op != XDR_ENCODE) {
        eprintf("%s: xdr->x_op %d is not supported!\n",
            "xdr_locker4", xdr->x_op);
        return FALSE;
    }

    if (!xdr_bool(xdr, &locker->new_lock_owner))
        return FALSE;

    if (locker->new_lock_owner) {
        /* open_to_lock_owner4 open_owner */
        if (!xdr_u_int32_t(xdr, &locker->u.open_owner.open_seqid))
            return FALSE;

        if (!xdr_stateid4(xdr, &locker->u.open_owner.open_stateid->stateid))
            return FALSE;

        if (!xdr_u_int32_t(xdr, &locker->u.open_owner.lock_seqid))
            return FALSE;

        return xdr_state_owner4(xdr, locker->u.open_owner.lock_owner);
    } else {
        /* exist_lock_owner4 lock_owner */
        if (!xdr_stateid4(xdr, &locker->u.lock_owner.lock_stateid->stateid))
            return FALSE;

        return xdr_u_int32_t(xdr, &locker->u.lock_owner.lock_seqid);
    }
}

static bool_t encode_op_lock(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_lock_args *args = (nfs41_lock_args*)argop->arg;

    if (unexpected_op(argop->op, OP_LOCK))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->locktype))
        return FALSE;

    if (!xdr_bool(xdr, &args->reclaim))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->offset))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->length))
        return FALSE;

    return xdr_locker4(xdr, &args->locker);
}

static bool_t decode_lock_res_denied(
    XDR *xdr,
    lock_res_denied *denied)
{
    if (!xdr_u_hyper(xdr, &denied->offset))
        return FALSE;

    if (!xdr_u_hyper(xdr, &denied->length))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &denied->locktype))
        return FALSE;

    return xdr_state_owner4(xdr, &denied->owner);
}

static bool_t decode_op_lock(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_lock_res *res = (nfs41_lock_res*)resop->res;

    if (unexpected_op(resop->op, OP_LOCK))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    switch (res->status) {
    case NFS4_OK:
        return xdr_stateid4(xdr, res->u.resok4.lock_stateid);
        break;
    case NFS4ERR_DENIED:
        return decode_lock_res_denied(xdr, &res->u.denied);
        break;
    default:
        break;
    }

    return TRUE;
}


/*
 * OP_LOCKT
 */
static bool_t encode_op_lockt(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_lockt_args *args = (nfs41_lockt_args*)argop->arg;

    if (unexpected_op(argop->op, OP_LOCKT))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->locktype))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->offset))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->length))
        return FALSE;

    return xdr_state_owner4(xdr, args->owner);
}

static bool_t decode_op_lockt(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_lockt_res *res = (nfs41_lockt_res*)resop->res;

    if (unexpected_op(resop->op, OP_LOCKT))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4ERR_DENIED)
        return decode_lock_res_denied(xdr, &res->denied);

    return TRUE;
}


/*
 * OP_LOCKU
 */
static bool_t encode_op_locku(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_locku_args *args = (nfs41_locku_args*)argop->arg;

    if (unexpected_op(argop->op, OP_LOCKU))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->locktype))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->seqid))
        return FALSE;

    if (!xdr_stateid4(xdr, &args->lock_stateid->stateid))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->offset))
        return FALSE;

    return xdr_u_hyper(xdr, &args->length);
}

static bool_t decode_op_locku(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_locku_res *res = (nfs41_locku_res*)resop->res;

    if (unexpected_op(resop->op, OP_LOCKU))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_stateid4(xdr, res->lock_stateid);

    return TRUE;
}


/*
 * OP_DELEGPURGE
 */
static bool_t encode_op_delegpurge(
    XDR *xdr,
    nfs_argop4 *argop)
{
    uint64_t zero = 0;

    if (unexpected_op(argop->op, OP_DELEGPURGE))
        return FALSE;

    /* The client SHOULD set the client field to zero,
     * and the server MUST ignore the clientid field. */
    return xdr_u_int64_t(xdr, &zero);
}

static bool_t decode_op_delegpurge(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_delegpurge_res *res = (nfs41_delegpurge_res*)resop->res;

    if (unexpected_op(resop->op, OP_DELEGPURGE))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_DELEGRETURN
 */
static bool_t encode_op_delegreturn(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_delegreturn_args *args = (nfs41_delegreturn_args*)argop->arg;

    if (unexpected_op(argop->op, OP_DELEGRETURN))
        return FALSE;

    return xdr_stateid4(xdr, &args->stateid->stateid);
}

static bool_t decode_op_delegreturn(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_delegreturn_res *res = (nfs41_delegreturn_res*)resop->res;

    if (unexpected_op(resop->op, OP_DELEGRETURN))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_GETATTR
 */
static bool_t encode_op_getattr(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_getattr_args *args = (nfs41_getattr_args*)argop->arg;

    if (unexpected_op(argop->op, OP_GETATTR))
        return FALSE;

    return xdr_bitmap4(xdr, args->attr_request);
}

static bool_t decode_file_attrs(
    XDR *xdr,
    fattr4 *attrs,
    nfs41_file_info *info)
{
    if (attrs->attrmask.count >= 1) {
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_SUPPORTED_ATTRS) {
            if (!xdr_bitmap4(xdr, info->supported_attrs))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_TYPE) {
            if (!xdr_u_int32_t(xdr, &info->type))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_CHANGE) {
            if (!xdr_u_hyper(xdr, &info->change))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_SIZE) {
            if (!xdr_u_hyper(xdr, &info->size))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_LINK_SUPPORT) {
            if (!xdr_bool(xdr, &info->link_support))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_SYMLINK_SUPPORT) {
            if (!xdr_bool(xdr, &info->symlink_support))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_FSID) {
            if (!xdr_fsid(xdr, &info->fsid))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_LEASE_TIME) {
            if (!xdr_u_int32_t(xdr, &info->lease_time))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_RDATTR_ERROR) {
            if (!xdr_u_int32_t(xdr, &info->rdattr_error))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_ACL) {
            nfsacl41 *acl = info->acl;
            if (!xdr_array(xdr, (char**)&acl->aces, &acl->count,
                32, sizeof(nfsace4), (xdrproc_t)xdr_nfsace4))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_ACLSUPPORT) {
            if (!xdr_u_int32_t(xdr, &info->aclsupport))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_ARCHIVE) {
            if (!xdr_bool(xdr, &info->archive))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_CANSETTIME) {
            if (!xdr_bool(xdr, &info->cansettime))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_CASE_INSENSITIVE) {
            if (!xdr_bool(xdr, &info->case_insensitive))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_CASE_PRESERVING) {
            if (!xdr_bool(xdr, &info->case_preserving))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_FILEID) {
            if (!xdr_u_hyper(xdr, &info->fileid))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_FS_LOCATIONS) {
            if (!decode_fs_locations4(xdr, info->fs_locations))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_HIDDEN) {
            if (!xdr_bool(xdr, &info->hidden))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_MAXREAD) {
            if (!xdr_u_hyper(xdr, &info->maxread))
                return FALSE;
        }
        if (attrs->attrmask.arr[0] & FATTR4_WORD0_MAXWRITE) {
            if (!xdr_u_hyper(xdr, &info->maxwrite))
                return FALSE;
        }
    }
    if (attrs->attrmask.count >= 2) {
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_MODE) {
            if (!xdr_u_int32_t(xdr, &info->mode))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_NUMLINKS) {
            if (!xdr_u_int32_t(xdr, &info->numlinks))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_OWNER) {
            char *ptr = &info->owner[0];
            uint32_t owner_len;
            if (!xdr_bytes(xdr, &ptr, &owner_len, 
                            NFS4_OPAQUE_LIMIT))
                return FALSE;
            info->owner[owner_len] = '\0';
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_OWNER_GROUP) {
            char *ptr = &info->owner_group[0];
            uint32_t owner_group_len;
            if (!xdr_bytes(xdr, &ptr, &owner_group_len, 
                            NFS4_OPAQUE_LIMIT))
                return FALSE;
            info->owner_group[owner_group_len] = '\0';
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_SPACE_AVAIL) {
            if (!xdr_u_hyper(xdr, &info->space_avail))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_SPACE_FREE) {
            if (!xdr_u_hyper(xdr, &info->space_free))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_SPACE_TOTAL) {
            if (!xdr_u_hyper(xdr, &info->space_total))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_SYSTEM) {
            if (!xdr_bool(xdr, &info->system))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_TIME_ACCESS) {
            if (!xdr_nfstime4(xdr, &info->time_access))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_TIME_CREATE) {
            if (!xdr_nfstime4(xdr, &info->time_create))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_TIME_DELTA) {
            if (!xdr_nfstime4(xdr, info->time_delta))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_TIME_MODIFY) {
            if (!xdr_nfstime4(xdr, &info->time_modify))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_DACL) {
            if (!xdr_nfsdacl41(xdr, info->acl))
                return FALSE;
        }
        if (attrs->attrmask.arr[1] & FATTR4_WORD1_FS_LAYOUT_TYPE) {
            if (!xdr_layout_types(xdr, &info->fs_layout_types))
                return FALSE;
        }
    }
    if (attrs->attrmask.count >= 3) {
        if (attrs->attrmask.arr[2] & FATTR4_WORD2_MDSTHRESHOLD) {
            if (!xdr_mdsthreshold(xdr, &info->mdsthreshold))
                return FALSE;
        }
        if (attrs->attrmask.arr[2] & FATTR4_WORD2_SUPPATTR_EXCLCREAT) {
            if (!xdr_bitmap4(xdr, info->suppattr_exclcreat))
                return FALSE;
        }
    }
    return TRUE;
}

static bool_t decode_op_getattr(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_getattr_res *res = (nfs41_getattr_res*)resop->res;

    if (unexpected_op(resop->op, OP_GETATTR))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
    {
        XDR attr_xdr;

        if (!xdr_fattr4(xdr, &res->obj_attributes))
            return FALSE;
        xdrmem_create(&attr_xdr, (char *)res->obj_attributes.attr_vals, res->obj_attributes.attr_vals_len, XDR_DECODE);
        return  decode_file_attrs(&attr_xdr, &res->obj_attributes, res->info);
    }
    return TRUE;
}


/*
 * OP_OPEN
 */
static bool_t encode_createhow4(
    XDR *xdr,
    createhow4 *ch)
{
    bool_t result = TRUE;

    if (!xdr_u_int32_t(xdr, &ch->mode))
        return FALSE;

    switch (ch->mode)
    {
    case UNCHECKED4:
    case GUARDED4:
        result = encode_createattrs4(xdr, ch->createattrs);
        break;
    case EXCLUSIVE4:
        result = xdr_opaque(xdr, (char *)ch->createverf, NFS4_VERIFIER_SIZE);
        break;
    case EXCLUSIVE4_1:
        if (!xdr_opaque(xdr, (char *)ch->createverf, NFS4_VERIFIER_SIZE))
            return FALSE;
        if (!encode_createattrs4(xdr, ch->createattrs))
            return FALSE;
        break;
    }
    return result;
}

static bool_t encode_openflag4(
    XDR *xdr,
    openflag4 *of)
{
    bool_t result = TRUE;

    if (!xdr_u_int32_t(xdr, &of->opentype))
        return FALSE;

    switch (of->opentype)
    {
    case OPEN4_CREATE:
        result = encode_createhow4(xdr, &of->how);
        break;
    default:
        break;
    }
    return result;
}

static bool_t encode_claim_deleg_cur(
    XDR *xdr,
    stateid4 *stateid,
    nfs41_component *name)
{
    if (!xdr_stateid4(xdr, stateid))
        return FALSE;
    return encode_component(xdr, name);
}

static bool_t encode_open_claim4(
    XDR *xdr,
    open_claim4 *oc)
{
    if (!xdr_u_int32_t(xdr, &oc->claim))
        return FALSE;

    switch (oc->claim)
    {
    case CLAIM_NULL:
        return encode_component(xdr, oc->u.null.filename);
    case CLAIM_PREVIOUS:
        return xdr_u_int32_t(xdr, &oc->u.prev.delegate_type);
    case CLAIM_FH:
        return TRUE; /* use current file handle */
    case CLAIM_DELEGATE_CUR:
        return encode_claim_deleg_cur(xdr,
            &oc->u.deleg_cur.delegate_stateid->stateid,
            oc->u.deleg_cur.name);
    case CLAIM_DELEG_CUR_FH:
        return xdr_stateid4(xdr,
            &oc->u.deleg_cur_fh.delegate_stateid->stateid);
    case CLAIM_DELEGATE_PREV:
        return encode_component(xdr, oc->u.deleg_prev.filename);
    case CLAIM_DELEG_PREV_FH:
        return TRUE; /* use current file handle */
    default:
        eprintf("encode_open_claim4: unsupported claim %d.\n",
            oc->claim);
        return FALSE;
    }
}

static bool_t encode_op_open(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_op_open_args *args = (nfs41_op_open_args*)argop->arg;

    if (unexpected_op(argop->op, OP_OPEN))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->seqid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->share_access))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->share_deny))
        return FALSE;

    if (!xdr_state_owner4(xdr, args->owner))
        return FALSE;

    if (!encode_openflag4(xdr, &args->openhow))
        return FALSE;

    return encode_open_claim4(xdr, args->claim);
}

static bool_t decode_open_none_delegation4(
    XDR *xdr,
    open_delegation4 *delegation)
{
    enum_t why_no_deleg;
    bool_t will_signal;

    if (!xdr_enum(xdr, (enum_t*)&why_no_deleg))
        return FALSE;

    switch (why_no_deleg)
    {
    case WND4_CONTENTION:
    case WND4_RESOURCE:
        return xdr_bool(xdr, &will_signal);
    default:
        return TRUE;
    }
}

static bool_t decode_open_read_delegation4(
    XDR *xdr,
    open_delegation4 *delegation)
{
    if (!xdr_stateid4(xdr, &delegation->stateid))
        return FALSE;

    if (!xdr_bool(xdr, &delegation->recalled))
        return FALSE;

    return xdr_nfsace4(xdr, &delegation->permissions);
}

static bool_t decode_modified_limit4(
    XDR *xdr,
    uint64_t *filesize)
{
    uint32_t blocks, bytes_per_block;

    if (!xdr_u_int32_t(xdr, &blocks))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &bytes_per_block))
        return FALSE;

    *filesize = blocks * bytes_per_block;
    return TRUE;
}

enum limit_by4 {
    NFS_LIMIT_SIZE          = 1,
    NFS_LIMIT_BLOCKS        = 2
};

static bool_t decode_space_limit4(
    XDR *xdr,
    uint64_t *filesize)
{
    uint32_t limitby;

    if (!xdr_u_int32_t(xdr, &limitby))
        return FALSE;

    switch (limitby)
    {
    case NFS_LIMIT_SIZE:
        return xdr_u_hyper(xdr, filesize);
    case NFS_LIMIT_BLOCKS:
        return decode_modified_limit4(xdr, filesize);
    default:
        eprintf("decode_space_limit4: limitby %d invalid\n", limitby);
        return FALSE;
    }
}

static bool_t decode_open_write_delegation4(
    XDR *xdr,
    open_delegation4 *delegation)
{
    uint64_t size_limit;

    if (!xdr_stateid4(xdr, &delegation->stateid))
        return FALSE;

    if (!xdr_bool(xdr, &delegation->recalled))
        return FALSE;

    if (!decode_space_limit4(xdr, &size_limit))
        return FALSE;

    return xdr_nfsace4(xdr, &delegation->permissions);
}

static bool_t decode_open_res_ok(
    XDR *xdr,
    nfs41_op_open_res_ok *res)
{
    if (!xdr_stateid4(xdr, res->stateid))
        return FALSE;

    if (!xdr_change_info4(xdr, &res->cinfo))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->rflags))
        return FALSE;

    if (!xdr_bitmap4(xdr, &res->attrset))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t*)&res->delegation->type))
        return FALSE;

    switch (res->delegation->type)
    {
    case OPEN_DELEGATE_NONE:
        return TRUE;
    case OPEN_DELEGATE_NONE_EXT:
        return decode_open_none_delegation4(xdr, res->delegation);
    case OPEN_DELEGATE_READ:
        return decode_open_read_delegation4(xdr, res->delegation);
    case OPEN_DELEGATE_WRITE:
        return decode_open_write_delegation4(xdr, res->delegation);
    default:
        eprintf("decode_open_res_ok: delegation type %d not "
            "supported.\n", res->delegation->type);
        return FALSE;
    }
}

static bool_t decode_op_open(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_op_open_res *res = (nfs41_op_open_res*)resop->res;

    if (unexpected_op(resop->op, OP_OPEN))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return decode_open_res_ok(xdr, &res->resok4);

    return TRUE;
}


/*
 * OP_OPENATTR
 */
static bool_t encode_op_openattr(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_openattr_args *args = (nfs41_openattr_args*)argop->arg;

    if (unexpected_op(argop->op, OP_OPENATTR))
        return FALSE;

    return xdr_bool(xdr, &args->createdir);
}

static bool_t decode_op_openattr(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_openattr_res *res = (nfs41_openattr_res*)resop->res;

    if (unexpected_op(resop->op, OP_OPENATTR))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_READ
 */
static bool_t encode_op_read(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_read_args *args = (nfs41_read_args*)argop->arg;

    if (unexpected_op(argop->op, OP_READ))
        return FALSE;

    if (!xdr_stateid4(xdr, &args->stateid->stateid))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->offset))
        return FALSE;

    return xdr_u_int32_t(xdr, &args->count);
}

static bool_t decode_read_res_ok(
    XDR *xdr,
    nfs41_read_res_ok *res)
{
    unsigned char *data = res->data;

    if (!xdr_bool(xdr, &res->eof))
        return FALSE;

    return xdr_bytes(xdr, (char **)&data, &res->data_len, NFS41_MAX_FILEIO_SIZE);
}

static bool_t decode_op_read(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_read_res *res = (nfs41_read_res*)resop->res;

    if (unexpected_op(resop->op, OP_READ))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return decode_read_res_ok(xdr, &res->resok4);

    return TRUE;
}


/*
 * OP_READDIR
 */
static bool_t encode_op_readdir(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_readdir_args *args = (nfs41_readdir_args*)argop->arg;

    if (unexpected_op(argop->op, OP_READDIR))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->cookie.cookie))
        return FALSE;

    if (!xdr_opaque(xdr, (char *)args->cookie.verf, NFS4_VERIFIER_SIZE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->dircount))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->maxcount))
        return FALSE;

    return xdr_bitmap4(xdr, args->attr_request);
}

typedef struct __readdir_entry_iterator {
    unsigned char   *buf_pos;
    uint32_t        remaining_len;
    uint32_t        *last_entry_offset;
    bool_t          ignore_the_rest;
    bool_t          has_next_entry;
} readdir_entry_iterator;

static bool_t decode_readdir_entry(
    XDR *xdr,
    readdir_entry_iterator *it)
{
    uint64_t cookie;
    unsigned char name[NFS4_OPAQUE_LIMIT];
    unsigned char *nameptr = &name[0];
    uint32_t name_len, entry_len;
    fattr4 attrs;

    /* decode into temporaries so we can determine if there's enough
     * room in the buffer for this entry */
    ZeroMemory(name, NFS4_OPAQUE_LIMIT);
    name_len = NFS4_OPAQUE_LIMIT;
    entry_len = (uint32_t)FIELD_OFFSET(nfs41_readdir_entry, name);
    attrs.attr_vals_len = NFS4_OPAQUE_LIMIT;

    if (!xdr_u_hyper(xdr, &cookie))
        return FALSE;

    if (!xdr_bytes(xdr, (char **)&nameptr, &name_len, NFS4_OPAQUE_LIMIT))
        return FALSE;

    if (!xdr_fattr4(xdr, &attrs))
        return FALSE;

    if (!xdr_bool(xdr, &it->has_next_entry))
        return FALSE;

    if (it->ignore_the_rest)
        return TRUE;

    name_len += 1; /* account for null terminator */
    if (entry_len + name_len <= it->remaining_len)
    {
        XDR fattr_xdr;
        nfs41_readdir_entry *entry = (nfs41_readdir_entry*)it->buf_pos;
        entry->cookie = cookie;
        entry->name_len = name_len;

        if (it->has_next_entry)
            entry->next_entry_offset = entry_len + name_len;
        else
            entry->next_entry_offset = 0;

        xdrmem_create(&fattr_xdr, (char *)attrs.attr_vals, attrs.attr_vals_len, XDR_DECODE);
        if (!(decode_file_attrs(&fattr_xdr, &attrs, &entry->attr_info)))
            entry->attr_info.rdattr_error = NFS4ERR_BADXDR;
        StringCchCopyA(entry->name, name_len, (STRSAFE_LPCSTR)name);

        it->buf_pos += entry_len + name_len;
        it->remaining_len -= entry_len + name_len;
        it->last_entry_offset = &entry->next_entry_offset;
    }
    else if (it->last_entry_offset)
    {
        *(it->last_entry_offset) = 0;
        it->ignore_the_rest = 1;
    }

    return TRUE;
}

static bool_t decode_readdir_list(
    XDR *xdr,
    nfs41_readdir_list *dirs)
{
    readdir_entry_iterator iter;
    iter.buf_pos = dirs->entries;
    iter.remaining_len = dirs->entries_len;
    iter.last_entry_offset = NULL;
    iter.ignore_the_rest = 0;
    iter.has_next_entry = 0;

    if (!xdr_bool(xdr, &dirs->has_entries))
        return FALSE;

    if (dirs->has_entries)
    {
        do {
            if (!decode_readdir_entry(xdr, &iter))
                return FALSE;

        } while (iter.has_next_entry);
    }
    dirs->entries_len -= iter.remaining_len;

    if (!xdr_bool(xdr, &dirs->eof))
        return FALSE;

    /* reset eof if we couldn't fit everything in the buffer */
    if (iter.ignore_the_rest)
        dirs->eof = 0;
    return TRUE;
}

static bool_t decode_op_readdir(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_readdir_res *res = (nfs41_readdir_res*)resop->res;

    if (unexpected_op(resop->op, OP_READDIR))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK) {
        if (!xdr_opaque(xdr, (char *)res->cookieverf, NFS4_VERIFIER_SIZE))
            return FALSE;
        return decode_readdir_list(xdr, &res->reply);
    }
    return TRUE;
}


/*
 * OP_READLINK
 */
static bool_t encode_op_readlink(
    XDR *xdr,
    nfs_argop4 *argop)
{
    if (unexpected_op(argop->op, OP_READLINK))
        return FALSE;

    /* void */
    return TRUE;
}

static bool_t decode_op_readlink(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_readlink_res *res = (nfs41_readlink_res*)resop->res;

    if (unexpected_op(resop->op, OP_READLINK))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK) {
        char *link = res->link;
        return xdr_bytes(xdr, &link, &res->link_len, res->link_len);
    }

    return TRUE;
}


/*
 * OP_REMOVE
 */
static bool_t encode_op_remove(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_remove_args *args = (nfs41_remove_args*)argop->arg;

    if (unexpected_op(argop->op, OP_REMOVE))
        return FALSE;

    return encode_component(xdr, args->target);
}

static bool_t decode_op_remove(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_remove_res *res = (nfs41_remove_res*)resop->res;

    if (unexpected_op(resop->op, OP_REMOVE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_change_info4(xdr, &res->cinfo);

    return TRUE;
}


/*
 * OP_RENAME
 */
static bool_t encode_op_rename(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_rename_args *args = (nfs41_rename_args*)argop->arg;

    if (unexpected_op(argop->op, OP_RENAME))
        return FALSE;

    if (!encode_component(xdr, args->oldname))
        return FALSE;

    return encode_component(xdr, args->newname);
}

static bool_t decode_op_rename(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_rename_res *res = (nfs41_rename_res*)resop->res;

    if (unexpected_op(resop->op, OP_RENAME))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
    {
        if (!xdr_change_info4(xdr, &res->source_cinfo))
            return FALSE;
        return xdr_change_info4(xdr, &res->target_cinfo);
    }
    return TRUE;
}


/*
 * OP_RESTOREFH
 */
static bool_t encode_op_restorefh(
    XDR *xdr,
    nfs_argop4 *argop)
{
    if (unexpected_op(argop->op, OP_RESTOREFH))
        return FALSE;

    /* void */
    return TRUE;
}

static bool_t decode_op_restorefh(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_restorefh_res *res = (nfs41_restorefh_res*)resop->res;

    if (unexpected_op(resop->op, OP_RESTOREFH))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_SAVEFH
 */
static bool_t encode_op_savefh(
    XDR *xdr,
    nfs_argop4 *argop)
{
    if (unexpected_op(argop->op, OP_SAVEFH))
        return FALSE;

    /* void */
    return TRUE;
}

static bool_t decode_op_savefh(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_savefh_res *res = (nfs41_savefh_res*)resop->res;

    if (unexpected_op(resop->op, OP_SAVEFH))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_SETATTR
 */
static bool_t encode_file_attrs(
    fattr4 *attrs,
    nfs41_file_info *info)
{
    uint32_t i;
    XDR localxdr;

    xdrmem_create(&localxdr, (char *)attrs->attr_vals, NFS4_OPAQUE_LIMIT, XDR_ENCODE);

    attrs->attr_vals_len = 0;
    ZeroMemory(&attrs->attrmask, sizeof(bitmap4));
    attrs->attrmask.count = info->attrmask.count;

    if (info->attrmask.count > 0) {
        if (info->attrmask.arr[0] & FATTR4_WORD0_SIZE) {
            if (!xdr_u_hyper(&localxdr, &info->size))
                return FALSE;
            attrs->attrmask.arr[0] |= FATTR4_WORD0_SIZE;
        }
        if (info->attrmask.arr[0] & FATTR4_WORD0_ACL) {
            if (!xdr_nfsacl41(&localxdr, info->acl))
                return FALSE;
            attrs->attrmask.arr[0] |= FATTR4_WORD0_ACL;
        }
        if (info->attrmask.arr[0] & FATTR4_WORD0_ARCHIVE) {
            if (!xdr_bool(&localxdr, &info->archive))
                return FALSE;
            attrs->attrmask.arr[0] |= FATTR4_WORD0_ARCHIVE;
        }
        if (info->attrmask.arr[0] & FATTR4_WORD0_HIDDEN) {
            if (!xdr_bool(&localxdr, &info->hidden))
                return FALSE;
            attrs->attrmask.arr[0] |= FATTR4_WORD0_HIDDEN;
        }
    }
    if (info->attrmask.count > 1) {
        if (info->attrmask.arr[1] & FATTR4_WORD1_MODE) {
            if (!xdr_u_int32_t(&localxdr, &info->mode))
                return FALSE;
            attrs->attrmask.arr[1] |= FATTR4_WORD1_MODE;
        }
        if (info->attrmask.arr[1] & FATTR4_WORD1_SYSTEM) {
            if (!xdr_bool(&localxdr, &info->system))
                return FALSE;
            attrs->attrmask.arr[1] |= FATTR4_WORD1_SYSTEM;
        }
        if (info->attrmask.arr[1] & FATTR4_WORD1_TIME_ACCESS_SET) {
            if (!xdr_settime4(&localxdr, &info->time_access, info->time_delta))
                return FALSE;
            attrs->attrmask.arr[1] |= FATTR4_WORD1_TIME_ACCESS_SET;
        }
        if (info->attrmask.arr[1] & FATTR4_WORD1_TIME_CREATE) {
            if (!xdr_nfstime4(&localxdr, &info->time_create))
                return FALSE;
            attrs->attrmask.arr[1] |= FATTR4_WORD1_TIME_CREATE;
        }
        if (info->attrmask.arr[1] & FATTR4_WORD1_TIME_MODIFY_SET) {
            if (!xdr_settime4(&localxdr, &info->time_modify, info->time_delta))
                return FALSE;
            attrs->attrmask.arr[1] |= FATTR4_WORD1_TIME_MODIFY_SET;
        }
        if (info->attrmask.arr[1] & FATTR4_WORD1_OWNER) {
            char *ptr = &info->owner[0];
            uint32_t owner_len = (uint32_t)strlen(info->owner);
            if (!xdr_bytes(&localxdr, &ptr, &owner_len, 
                            NFS4_OPAQUE_LIMIT))
                return FALSE;
            attrs->attrmask.arr[1] |= FATTR4_WORD1_OWNER;
        }
        if (info->attrmask.arr[1] & FATTR4_WORD1_OWNER_GROUP) {
            char *ptr = &info->owner_group[0];
            uint32_t owner_group_len = (uint32_t)strlen(info->owner_group);
            if (!xdr_bytes(&localxdr, &ptr, &owner_group_len, 
                            NFS4_OPAQUE_LIMIT))
                return FALSE;
            attrs->attrmask.arr[1] |= FATTR4_WORD1_OWNER_GROUP;
        }
    }
    if (info->attrmask.count > 2) {
        if (info->attrmask.arr[2] & FATTR4_WORD2_MODE_SET_MASKED) {
            if (!xdr_u_int32_t(&localxdr, &info->mode))
                return FALSE;
            if (!xdr_u_int32_t(&localxdr, &info->mode_mask))
                return FALSE;
            attrs->attrmask.arr[2] |= FATTR4_WORD2_MODE_SET_MASKED;
        }
    }

    /* warn if we try to set attributes that aren't handled */
    for (i = 0; i < info->attrmask.count; i++)
        if (attrs->attrmask.arr[i] != info->attrmask.arr[i])
            eprintf("encode_file_attrs() attempted to encode extra "
                "attributes in arr[%d]: encoded %d, but wanted %d.\n",
                i, attrs->attrmask.arr[i], info->attrmask.arr[i]);

    attrs->attr_vals_len = xdr_getpos(&localxdr);
    return TRUE;
}

static bool_t encode_op_setattr(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_setattr_args *args = (nfs41_setattr_args*)argop->arg;
    fattr4 attrs;

    if (unexpected_op(argop->op, OP_SETATTR))
        return FALSE;

    if (!xdr_stateid4(xdr, &args->stateid->stateid))
        return FALSE;

    /* encode attribute values from args->info into attrs.attr_vals */
    attrs.attr_vals_len = NFS4_OPAQUE_LIMIT;
    if (!encode_file_attrs(&attrs, args->info))
        return FALSE;

    return xdr_fattr4(xdr, &attrs);
}

static bool_t decode_op_setattr(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_setattr_res *res = (nfs41_setattr_res*)resop->res;

    if (unexpected_op(resop->op, OP_SETATTR))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_bitmap4(xdr, &res->attrsset);

    return TRUE;
}


/*
 * OP_WANT_DELEGATION
 */
static bool_t encode_op_want_delegation(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_want_delegation_args *args = (nfs41_want_delegation_args*)argop->arg;

    if (unexpected_op(argop->op, OP_WANT_DELEGATION))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->want))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->claim->claim))
        return FALSE;

    return args->claim->claim != CLAIM_PREVIOUS ||
        xdr_u_int32_t(xdr, &args->claim->prev_delegate_type);
}

static bool_t decode_op_want_delegation(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_want_delegation_res *res = (nfs41_want_delegation_res*)resop->res;

    if (unexpected_op(resop->op, OP_WANT_DELEGATION))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status)
        return TRUE;

    if (!xdr_enum(xdr, (enum_t*)&res->delegation->type))
        return FALSE;

    switch (res->delegation->type)
    {
    case OPEN_DELEGATE_NONE:
        return TRUE;
    case OPEN_DELEGATE_NONE_EXT:
        return decode_open_none_delegation4(xdr, res->delegation);
    case OPEN_DELEGATE_READ:
        return decode_open_read_delegation4(xdr, res->delegation);
    case OPEN_DELEGATE_WRITE:
        return decode_open_write_delegation4(xdr, res->delegation);
    default:
        eprintf("decode_open_res_ok: delegation type %d not "
            "supported.\n", res->delegation->type);
        return FALSE;
    }
}


/*
 * OP_FREE_STATEID
 */
static bool_t encode_op_free_stateid(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_free_stateid_args *args = (nfs41_free_stateid_args*)argop->arg;

    if (unexpected_op(argop->op, OP_FREE_STATEID))
        return FALSE;

    return xdr_stateid4(xdr, args->stateid);
}

static bool_t decode_op_free_stateid(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_free_stateid_res *res = (nfs41_free_stateid_res*)resop->res;

    if (unexpected_op(resop->op, OP_FREE_STATEID))
        return FALSE;

    return xdr_u_int32_t(xdr, &res->status);
}


/*
 * OP_TEST_STATEID
 */
static bool_t encode_op_test_stateid(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_test_stateid_args *args = (nfs41_test_stateid_args*)argop->arg;

    if (unexpected_op(argop->op, OP_TEST_STATEID))
        return FALSE;

    return xdr_array(xdr, (char**)&args->stateids, &args->count,
        args->count, sizeof(stateid_arg), (xdrproc_t)xdr_stateid4);
}

static bool_t decode_op_test_stateid(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_test_stateid_res *res = (nfs41_test_stateid_res*)resop->res;

    if (unexpected_op(resop->op, OP_TEST_STATEID))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK) {
        return xdr_array(xdr, (char**)&res->resok.status, &res->resok.count,
            res->resok.count, sizeof(uint32_t), (xdrproc_t)xdr_u_int32_t);
    }
    return TRUE;
}


/*
 * OP_WRITE
 */
static bool_t encode_op_write(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_write_args *args = (nfs41_write_args*)argop->arg;
    unsigned char *data = args->data;

    if (unexpected_op(argop->op, OP_WRITE))
        return FALSE;

    if (!xdr_stateid4(xdr, &args->stateid->stateid))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->offset))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->stable))
        return FALSE;

    return xdr_bytes(xdr, (char **)&data, &args->data_len, NFS41_MAX_FILEIO_SIZE);
}

static bool_t xdr_write_verf(
    XDR *xdr,
    nfs41_write_verf *verf)
{
    if (!xdr_enum(xdr, (enum_t *)&verf->committed))
        return FALSE;

    return xdr_opaque(xdr, (char *)verf->verf, NFS4_VERIFIER_SIZE);
}

static bool_t xdr_write_res_ok(
    XDR *xdr,
    nfs41_write_res_ok *res)
{
    if (!xdr_u_int32_t(xdr, &res->count))
        return FALSE;

    return xdr_write_verf(xdr, res->verf);
}

static bool_t decode_op_write(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_write_res *res = (nfs41_write_res*)resop->res;

    if (unexpected_op(resop->op, OP_WRITE))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_write_res_ok(xdr, &res->resok4);

    return TRUE;
}

/* 
 * OP_SECINFO_NO_NAME
 */
static bool_t xdr_secinfo(
    XDR *xdr,
    nfs41_secinfo_info *secinfo)
{
    if (!xdr_u_int32_t(xdr, &secinfo->sec_flavor))
        return FALSE;
    if (secinfo->sec_flavor == RPCSEC_GSS) {
        char *p = secinfo->oid;
        if (!xdr_bytes(xdr, (char **)&p, &secinfo->oid_len, MAX_OID_LEN))
            return FALSE;
        if (!xdr_u_int32_t(xdr, &secinfo->qop))
            return FALSE;
        if (!xdr_enum(xdr, (enum_t *)&secinfo->type))
            return FALSE;
    }
    return TRUE;
}

static bool_t encode_op_secinfo_noname(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_secinfo_noname_args *args = (nfs41_secinfo_noname_args *)argop->arg;

    if (unexpected_op(argop->op, OP_SECINFO_NO_NAME))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&args->type))
        return FALSE;

    return TRUE;
}

static bool_t decode_op_secinfo_noname(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_secinfo_noname_res *res = (nfs41_secinfo_noname_res *)resop->res;
    nfs41_secinfo_info *secinfo = res->secinfo;
    if (unexpected_op(resop->op, OP_SECINFO_NO_NAME))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_array(xdr, (char**)&secinfo, &res->count,
            MAX_SECINFOS, sizeof(nfs41_secinfo_info), (xdrproc_t)xdr_secinfo);

    return TRUE;
}

/* 
 * OP_SECINFO
 */
static bool_t encode_op_secinfo(
    XDR *xdr,
    nfs_argop4 *argop)
{
    nfs41_secinfo_args *args = (nfs41_secinfo_args *)argop->arg;

    if (unexpected_op(argop->op, OP_SECINFO))
        return FALSE;

    if (!encode_component(xdr, args->name))
        return FALSE;

    return TRUE;
}

static bool_t decode_op_secinfo(
    XDR *xdr,
    nfs_resop4 *resop)
{
    nfs41_secinfo_noname_res *res = (nfs41_secinfo_noname_res *)resop->res;
    nfs41_secinfo_info *secinfo = res->secinfo;

    if (unexpected_op(resop->op, OP_SECINFO))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK)
        return xdr_array(xdr, (char**)&secinfo, &res->count,
            MAX_SECINFOS, sizeof(nfs41_secinfo_info), (xdrproc_t)xdr_secinfo);

    return TRUE;
}
/*
 * OP_GETDEVICEINFO
 */
static bool_t encode_op_getdeviceinfo(
    XDR *xdr,
    nfs_argop4 *argop)
{
    pnfs_getdeviceinfo_args *args = (pnfs_getdeviceinfo_args*)argop->arg;

    if (unexpected_op(argop->op, OP_GETDEVICEINFO))
        return FALSE;

    if (!xdr_opaque(xdr, (char *)args->deviceid, PNFS_DEVICEID_SIZE))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&args->layout_type))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->maxcount))
        return FALSE;

    return xdr_bitmap4(xdr, &args->notify_types);
}

static bool_t xdr_stripe_indices(
    XDR *xdr,
    pnfs_stripe_indices *indices)
{
    uint32_t i, count;

    if (!xdr_u_int32_t(xdr, &count))
        return FALSE;

    if (count && count != indices->count) {
        uint32_t *tmp;
        tmp = realloc(indices->arr, count * sizeof(uint32_t));
        if (tmp == NULL)
            return FALSE;
        indices->arr = tmp;
        ZeroMemory(indices->arr, count * sizeof(uint32_t));
        indices->count = count;
    }
    
    for (i = 0; i < indices->count; i++) {
        if (!xdr_u_int32_t(xdr, &indices->arr[i]))
            return FALSE;
    }
    return TRUE;
}

static bool_t xdr_pnfs_addr(
    XDR *xdr,
    netaddr4 *addr)
{
    uint32_t len;
    char *netid = addr->netid;
    char *uaddr = addr->uaddr;

    if (xdr->x_op == XDR_ENCODE)
        len = sizeof(addr->netid);
    if (!xdr_bytes(xdr, &netid, &len, NFS41_NETWORK_ID_LEN))
        return FALSE;

    if (xdr->x_op == XDR_DECODE) {
        if (len < NFS41_NETWORK_ID_LEN)
            addr->netid[len] = 0;
        else
            addr->netid[NFS41_NETWORK_ID_LEN] = 0;
    }

    if (xdr->x_op == XDR_ENCODE)
        len = sizeof(addr->uaddr);
    if (!xdr_bytes(xdr, &uaddr, &len, NFS41_UNIVERSAL_ADDR_LEN))
        return FALSE;

    if (xdr->x_op == XDR_DECODE){
        if (len < NFS41_UNIVERSAL_ADDR_LEN)
            addr->uaddr[len] = 0;
        else
            addr->uaddr[NFS41_UNIVERSAL_ADDR_LEN] = 0;
    }

    return TRUE;
}

static bool_t xdr_multi_addr(
    XDR *xdr,
    multi_addr4 *list)
{
    netaddr4 dummy, *dest;
    uint32_t i;

    if (!xdr_u_int32_t(xdr, &list->count))
        return FALSE;

    for (i = 0; i < list->count; i++) {
        /* if there are too many addrs, decode the extras into 'dummy' */
        dest = i < NFS41_ADDRS_PER_SERVER ? &list->arr[i] : &dummy;

        if (!xdr_pnfs_addr(xdr, dest))
            return FALSE;
    }
    return TRUE;
}

static bool_t xdr_data_server_list(
    XDR *xdr,
    pnfs_data_server_list *servers)
{
    uint32_t i, count;

    if (!xdr_u_int32_t(xdr, &count))
        return FALSE;

    if (count && count != servers->count) {
        pnfs_data_server *tmp;
        /* clear data server clients; they're still cached with nfs41_root,
         * so pnfs_data_server_client() will look them up again */
        for (i = 0; i < servers->count; i++)
            servers->arr[i].client = NULL;

        tmp = realloc(servers->arr, count * sizeof(pnfs_data_server));
        if (tmp == NULL) 
            return FALSE;
        servers->arr = tmp;
        ZeroMemory(servers->arr, count * sizeof(pnfs_data_server));
        for (i = servers->count; i < count; i++) /* initialize new elements */
            InitializeSRWLock(&servers->arr[i].lock);
        servers->count = count;
    }

    for (i = 0; i < servers->count; i++) {
        if (!xdr_multi_addr(xdr, &servers->arr[i].addrs))
            return FALSE;
    }
    return TRUE;
}

static bool_t xdr_file_device(
    XDR *xdr,
    pnfs_file_device *device)
{
    if (!xdr_stripe_indices(xdr, &device->stripes))
        return FALSE;

    return xdr_data_server_list(xdr, &device->servers);
}

static bool_t decode_getdeviceinfo_ok(
    XDR *xdr,
    pnfs_getdeviceinfo_res_ok *res_ok)
{
    u_int32_t len_ignored;

    if (!xdr_enum(xdr, (enum_t *)&res_ok->device->device.type))
        return FALSE;

    if (res_ok->device->device.type != PNFS_LAYOUTTYPE_FILE)
        return FALSE;

    if (!xdr_u_int32_t(xdr, &len_ignored))
        return FALSE;

    if (!xdr_file_device(xdr, res_ok->device))
        return FALSE;

    return xdr_bitmap4(xdr, &res_ok->notification);
}

static bool_t decode_op_getdeviceinfo(
    XDR *xdr,
    nfs_resop4 *resop)
{
    pnfs_getdeviceinfo_res *res = (pnfs_getdeviceinfo_res*)resop->res;

    if (unexpected_op(resop->op, OP_GETDEVICEINFO))
        return FALSE;

    if (!xdr_u_int32_t(xdr, (uint32_t *)&res->status))
        return FALSE;

    switch (res->status) {
    case NFS4_OK:
        return decode_getdeviceinfo_ok(xdr, &res->u.res_ok);
        break;
    case NFS4ERR_TOOSMALL:
        {
            uint32_t ignored;
            return xdr_u_int32_t(xdr, &ignored);
        }
        break;
    }
    return TRUE;
}


/*
 * OP_LAYOUTCOMMIT
 */
static bool_t encode_op_layoutcommit(
    XDR *xdr,
    nfs_argop4 *argop)
{
    pnfs_layoutcommit_args *args = (pnfs_layoutcommit_args*)argop->arg;
    bool_t false_bool = FALSE;
    bool_t true_bool = TRUE;
    enum_t pnfs_layout = PNFS_LAYOUTTYPE_FILE;
    uint32_t zero = 0;

    if (unexpected_op(argop->op, OP_LAYOUTCOMMIT))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->offset))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->length))
        return FALSE;

    if (!xdr_bool(xdr, &false_bool)) /* loca_reclaim = 0 */
        return FALSE;

    if (!xdr_stateid4(xdr, args->stateid))
        return FALSE;

    /* loca_last_write_offset */
    if (args->new_offset) {
        if (!xdr_bool(xdr, &true_bool))
            return FALSE;

        if (!xdr_u_hyper(xdr, args->new_offset))
            return FALSE;
    } else {
        if (!xdr_bool(xdr, &false_bool))
            return FALSE;
    }

    /* loca_time_modify */
    if (args->new_time) {
        if (!xdr_bool(xdr, &true_bool))
            return FALSE;

        if (!xdr_nfstime4(xdr, args->new_time))
            return FALSE;
    } else {
        if (!xdr_bool(xdr, &false_bool))
            return FALSE;
    }

    /* loca_layoutupdate */
    if (!xdr_enum(xdr, &pnfs_layout))
        return FALSE;

    return xdr_u_int32_t(xdr, &zero);
}

static bool_t decode_op_layoutcommit(
    XDR *xdr,
    nfs_resop4 *resop)
{
    pnfs_layoutcommit_res *res = (pnfs_layoutcommit_res*)resop->res;

    if (unexpected_op(resop->op, OP_LAYOUTCOMMIT))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (res->status == NFS4_OK) {
        if (!xdr_bool(xdr, &res->has_new_size))
            return FALSE;

        if (res->has_new_size)
            if (!xdr_u_hyper(xdr, &res->new_size))
                return FALSE;
    }
    return TRUE;
}

/*
 * OP_LAYOUTGET
 */
static bool_t encode_op_layoutget(
    XDR *xdr,
    nfs_argop4 *argop)
{
    pnfs_layoutget_args *args = (pnfs_layoutget_args*)argop->arg;

    if (unexpected_op(argop->op, OP_LAYOUTGET))
        return FALSE;

    if (!xdr_bool(xdr, &args->signal_layout_avail))
        return FALSE;

    if (!xdr_u_int32_t(xdr, (u_int32_t *)&args->layout_type))
        return FALSE;

    if (!xdr_u_int32_t(xdr, (u_int32_t *)&args->iomode))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->offset))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->length))
        return FALSE;

    if (!xdr_u_hyper(xdr, &args->minlength))
        return FALSE;

    if (!xdr_stateid4(xdr, &args->stateid->stateid))
        return FALSE;

    return xdr_u_int32_t(xdr, &args->maxcount);
}

static bool_t decode_file_layout_handles(
    XDR *xdr,
    pnfs_file_layout_handles *handles)
{
    uint32_t i, count;

    if (!xdr_u_int32_t(xdr, &count))
        return FALSE;

    if (count && count != handles->count) {
        nfs41_path_fh *tmp;
        tmp = realloc(handles->arr, count * sizeof(nfs41_path_fh));
        if (tmp == NULL)
            return FALSE;
        handles->arr = tmp;
        ZeroMemory(handles->arr, count * sizeof(nfs41_path_fh));
        handles->count = count;
    }
    
    for (i = 0; i < handles->count; i++) {
        if (!xdr_fh(xdr, &handles->arr[i].fh))
            return FALSE;
    }
    return TRUE;
}

static bool_t decode_file_layout(
    XDR *xdr,
    struct list_entry *list,
    pnfs_layout *base)
{
    pnfs_file_layout *layout;
    u_int32_t len_ignored;

    if (!xdr_u_int32_t(xdr, &len_ignored))
        return FALSE;

    layout = calloc(1, sizeof(pnfs_file_layout));
    if (layout == NULL)
        return FALSE;

    layout->layout.offset = base->offset;
    layout->layout.length = base->length;
    layout->layout.iomode = base->iomode;
    layout->layout.type = base->type;
    list_init(&layout->layout.entry);

    if (!xdr_opaque(xdr, (char *)layout->deviceid, PNFS_DEVICEID_SIZE))
        goto out_error;

    if (!xdr_u_int32_t(xdr, &layout->util))
        goto out_error;

    if (!xdr_u_int32_t(xdr, &layout->first_index))
        goto out_error;

    if (!xdr_u_hyper(xdr, &layout->pattern_offset))
        goto out_error;

    if (!decode_file_layout_handles(xdr, &layout->filehandles))
        goto out_error;

    list_add_tail(list, &layout->layout.entry);
    return TRUE;

out_error:
    free(layout);
    return FALSE;
}

static bool_t decode_layout(
    XDR *xdr,
    struct list_entry *list)
{
    pnfs_layout layout;

    if (!xdr_u_hyper(xdr, &layout.offset))
        return FALSE;

    if (!xdr_u_hyper(xdr, &layout.length))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&layout.iomode))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&layout.type))
        return FALSE;

    switch (layout.type) {
    case PNFS_LAYOUTTYPE_FILE:
        return decode_file_layout(xdr, list, &layout);

    default:
        eprintf("%s: received non-FILE layout type, %d\n",
            "decode_file_layout", layout.type);
    }
    return FALSE;
}

static bool_t decode_layout_res_ok(
    XDR *xdr,
    pnfs_layoutget_res_ok *res)
{
    uint32_t i;

    if (!xdr_bool(xdr, &res->return_on_close))
        return FALSE;

    if (!xdr_stateid4(xdr, &res->stateid))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &res->count))
        return FALSE;

    for (i = 0; i < res->count; i++)
        if (!decode_layout(xdr, &res->layouts))
            return FALSE;
    return TRUE;
}

static bool_t decode_op_layoutget(
    XDR *xdr,
    nfs_resop4 *resop)
{
    pnfs_layoutget_res *res = (pnfs_layoutget_res*)resop->res;

    if (unexpected_op(resop->op, OP_LAYOUTGET))
        return FALSE;

    if (!xdr_u_int32_t(xdr, (uint32_t *)&res->status))
        return FALSE;

    switch (res->status) {
    case NFS4_OK:
        return decode_layout_res_ok(xdr, res->u.res_ok);
    case NFS4ERR_LAYOUTTRYLATER:
        return xdr_bool(xdr, &res->u.will_signal_layout_avail);
    }
    return TRUE;
}


/*
 * OP_LAYOUTRETURN
 */
static bool_t encode_op_layoutreturn(
    XDR *xdr,
    nfs_argop4 *argop)
{
    pnfs_layoutreturn_args *args = (pnfs_layoutreturn_args*)argop->arg;

    if (unexpected_op(argop->op, OP_LAYOUTRETURN))
        return FALSE;

    if (!xdr_bool(xdr, &args->reclaim))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&args->type))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&args->iomode))
        return FALSE;

    if (!xdr_enum(xdr, (enum_t *)&args->return_type))
        return FALSE;

    if (args->return_type == PNFS_RETURN_FILE) {
        u_int32_t zero = 0;

        if (!xdr_u_hyper(xdr, &args->offset))
            return FALSE;

        if (!xdr_u_hyper(xdr, &args->length))
            return FALSE;

        if (!xdr_stateid4(xdr, args->stateid))
            return FALSE;

        return xdr_u_int32_t(xdr, &zero); /* size of lrf_body is 0 */
    } else {
        eprintf("%s: layout type (%d) is not PNFS_RETURN_FILE!\n",
            "encode_op_layoutreturn", args->return_type);
        return FALSE;
    }
}

static bool_t decode_op_layoutreturn(
    XDR *xdr,
    nfs_resop4 *resop)
{
    pnfs_layoutreturn_res *res = (pnfs_layoutreturn_res*)resop->res;

    if (unexpected_op(resop->op, OP_LAYOUTRETURN))
        return FALSE;

    if (!xdr_u_int32_t(xdr, (uint32_t *)&res->status))
        return FALSE;

    if (res->status == NFS4_OK) {
        if (!xdr_bool(xdr, &res->stateid_present))
            return FALSE;

        if (res->stateid_present)
            return xdr_stateid4(xdr, &res->stateid);
    }
    return TRUE;
}


/* op encode/decode table */
typedef bool_t (*nfs_op_encode_proc)(XDR*, nfs_argop4*);
typedef bool_t (*nfs_op_decode_proc)(XDR*, nfs_resop4*);

typedef struct __op_table_entry {
    nfs_op_encode_proc      encode;
    nfs_op_decode_proc      decode;
} op_table_entry;

/* table of encode/decode functions, indexed by operation number */
static const op_table_entry g_op_table[] = {
    { NULL, NULL }, /* 0 unused */
    { NULL, NULL }, /* 1 unused */
    { NULL, NULL }, /* 2 unused */
    { encode_op_access, decode_op_access }, /* OP_ACCESS = 3 */
    { encode_op_close, decode_op_close }, /* OP_CLOSE = 4 */
    { encode_op_commit, decode_op_commit }, /* OP_COMMIT = 5 */
    { encode_op_create, decode_op_create }, /* OP_CREATE = 6 */
    { encode_op_delegpurge, decode_op_delegpurge }, /* OP_DELEGPURGE = 7 */
    { encode_op_delegreturn, decode_op_delegreturn }, /* OP_DELEGRETURN = 8 */
    { encode_op_getattr, decode_op_getattr }, /* OP_GETATTR = 9 */
    { encode_op_getfh, decode_op_getfh }, /* OP_GETFH = 10 */
    { encode_op_link, decode_op_link }, /* OP_LINK = 11 */
    { encode_op_lock, decode_op_lock }, /* OP_LOCK = 12 */
    { encode_op_lockt, decode_op_lockt }, /* OP_LOCKT = 13 */
    { encode_op_locku, decode_op_locku }, /* OP_LOCKU = 14 */
    { encode_op_lookup, decode_op_lookup }, /* OP_LOOKUP = 15 */
    { NULL, NULL }, /* OP_LOOKUPP = 16 */
    { NULL, NULL }, /* OP_NVERIFY = 17 */
    { encode_op_open, decode_op_open }, /* OP_OPEN = 18 */
    { encode_op_openattr, decode_op_openattr }, /* OP_OPENATTR = 19 */
    { NULL, NULL }, /* OP_OPEN_CONFIRM = 20 */
    { NULL, NULL }, /* OP_OPEN_DOWNGRADE = 21 */
    { encode_op_putfh, decode_op_putfh }, /* OP_PUTFH = 22 */
    { NULL, NULL }, /* OP_PUTPUBFH = 23 */
    { encode_op_putrootfh, decode_op_putrootfh }, /* OP_PUTROOTFH = 24 */
    { encode_op_read, decode_op_read }, /* OP_READ = 25 */
    { encode_op_readdir, decode_op_readdir }, /* OP_READDIR = 26 */
    { encode_op_readlink, decode_op_readlink }, /* OP_READLINK = 27 */
    { encode_op_remove, decode_op_remove }, /* OP_REMOVE = 28 */
    { encode_op_rename, decode_op_rename }, /* OP_RENAME = 29 */
    { NULL, NULL }, /* OP_RENEW = 30 */
    { encode_op_restorefh, decode_op_restorefh }, /* OP_RESTOREFH = 31 */
    { encode_op_savefh, decode_op_savefh }, /* OP_SAVEFH = 32 */
    { encode_op_secinfo, decode_op_secinfo }, /* OP_SECINFO = 33 */
    { encode_op_setattr, decode_op_setattr }, /* OP_SETATTR = 34 */
    { NULL, NULL }, /* OP_SETCLIENTID = 35 */
    { NULL, NULL }, /* OP_SETCLIENTID_CONFIRM  = 36 */
    { NULL, NULL }, /* OP_VERIFY = 37 */
    { encode_op_write, decode_op_write }, /* OP_WRITE = 38 */
    { NULL, NULL }, /* OP_RELEASE_LOCKOWNER = 39 */
    { NULL, NULL }, /* OP_BACKCHANNEL_CTL = 40 */
    { encode_op_bind_conn_to_session, decode_op_bind_conn_to_session }, /* OP_BIND_CONN_TO_SESSION = 41 */
    { encode_op_exchange_id, decode_op_exchange_id }, /* OP_EXCHANGE_ID = 42 */
    { encode_op_create_session, decode_op_create_session }, /* OP_CREATE_SESSION = 43 */
    { encode_op_destroy_session, decode_op_destroy_session }, /* OP_DESTROY_SESSION = 44 */
    { encode_op_free_stateid, decode_op_free_stateid }, /* OP_FREE_STATEID = 45 */
    { NULL, NULL }, /* OP_GET_DIR_DELEGATION = 46 */
    { encode_op_getdeviceinfo, decode_op_getdeviceinfo }, /* OP_GETDEVICEINFO = 47 */
    { NULL, NULL }, /* OP_GETDEVICELIST = 48 */
    { encode_op_layoutcommit, decode_op_layoutcommit }, /* OP_LAYOUTCOMMIT = 49 */
    { encode_op_layoutget, decode_op_layoutget }, /* OP_LAYOUTGET = 50 */
    { encode_op_layoutreturn, decode_op_layoutreturn }, /* OP_LAYOUTRETURN = 51 */
    { encode_op_secinfo_noname, decode_op_secinfo_noname }, /* OP_SECINFO_NO_NAME = 52 */
    { encode_op_sequence, decode_op_sequence }, /* OP_SEQUENCE = 53 */
    { NULL, NULL }, /* OP_SET_SSV = 54 */
    { encode_op_test_stateid, decode_op_test_stateid }, /* OP_TEST_STATEID = 55 */
    { encode_op_want_delegation, decode_op_want_delegation }, /* OP_WANT_DELEGATION = 56 */
    { encode_op_destroy_clientid, decode_op_destroy_clientid }, /* OP_DESTROY_CLIENTID = 57 */
    { encode_op_reclaim_complete, decode_op_reclaim_complete }, /* OP_RECLAIM_COMPLETE = 58 */
};
#ifdef __REACTOS__
static const uint32_t g_op_table_size = (sizeof(g_op_table) / sizeof(g_op_table[0]));
#else
static const uint32_t g_op_table_size = ARRAYSIZE(g_op_table);
#endif

static const op_table_entry* op_table_find(uint32_t op)
{
    return op >= g_op_table_size ? NULL : &g_op_table[op];
}


/*
 * COMPOUND
 */
bool_t nfs_encode_compound(
    XDR *xdr,
    caddr_t *pargs)
{
    unsigned char *tag;

    nfs41_compound_args *args = (nfs41_compound_args*)pargs;
    uint32_t i;
    const op_table_entry *entry;

    tag = args->tag;
    if (!xdr_bytes(xdr, (char **)&tag, &args->tag_len, NFS4_OPAQUE_LIMIT))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->minorversion))
        return FALSE;

    if (!xdr_u_int32_t(xdr, &args->argarray_count))
        return FALSE;

    for (i = 0; i < args->argarray_count; i++)
    {
        entry = op_table_find(args->argarray[i].op);
        if (entry == NULL || entry->encode == NULL)
            return FALSE;

        if (!xdr_u_int32_t(xdr, &args->argarray[i].op))
            return FALSE;
        if (!entry->encode(xdr, &args->argarray[i]))
            return FALSE;
    }
    return TRUE;
}

bool_t nfs_decode_compound(
    XDR *xdr,
    caddr_t *pres)
{
    nfs41_compound_res *res = (nfs41_compound_res*)pres;
    uint32_t i, expected_count, expected_op;
    const op_table_entry *entry;
    unsigned char *tag = res->tag;

    if (!xdr_u_int32_t(xdr, &res->status))
        return FALSE;

    if (!xdr_bytes(xdr, (char **)&tag, &res->tag_len, NFS4_OPAQUE_LIMIT))
        return FALSE;

    expected_count = res->resarray_count;
    if (!xdr_u_int32_t(xdr, &res->resarray_count))
        return FALSE;

    /* validate the number of operations against what we sent */
    if (res->resarray_count > expected_count) {
        eprintf("reply with %u operations, only sent %u!\n",
            res->resarray_count, expected_count);
        return FALSE;
    } else if (res->resarray_count < expected_count &&
        res->status == NFS4_OK) {
        /* illegal for a server to reply with less operations,
         * unless one of them fails */
        eprintf("successful reply with only %u operations, sent %u!\n",
            res->resarray_count, expected_count);
        return FALSE;
    }

    for (i = 0; i < res->resarray_count; i++)
    {
        expected_op = res->resarray[i].op;
        if (!xdr_u_int32_t(xdr, &res->resarray[i].op))
            return FALSE;

        /* validate each operation number against what we sent */
        if (res->resarray[i].op != expected_op) {
            eprintf("reply with %s in operation %u, expected %s!\n",
                nfs_opnum_to_string(res->resarray[i].op), i+1,
                nfs_opnum_to_string(expected_op));
            return FALSE;
        }

        entry = op_table_find(res->resarray[i].op);
        if (entry == NULL || entry->decode == NULL)
            return FALSE;
        if (!entry->decode(xdr, &res->resarray[i]))
            return FALSE;
    }
    return TRUE;
}
