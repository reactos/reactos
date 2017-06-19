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
#include <stdio.h>
#include <time.h>

#include "upcall.h"
#include "daemon_debug.h"
#include "util.h"

extern const nfs41_upcall_op nfs41_op_mount;
extern const nfs41_upcall_op nfs41_op_unmount;
extern const nfs41_upcall_op nfs41_op_open;
extern const nfs41_upcall_op nfs41_op_close;
extern const nfs41_upcall_op nfs41_op_read;
extern const nfs41_upcall_op nfs41_op_write;
extern const nfs41_upcall_op nfs41_op_lock;
extern const nfs41_upcall_op nfs41_op_unlock;
extern const nfs41_upcall_op nfs41_op_readdir;
extern const nfs41_upcall_op nfs41_op_getattr;
extern const nfs41_upcall_op nfs41_op_setattr;
extern const nfs41_upcall_op nfs41_op_getexattr;
extern const nfs41_upcall_op nfs41_op_setexattr;
extern const nfs41_upcall_op nfs41_op_symlink;
extern const nfs41_upcall_op nfs41_op_volume;
extern const nfs41_upcall_op nfs41_op_getacl;
extern const nfs41_upcall_op nfs41_op_setacl;

static const nfs41_upcall_op *g_upcall_op_table[] = {
    &nfs41_op_mount,
    &nfs41_op_unmount,
    &nfs41_op_open,
    &nfs41_op_close,
    &nfs41_op_read,
    &nfs41_op_write,
    &nfs41_op_lock,
    &nfs41_op_unlock,
    &nfs41_op_readdir,
    &nfs41_op_getattr,
    &nfs41_op_setattr,
    &nfs41_op_getexattr,
    &nfs41_op_setexattr,
    &nfs41_op_symlink,
    &nfs41_op_volume,
    &nfs41_op_getacl,
    &nfs41_op_setacl,
    NULL,
    NULL
};
#ifdef __REACTOS__
static const uint32_t g_upcall_op_table_size = (sizeof(g_upcall_op_table) / sizeof(g_upcall_op_table[0]));
#else
static const uint32_t g_upcall_op_table_size = ARRAYSIZE(g_upcall_op_table);
#endif

int upcall_parse(
    IN unsigned char *buffer,
    IN uint32_t length,
    OUT nfs41_upcall *upcall)
{
    int status;
    const nfs41_upcall_op *op;
    DWORD version;

    ZeroMemory(upcall, sizeof(nfs41_upcall));
    if (!length) {
        eprintf("empty upcall\n");
        upcall->status = status = 102;
        goto out;
    }

    dprintf(2, "received %d bytes upcall data: processing upcall\n", length);
    print_hexbuf(4, (unsigned char *)"upcall buffer: ", buffer, length);

    /* parse common elements */
    status = safe_read(&buffer, &length, &version, sizeof(uint32_t));
    if (status) goto out;
    status = safe_read(&buffer, &length, &upcall->xid, sizeof(uint64_t));
    if (status) goto out;
    status = safe_read(&buffer, &length, &upcall->opcode, sizeof(uint32_t));
    if (status) goto out;
    status = safe_read(&buffer, &length, &upcall->root_ref, sizeof(HANDLE));
    if (status) goto out;
    status = safe_read(&buffer, &length, &upcall->state_ref, sizeof(HANDLE));
    if (status) goto out;

    dprintf(2, "time=%ld version=%d xid=%d opcode=%s session=0x%x open_state=0x%x\n", 
        time(NULL), version, upcall->xid, opcode2string(upcall->opcode), upcall->root_ref, 
        upcall->state_ref);
    if (version != NFS41D_VERSION) {
        eprintf("received version %d expecting version %d\n", version, NFS41D_VERSION);
        upcall->status = status = NFSD_VERSION_MISMATCH;
        goto out;
    }
    if (upcall->opcode >= g_upcall_op_table_size) {
        status = ERROR_NOT_SUPPORTED;
        eprintf("unrecognized upcall opcode %d!\n", upcall->opcode);
        goto out;
    }
    if (upcall->root_ref != INVALID_HANDLE_VALUE)
        nfs41_root_ref(upcall->root_ref);
    if (upcall->state_ref != INVALID_HANDLE_VALUE)
        nfs41_open_state_ref(upcall->state_ref);

    /* parse the operation's arguments */
    op = g_upcall_op_table[upcall->opcode];
    if (op && op->parse) {
        status = op->parse(buffer, length, upcall);
        if (status) {
            eprintf("parsing of upcall '%s' failed with %d.\n",
                opcode2string(upcall->opcode), status);
            goto out;
        }
    }
out:
    return status;
}

int upcall_handle(
    IN nfs41_upcall *upcall)
{
    int status = NO_ERROR;
    const nfs41_upcall_op *op;

    op = g_upcall_op_table[upcall->opcode];
    if (op == NULL || op->handle == NULL) {
        status = ERROR_NOT_SUPPORTED;
        eprintf("upcall '%s' missing handle function!\n",
            opcode2string(upcall->opcode));
        goto out;
    }

    upcall->status = op->handle(upcall);
out:
    return status;
}
#pragma warning (disable : 4706) /* assignment within conditional expression */
void upcall_marshall(
    IN nfs41_upcall *upcall,
    OUT unsigned char *buffer,
    IN uint32_t length,
    OUT uint32_t *length_out)
{
    const nfs41_upcall_op *op;
    unsigned char *orig_buf = buffer;
    const uint32_t total = length, orig_len = length;

    /* marshall common elements */
write_downcall:
    length = orig_len;
    buffer = orig_buf;
    safe_write(&buffer, &length, &upcall->xid, sizeof(upcall->xid));
    safe_write(&buffer, &length, &upcall->opcode, sizeof(upcall->opcode));
    safe_write(&buffer, &length, &upcall->status, sizeof(upcall->status));
    safe_write(&buffer, &length, &upcall->last_error, sizeof(upcall->last_error));

    if (upcall->status)
        goto out;

    /* marshall the operation's results */
    op = g_upcall_op_table[upcall->opcode];
    if (op && op->marshall) {
        if ((upcall->status = op->marshall(buffer, &length, upcall)))
            goto write_downcall;
    }
out:
    *length_out = total - length;
}

void upcall_cancel(
    IN nfs41_upcall *upcall)
{
    const nfs41_upcall_op *op = g_upcall_op_table[upcall->opcode];
    if (op && op->cancel)
        op->cancel(upcall);
}

void upcall_cleanup(
    IN nfs41_upcall *upcall)
{
    const nfs41_upcall_op *op = g_upcall_op_table[upcall->opcode];
    if (op && op->cleanup && upcall->status != NFSD_VERSION_MISMATCH)
        op->cleanup(upcall);

    if (upcall->state_ref && upcall->state_ref != INVALID_HANDLE_VALUE) {
        nfs41_open_state_deref(upcall->state_ref);
        upcall->state_ref = NULL;
    }
    if (upcall->root_ref && upcall->root_ref != INVALID_HANDLE_VALUE) {
        nfs41_root_deref(upcall->root_ref);
        upcall->root_ref = NULL;
    }
}
