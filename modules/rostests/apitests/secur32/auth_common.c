#include "client_server.h"

int AUTH_TEST_DATA_SIZE = 2;
AUTH_TEST_DATA authtestdata[] =
{
    // Standard-Test
    {
        L"NTLM",
        { // cli
            L"test", L"ROSauth!", L"ANDY-PC",
            ISC_REQ_CONFIDENTIALITY,
            0xe20882b7, /* Negotiate NegotiateFlags */
            0x10010,    /* ISC ContextRETAttr1 */
            0xe2888235, /* Authent. NegotiateFlags */
            0x1001c,    /* ISC ContextRETAttr2 */
        },
        {   // svr
            0,
            0xe28a8235, /* challenge NegotiateFlags */
            0x1c,       /* ASC ContextRETAttr1 (-> nego) */
            0x2001c,    /* ASC ContextRETAttr2 (-> auth) */
            FALSE /* has avl timestamp */
        }
    },
    // Anonymouse-Test
    {
        L"NTLM",
        { // cli
            NULL, NULL, NULL,
            ISC_REQ_CONFIDENTIALITY |
            ISC_REQ_NULL_SESSION,
            0xe20882b7, /* Negotiate NegotiateFlags */
            0x50010,    /* ISC ContextRETAttr1 */
            0xe2888a35, /* Authent. NegotiateFlags */
            0x5001c,    /* ISC ContextRETAttr2 */
        },
        {   // svr
            0,
            0xe28a8235, /* challenge NegotiateFlags */
            0x1c,       /* ASC ContextRETAttr1 (-> nego) */
            0x12001c,   /* ASC ContextRETAttr2 (-> auth) */
            FALSE /* has avl timestamp */
        }
    }
};

