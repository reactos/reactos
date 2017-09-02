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

#ifndef RECOVERY_H
#define RECOVERY_H

#include "nfs41.h"


/* session/client recovery uses a lock and condition variable in nfs41_client
 * to prevent multiple threads from attempting to recover at the same time */
bool_t nfs41_recovery_start_or_wait(
    IN nfs41_client *client);

void nfs41_recovery_finish(
    IN nfs41_client *client);


int nfs41_recover_session(
    IN nfs41_session *session,
    IN bool_t client_state_lost);

void nfs41_recover_sequence_flags(
    IN nfs41_session *session,
    IN uint32_t flags);

int nfs41_recover_client_state(
    IN nfs41_session *session,
    IN nfs41_client *client);

void nfs41_client_state_revoked(
    IN nfs41_session *session,
    IN nfs41_client *client,
    IN uint32_t revoked);

struct __nfs_argop4;
bool_t nfs41_recover_stateid(
    IN nfs41_session *session,
    IN struct __nfs_argop4 *argop);

#endif /* RECOVERY_H */
