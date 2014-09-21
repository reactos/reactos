
#pragma once

#define typeof(_X) __typeof_ ## _X

typedef gnutls_alert_description_t (__cdecl typeof(gnutls_alert_get))(gnutls_session_t session);
typedef const char* (__cdecl typeof(gnutls_alert_get_name))(gnutls_alert_description_t alert);
typedef void (__cdecl typeof(gnutls_certificate_free_credentials))(gnutls_certificate_credentials_t sc);
typedef int (__cdecl typeof(gnutls_certificate_allocate_credentials))(gnutls_certificate_credentials_t
                    * res);
typedef const gnutls_datum_t* (__cdecl typeof(gnutls_certificate_get_peers))(gnutls_session_t
                           session, unsigned int
                           *list_size);
typedef gnutls_cipher_algorithm_t (__cdecl typeof(gnutls_cipher_get))(gnutls_session_t session);
typedef size_t (__cdecl typeof(gnutls_cipher_get_key_size))(gnutls_cipher_algorithm_t algorithm);
typedef int (__cdecl typeof(gnutls_credentials_set))(gnutls_session_t session,
               gnutls_credentials_type_t type, void *cred);
typedef void (__cdecl typeof(gnutls_deinit))(gnutls_session_t session);
typedef int (__cdecl typeof(gnutls_global_init))(void);
typedef void (__cdecl typeof(gnutls_global_deinit))(void);
typedef void (__cdecl typeof(gnutls_global_set_log_function))(gnutls_log_func log_func);
typedef void (__cdecl typeof(gnutls_global_set_log_level))(int level);
typedef int (__cdecl typeof(gnutls_handshake))(gnutls_session_t session);
typedef int (__cdecl typeof(gnutls_init))(gnutls_session_t * session, unsigned int flags);
typedef gnutls_kx_algorithm_t (__cdecl typeof(gnutls_kx_get))(gnutls_session_t session);
typedef gnutls_mac_algorithm_t (__cdecl typeof(gnutls_mac_get))(gnutls_session_t session);
typedef size_t (__cdecl typeof(gnutls_mac_get_key_size))(gnutls_mac_algorithm_t algorithm);
typedef void (__cdecl typeof(gnutls_perror))(int error);
typedef gnutls_protocol_t (__cdecl typeof(gnutls_protocol_get_version))(gnutls_session_t session);
typedef int (__cdecl typeof(gnutls_priority_set_direct))(gnutls_session_t session,
                   const char *priorities,
                   const char **err_pos);
typedef size_t (__cdecl typeof(gnutls_record_get_max_size))(gnutls_session_t session);
typedef ssize_t (__cdecl typeof(gnutls_record_recv))(gnutls_session_t session, void *data,
               size_t data_size);
typedef ssize_t (__cdecl typeof(gnutls_record_send))(gnutls_session_t session, const void *data,
               size_t data_size);
typedef int (__cdecl typeof(gnutls_server_name_set))(gnutls_session_t session,
               gnutls_server_name_type_t type,
               const void *name, size_t name_length);
typedef gnutls_transport_ptr_t (__cdecl typeof(gnutls_transport_get_ptr))(gnutls_session_t session);
typedef void (__cdecl typeof(gnutls_transport_set_errno))(gnutls_session_t session, int err);
typedef void (__cdecl typeof(gnutls_transport_set_ptr))(gnutls_session_t session,
                  gnutls_transport_ptr_t ptr);
typedef void (__cdecl typeof(gnutls_transport_set_push_function))(gnutls_session_t session,
                    gnutls_push_func push_func);
typedef void (__cdecl typeof(gnutls_transport_set_pull_function))(gnutls_session_t session,
                    gnutls_pull_func pull_func);
