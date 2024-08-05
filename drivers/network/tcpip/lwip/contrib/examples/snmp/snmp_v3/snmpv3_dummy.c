/**
 * @file
 * Dummy SNMPv3 functions.
 */

/*
 * Copyright (c) 2016 Elias Oenal.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Elias Oenal <lwip@eliasoenal.com>
 *         Dirk Ziegelmeier <dirk@ziegelmeier.net>
 */

#include "lwip/apps/snmpv3.h"
#include "snmpv3_dummy.h"
#include <string.h>
#include "lwip/err.h"
#include "lwip/def.h"
#include "lwip/timeouts.h"

#if LWIP_SNMP && LWIP_SNMP_V3

struct user_table_entry {
  char               username[32];
  snmpv3_auth_algo_t auth_algo;
  u8_t               auth_key[20];
  snmpv3_priv_algo_t priv_algo;
  u8_t               priv_key[20];
};

static struct user_table_entry user_table[] = {
  { "lwip", SNMP_V3_AUTH_ALGO_INVAL, "" , SNMP_V3_PRIV_ALGO_INVAL, "" },
  { "piwl", SNMP_V3_AUTH_ALGO_INVAL, "" , SNMP_V3_PRIV_ALGO_INVAL, "" },
  { "test", SNMP_V3_AUTH_ALGO_INVAL, "" , SNMP_V3_PRIV_ALGO_INVAL, "" }
};

static char snmpv3_engineid[32];
static u8_t snmpv3_engineid_len;

static u32_t enginetime = 0;

/* In this implementation engineboots is volatile. In a real world application this value should be stored in non-volatile memory.*/
static u32_t engineboots = 0;

/**
 * @brief   Get the user table entry for the given username.
 *
 * @param[in] username  pointer to the username
 *
 * @return              pointer to the user table entry or NULL if not found.
 */
static struct user_table_entry*
get_user(const char *username)
{
  size_t i;

  for (i = 0; i < LWIP_ARRAYSIZE(user_table); i++) {
    if (strnlen(username, 32) != strnlen(user_table[i].username, 32)) {
      continue;
    }

    if (memcmp(username, user_table[i].username, strnlen(username, 32)) == 0) {
      return &user_table[i];
    }
  }

  return NULL;
}

u8_t
snmpv3_get_amount_of_users(void)
{
  return LWIP_ARRAYSIZE(user_table);
}

/**
 * @brief Get the username of a user number (index)
 * @param username is a pointer to a string.
 * @param index is the user index.
 * @return ERR_OK if user is found, ERR_VAL is user is not found.
 */
err_t
snmpv3_get_username(char *username, u8_t index)
{
  if (index < LWIP_ARRAYSIZE(user_table)) {
    MEMCPY(username, user_table[index].username, sizeof(user_table[0].username));
    return ERR_OK;
  }

  return ERR_VAL;
}

/**
 * Timer callback function that increments enginetime and reschedules itself.
 *
 * @param arg unused argument
 */
static void
snmpv3_enginetime_timer(void *arg)
{
  LWIP_UNUSED_ARG(arg);
  
  enginetime++;

  /* This handles the engine time reset */
  snmpv3_get_engine_time_internal();

  /* restart timer */
  sys_timeout(1000, snmpv3_enginetime_timer, NULL);
}

err_t
snmpv3_set_user_auth_algo(const char *username, snmpv3_auth_algo_t algo)
{
  struct user_table_entry *p = get_user(username);

  if (p) {
    switch (algo) {
    case SNMP_V3_AUTH_ALGO_INVAL:
      if (p->priv_algo != SNMP_V3_PRIV_ALGO_INVAL) {
        /* Privacy MUST be disabled before configuring authentication */
        break;
      } else {
        p->auth_algo = algo;
        return ERR_OK;
      }
#if LWIP_SNMP_V3_CRYPTO
    case SNMP_V3_AUTH_ALGO_MD5:
    case SNMP_V3_AUTH_ALGO_SHA:
#endif
      p->auth_algo = algo;
      return ERR_OK;
    default:
      break;
    }
  }

  return ERR_VAL;
}

err_t
snmpv3_set_user_priv_algo(const char *username, snmpv3_priv_algo_t algo)
{
  struct user_table_entry *p = get_user(username);

  if (p) {
    switch (algo) {
#if LWIP_SNMP_V3_CRYPTO
    case SNMP_V3_PRIV_ALGO_AES:
    case SNMP_V3_PRIV_ALGO_DES:
      if (p->auth_algo == SNMP_V3_AUTH_ALGO_INVAL) {
        /* Authentication MUST be enabled before configuring privacy */
        break;
      } else {
        p->priv_algo = algo;
        return ERR_OK;
      }
#endif
    case SNMP_V3_PRIV_ALGO_INVAL:
      p->priv_algo = algo;
      return ERR_OK;
    default:
      break;
    }
  }

  return ERR_VAL;
}

err_t
snmpv3_set_user_auth_key(const char *username, const char *password)
{
  struct user_table_entry *p = get_user(username);
  const char *engineid;
  u8_t engineid_len;

  if (p) {
    /* password should be at least 8 characters long */
    if (strlen(password) >= 8) {
      memset(p->auth_key, 0, sizeof(p->auth_key));
      snmpv3_get_engine_id(&engineid, &engineid_len);
      switch (p->auth_algo) {
      case SNMP_V3_AUTH_ALGO_INVAL:
        return ERR_OK;
#if LWIP_SNMP_V3_CRYPTO
      case SNMP_V3_AUTH_ALGO_MD5:
        snmpv3_password_to_key_md5((const u8_t*)password, strlen(password), (const u8_t*)engineid, engineid_len, p->auth_key);
        return ERR_OK;
      case SNMP_V3_AUTH_ALGO_SHA:
        snmpv3_password_to_key_sha((const u8_t*)password, strlen(password), (const u8_t*)engineid, engineid_len, p->auth_key);
        return ERR_OK;
#endif
      default:
        return ERR_VAL;
      }
    }
  }

  return ERR_VAL;
}

err_t
snmpv3_set_user_priv_key(const char *username, const char *password)
{
  struct user_table_entry *p = get_user(username);
  const char *engineid;
  u8_t engineid_len;

  if (p) {
    /* password should be at least 8 characters long */
    if (strlen(password) >= 8) {
      memset(p->priv_key, 0, sizeof(p->priv_key));
      snmpv3_get_engine_id(&engineid, &engineid_len);
      switch (p->auth_algo) {
      case SNMP_V3_AUTH_ALGO_INVAL:
        return ERR_OK;
#if LWIP_SNMP_V3_CRYPTO
      case SNMP_V3_AUTH_ALGO_MD5:
        snmpv3_password_to_key_md5((const u8_t*)password, strlen(password), (const u8_t*)engineid, engineid_len, p->priv_key);
        return ERR_OK;
      case SNMP_V3_AUTH_ALGO_SHA:
        snmpv3_password_to_key_sha((const u8_t*)password, strlen(password), (const u8_t*)engineid, engineid_len, p->priv_key);
        return ERR_OK;
#endif
      default:
        return ERR_VAL;
      }
    }
  }

  return ERR_VAL;
}

/**
 * @brief   Get the storage type of the given username.
 *
 * @param[in] username  pointer to the username
 * @param[out] type     the storage type
 *
 * @return              ERR_OK if the user was found, ERR_VAL if not.
 */
err_t
snmpv3_get_user_storagetype(const char *username, snmpv3_user_storagetype_t *type)
{
  if (get_user(username) != NULL) {
    /* Found user in user table
     * In this dummy implementation, storage is permanent because no user can be deleted.
     * All changes to users are lost after a reboot.*/
    *type = SNMP_V3_USER_STORAGETYPE_PERMANENT;
    return ERR_OK;
  }

  return ERR_VAL;
}

/**
 *  @param username is a pointer to a string.
 * @param auth_algo is a pointer to u8_t. The implementation has to set this if user was found.
 * @param auth_key is a pointer to a pointer to a string. Implementation has to set this if user was found.
 * @param priv_algo is a pointer to u8_t. The implementation has to set this if user was found.
 * @param priv_key is a pointer to a pointer to a string. Implementation has to set this if user was found.
 */
err_t
snmpv3_get_user(const char* username, snmpv3_auth_algo_t *auth_algo, u8_t *auth_key, snmpv3_priv_algo_t *priv_algo, u8_t *priv_key)
{
  const struct user_table_entry *p;
  
  /* The msgUserName specifies the user (principal) on whose behalf the
     message is being exchanged. Note that a zero-length userName will
     not match any user, but it can be used for snmpEngineID discovery. */
  if(strlen(username) == 0) {
    return ERR_OK;
  }
  
  p = get_user(username);

  if (!p) {
    return ERR_VAL;
  }
  
  if (auth_algo != NULL) {
    *auth_algo = p->auth_algo;
  }
  if(auth_key != NULL) {
    MEMCPY(auth_key, p->auth_key, sizeof(p->auth_key));
  }
  if (priv_algo != NULL) {
    *priv_algo = p->priv_algo;
  }
  if(priv_key != NULL) {
    MEMCPY(priv_key, p->priv_key, sizeof(p->priv_key));
  }
  return ERR_OK;
}

/**
 * Get engine ID from persistence
 */
void
snmpv3_get_engine_id(const char **id, u8_t *len)
{
  *id = snmpv3_engineid;
  *len = snmpv3_engineid_len;
}

/**
 * Store engine ID in persistence
 */
err_t
snmpv3_set_engine_id(const char *id, u8_t len)
{
  MEMCPY(snmpv3_engineid, id, len);
  snmpv3_engineid_len = len;
  return ERR_OK;
}

/**
 * Get engine boots from persistence. Must be increased on each boot.
 */
u32_t
snmpv3_get_engine_boots(void)
{
  return engineboots;
}

/**
 * Store engine boots in persistence
 */
void 
snmpv3_set_engine_boots(u32_t boots)
{
  engineboots = boots;
}

/**
 * RFC3414 2.2.2.
 * Once the timer reaches 2147483647 it gets reset to zero and the
 * engine boot ups get incremented.
 */
u32_t
snmpv3_get_engine_time(void)
{
  return enginetime;
}

/**
 * Reset current engine time to 0
 */
void
snmpv3_reset_engine_time(void)
{
  enginetime = 0;
}

/**
 * Initialize dummy SNMPv3 implementation
 */
void
snmpv3_dummy_init(void)
{
  snmpv3_set_engine_id("FOO", 3);

  snmpv3_set_user_auth_algo("lwip", SNMP_V3_AUTH_ALGO_SHA);
  snmpv3_set_user_auth_key("lwip", "maplesyrup");

  snmpv3_set_user_priv_algo("lwip", SNMP_V3_PRIV_ALGO_DES);
  snmpv3_set_user_priv_key("lwip", "maplesyrup");

  /* Start the engine time timer */
  snmpv3_enginetime_timer(NULL);
}

#endif /* LWIP_SNMP && LWIP_SNMP_V3 */
