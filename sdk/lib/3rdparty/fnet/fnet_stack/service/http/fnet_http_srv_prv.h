/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community.
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
*  Private. FNET HTTP Server API.
*
***************************************************************************/

#ifndef _FNET_HTTP_SRV_PRV_H_

#define _FNET_HTTP_SRV_PRV_H_

#include "fnet.h"

#if FNET_CFG_HTTP_SRV && FNET_CFG_FS

#if FNET_CFG_HTTP_SRV_SSI
    #include "fnet_http_srv_ssi_prv.h"
#endif

#if FNET_CFG_HTTP_SRV_POST && FNET_CFG_HTTP_SRV_VERSION_MAJOR
    #include "fnet_http_srv_post.h"
#endif

/* Minimum size of the internal buffer */
#if FNET_CFG_HTTP_SRV_VERSION_MAJOR /* HTTP/1.x*/
    #define FNET_HTTP_SRV_BUF_SIZE_MIN  (20u)
#else /* HTTP/0.9*/
    #define FNET_HTTP_SRV_BUF_SIZE_MIN  (10u)
#endif

/* Minimum buffer size protection.*/
#if FNET_CFG_HTTP_SRV_REQUEST_SIZE_MAX > FNET_HTTP_SRV_BUF_SIZE_MIN
    #define FNET_HTTP_SRV_BUF_SIZE  FNET_CFG_HTTP_SRV_REQUEST_SIZE_MAX
#else
    #define FNET_HTTP_SRV_BUF_SIZE  FNET_HTTP_SRV_BUF_SIZE_MIN
#endif

#if FNET_CFG_DEBUG_HTTP
    #define FNET_DEBUG_HTTP   FNET_DEBUG
#else
    #define FNET_DEBUG_HTTP(...)    do{}while(0)
#endif

#if FNET_CFG_HTTP_SRV_VERSION_MAJOR /* HTTP/1.x*/

/************************************************************************
*    HTTP 1.0 Reason-Phrase definitions.
*    RFC1945: The Reason-Phrase is intended to give a short textual
*             description of the Status-Code.
*************************************************************************/
#define FNET_HTTP_SRV_REASON_PHRASE_OK                      "OK"
#define FNET_HTTP_SRV_REASON_PHRASE_CREATED                 "Created"
#define FNET_HTTP_SRV_REASON_PHRASE_ACCEPTED                "Accepted"
#define FNET_HTTP_SRV_REASON_PHRASE_NO_CONTENT              "No Content"
#define FNET_HTTP_SRV_REASON_PHRASE_MOVED_PERMANENTLY       "Moved Permanently"
#define FNET_HTTP_SRV_REASON_PHRASE_MOVED_TEMPORARILY       "Moved Temporarily"
#define FNET_HTTP_SRV_REASON_PHRASE_NOT_MODIFIED            "Not Modified"
#define FNET_HTTP_SRV_REASON_PHRASE_BAD_REQUEST             "Bad Request"
#define FNET_HTTP_SRV_REASON_PHRASE_UNAUTHORIZED            "Unauthorized"
#define FNET_HTTP_SRV_REASON_PHRASE_FORBIDDEN               "Forbidden"
#define FNET_HTTP_SRV_REASON_PHRASE_NOT_FOUND               "Not Found"
#define FNET_HTTP_SRV_REASON_PHRASE_INTERNAL_SERVER_ERROR   "Internal Server Error"
#define FNET_HTTP_SRV_REASON_PHRASE_NOT_IMPLEMENTED         "Not Implemented"
#define FNET_HTTP_SRV_REASON_PHRASE_BAD_GATEWAY             "Bad Gateway"
#define FNET_HTTP_SRV_REASON_PHRASE_SERVICE_UNAVAILABLE     "Service Unavailable"

/************************************************************************
*    HTTP response status structure.
*************************************************************************/
struct fnet_http_srv_status
{
    fnet_http_srv_status_code_t   code;     /* Status-Code.*/
    fnet_char_t              *phrase;  /* An optional Reason-Phrase. May be set to NULL */
};

/************************************************************************
*    HTTP version structure.
*************************************************************************/
struct fnet_http_srv_version
{
    fnet_uint8_t major;
    fnet_uint8_t minor;
};

#endif /* FNET_CFG_HTTP_SRV_VERSION_MAJOR */

/******************************************************************************
 * HTTP server states.
 ******************************************************************************/
typedef enum
{
    FNET_HTTP_SRV_STATE_DISABLED = 0,       /**< @brief HTTP server service is
                                         * not initialized.*/
    FNET_HTTP_SRV_STATE_LISTENING = 1,      /**< @brief HTTP server is listening
                                         * for client socket.*/
    FNET_HTTP_SRV_STATE_RX_REQUEST = 2,     /**< @brief HTTP server is waiting or receiving
                                         * a HTTP request.*/
#if FNET_CFG_HTTP_SRV_POST && FNET_CFG_HTTP_SRV_VERSION_MAJOR
    FNET_HTTP_SRV_STATE_RX = 3,             /**< @brief HTTP server is receiving the
                                         * Entity-Body of a HTTP request.*/
#endif
    FNET_HTTP_SRV_STATE_TX = 4,             /**< @brief HTTP server is sending a
                                         * response to a client.*/
    FNET_HTTP_SRV_STATE_CLOSING = 5         /**< @brief HTTP server is closing
                                         * the socket connection.*/
} fnet_http_srv_state_t;

struct fnet_http_srv_if; /* Forward declaration.*/

/************************************************************************
*    HTTP response parameters structure.
*************************************************************************/
struct fnet_http_srv_response
{
    const struct fnet_http_srv_file_handler *send_file_handler;
    fnet_return_t                           (*tx_data)(struct fnet_http_srv_if *http);  /* TX state handler.*/
    fnet_bool_t                             send_eof;                   /* Optional EOF flag. It means nomore data for send*/
    fnet_size_t                             buffer_sent;                /* A number of bytes were sent.*/
    fnet_index_t                            status_line_state;
    fnet_uint32_t                           cookie;
#if FNET_CFG_HTTP_SRV_VERSION_MAJOR /* HTTP/1.x*/
    fnet_bool_t                             no_header;                  /* Flag do not send HTTP respnse status line and header*/
    const struct fnet_http_srv_content_type *send_file_content_type;    /* MIME Content-Type.*/
    struct fnet_http_srv_status             status;                     /* Status of the response.*/
    struct fnet_http_srv_version            version;                    /* Protocol version used for current request.*/
    fnet_int32_t                            content_length;             /* The total size of the data to send (is -1 if unknown).*/
#endif
#if FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC && FNET_CFG_HTTP_SRV_VERSION_MAJOR
    const fnet_http_srv_auth_t              *auth_entry;
    const struct fnet_http_srv_auth_scheme_if  *auth_scheme;
#endif
};

/************************************************************************
*    Parsed URI (Uniform Resource Identifier) structure.
*************************************************************************/
struct fnet_http_srv_uri
{
    fnet_char_t         *path;          /* File path (with file extension). */
    const fnet_char_t   *extension;     /* File extension. */
    fnet_char_t         *query;         /* Optional query string. */
};

/************************************************************************
*    HTTP request parameters structure.
*************************************************************************/
struct fnet_http_srv_request
{
    const struct fnet_http_srv_method   *method;
    struct fnet_http_srv_uri            uri;
    fnet_int32_t                    content_length;
#if FNET_CFG_HTTP_SRV_VERSION_MAJOR /* HTTP/1.x*/
    fnet_bool_t                     skip_line;
#endif
};

/************************************************************************
*    HTTP session control structure.
*************************************************************************/
struct fnet_http_srv_session_if
{
    fnet_http_srv_state_t           state;                                  /* Current state.*/
    fnet_time_t                     state_time_ms;                          /* Start time used by the state machine for timeout calculation.*/
    fnet_socket_t                   socket_foreign;                         /* Foreign socket.*/
    fnet_uint8_t                    buffer[FNET_HTTP_SRV_BUF_SIZE + 1u];    /* Receive/Transmit buffer */
    fnet_size_t                     buffer_actual_size;                     /* Size of the actual data in the buffer.*/
    fnet_fs_file_t                  file_desc;                              /* File to send */
    const void                      *data_ptr;                              /* Data to send */
    struct fnet_http_srv_response   response;                               /* Holds the accumulated data for the HTTP 1.0 response header. */
    struct fnet_http_srv_request    request;
#if (FNET_CFG_HTTP_SRV_TLS && FNET_CFG_TLS)
    fnet_tls_socket_t               tls_sock;                               /* TLS socket descriptor. */
#endif
};

/************************************************************************
*    HTTP interface control structure.
*************************************************************************/
struct fnet_http_srv_if
{
    fnet_socket_t                           socket_listen;              /* Listening socket.*/
    fnet_service_desc_t                     service_descriptor;         /* Descriptor of polling service.*/
    fnet_bool_t                             is_enabled;
    fnet_fs_dir_t                           root_dir;
    fnet_fs_file_t                          index_file;
    const struct fnet_http_srv_file_handler     *index_file_handler;
    struct fnet_http_srv_session_if             *session_active;
    struct fnet_http_srv_session_if             session[FNET_CFG_HTTP_SRV_SESSION_MAX];
#if FNET_CFG_HTTP_SRV_VERSION_MAJOR
    const struct fnet_http_srv_content_type     *index_file_content_type; /* MIME Content-Type of Index File.*/
#endif
#if FNET_CFG_HTTP_SRV_SSI
    struct fnet_http_srv_ssi_if                 ssi;
#endif
#if FNET_CFG_HTTP_SRV_CGI
    const fnet_http_srv_cgi_t                   *cgi_table;
#endif
#if FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC && FNET_CFG_HTTP_SRV_VERSION_MAJOR
    const fnet_http_srv_auth_t                  *auth_table;
#endif
#if FNET_CFG_HTTP_SRV_POST && FNET_CFG_HTTP_SRV_VERSION_MAJOR
    const fnet_http_srv_post_t                  *post_table;
#endif
#if (FNET_CFG_HTTP_SRV_TLS && FNET_CFG_TLS)
    fnet_tls_desc_t                             tls_desc;                   /* TLS descriptor. */
#endif
};

/************************************************************************
*    HTTP request handler structure.
*************************************************************************/
struct fnet_http_srv_method
{
    fnet_char_t     *token;	            /* Method token, which will identify protocol.
	                                    * It indicates the method to be performed on the resource identified
	                                    * by the Request-URI.*/
    fnet_return_t   (* handle)(struct fnet_http_srv_if *http, struct fnet_http_srv_uri *uri);
    fnet_return_t   (* receive)(struct fnet_http_srv_if *http);
    fnet_return_t   (* send)(struct fnet_http_srv_if *http);
    void            (* close)(struct fnet_http_srv_if *http);
};

/************************************************************************
*    File handler structure.
*************************************************************************/
struct fnet_http_srv_file_handler
{
    fnet_char_t     *file_extension;	      /* File extension */
    fnet_return_t   (*file_handle)(struct fnet_http_srv_if *http, struct fnet_http_srv_uri *uri);
    fnet_size_t     (*file_send)(struct fnet_http_srv_if *http);
    void            (*file_close)(struct fnet_http_srv_if *http);
};

/************************************************************************
*    File content type structure.
*************************************************************************/
struct fnet_http_srv_content_type
{
    const fnet_char_t *file_extension;	      /* File extension string*/
    const fnet_char_t *content_type;	      /* Content type string */
};

#if defined(__cplusplus)
extern "C" {
#endif

extern const struct fnet_http_srv_file_handler fnet_http_srv_cgi_handler;
extern const struct fnet_http_srv_method fnet_http_srv_method_get;
extern const struct fnet_http_srv_method fnet_http_srv_method_head;
#if FNET_CFG_HTTP_SRV_POST && FNET_CFG_HTTP_SRV_VERSION_MAJOR
extern const struct fnet_http_srv_method fnet_http_srv_method_post;
#endif
fnet_return_t _fnet_http_srv_default_handle (struct fnet_http_srv_if *http, struct fnet_http_srv_uri *uri);
fnet_size_t _fnet_http_srv_default_send (struct fnet_http_srv_if *http);
void _fnet_http_srv_default_close (struct fnet_http_srv_if *http);
fnet_char_t *_fnet_http_srv_uri_parse(fnet_char_t *in_str, struct fnet_http_srv_uri *uri);
const struct fnet_http_srv_file_handler *_fnet_http_srv_find_handler(struct fnet_http_srv_if *http, struct fnet_http_srv_uri *uri);
const struct fnet_http_srv_content_type *_fnet_http_srv_find_content_type(struct fnet_http_srv_if *http, struct fnet_http_srv_uri *uri);

#if defined(__cplusplus)
}
#endif

#endif /* FNET_CFG_HTTP_SRV && FNET_CFG_FS */

#endif /* _FNET_HTTP_SRV_PRV_H_ */
