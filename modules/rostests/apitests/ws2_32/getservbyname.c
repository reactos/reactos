/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for getservbyname
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "ws2_32.h"

START_TEST(getservbyname)
{
    WSADATA WsaData;
    struct servent *Serv;
    const struct
    {
        PCSTR Name;
        struct
        {
            PCSTR Proto;
            int Port;
            PCSTR Name;
            PCSTR Aliases[5+1];
        } Protos[3+1];
    } Tests[] =
    {
        { ""                                                              },
        { "xyzzy"                                                         },
        { "http",           { { "tcp", 80,  "http", { "www", "www-http" } },
                              { "udp", 0                                  },
                              { "xyz", 0                                  } } },
        { "smtp",           { { "tcp", 25,  "smtp", { "mail" }            } } },
        { "mail",           { { "tcp", 25,  "smtp", { "mail" }            } } },
        { "microsoft-ds",   { { "tcp", 445, "microsoft-ds",               },
                              { "udp", 445, "microsoft-ds",               } } },
        { "cmd",            { { "tcp", 514, "cmd", { "shell" }            },
                              { "udp", 0                                  } } },
        { "syslog",         { { "udp", 514, "syslog"                      },
                              { "tcp", 0                                  } } },
        { "directplaysrvr", { { "tcp", 47624, "directplaysrvr"            },
                              { "udp", 47624, "directplaysrvr"            } } },
    };
    ULONG i, Proto, Alias;
    int Error;
    ULONG ExpectProto;

    /* not yet initialized */
    Serv = getservbyname(NULL, NULL);
    Error = WSAGetLastError();
    ok(Serv == NULL, "Serv = %p\n", Serv);
    ok(Error == WSANOTINITIALISED, "Error = %d\n", Error);

    Error = WSAStartup(MAKEWORD(2, 2), &WsaData);
    ok_dec(Error, 0);

    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        Proto = 0;
        do
        {
            Serv = getservbyname(Tests[i].Name, Tests[i].Protos[Proto].Proto);
            Error = WSAGetLastError();

            /* For a NULL proto we expect the same as the first array entry */
            ExpectProto = Proto;
            if (Tests[i].Protos[Proto].Proto == NULL)
            {
                ExpectProto = 0;
            }

            if (Tests[i].Protos[ExpectProto].Port == 0)
            {
                ok(Serv == NULL, "[%s, %s] getservbyname succeeded unexpectedly\n",
                   Tests[i].Name, Tests[i].Protos[Proto].Proto);
                ok(Error == WSANO_DATA, "[%s, %s] getservbyname returned error %d\n",
                   Tests[i].Name, Tests[i].Protos[Proto].Proto, Error);
                continue;
            }
            else
            {
                ok(Serv != NULL, "[%s, %s] getservbyname failed with %d\n",
                   Tests[i].Name, Tests[i].Protos[Proto].Proto, Error);
            }

            if (Serv == NULL)
            {
                continue;
            }

            /* Check name */
            ok(!strcmp(Serv->s_name, Tests[i].Protos[ExpectProto].Name),
               "[%s, %s] s_name = '%s', expected '%s'\n",
               Tests[i].Name, Tests[i].Protos[Proto].Proto, Serv->s_name, Tests[i].Protos[ExpectProto].Name);

            /* Check aliases */
            ok(Serv->s_aliases != NULL, "[%s, %s] s_aliases = NULL\n",
               Tests[i].Name, Tests[i].Protos[Proto].Proto);
            for (Alias = 0; Serv->s_aliases; Alias++)
            {
                if (Alias >= RTL_NUMBER_OF(Tests[i].Protos[ExpectProto].Aliases))
                {
                    ok(0, "[%s, %s] Too many aliases\n",
                       Tests[i].Name, Tests[i].Protos[Proto].Proto);
                    break;
                }
                if (Serv->s_aliases[Alias] == NULL)
                {
                    ok(Tests[i].Protos[ExpectProto].Aliases[Alias] == NULL,
                       "[%s, %s] getservbyname did not return expected alias '%s'\n",
                       Tests[i].Name, Tests[i].Protos[Proto].Proto, Tests[i].Protos[ExpectProto].Aliases[Alias]);
                    break;
                }
                if (Tests[i].Protos[ExpectProto].Aliases[Alias] == NULL)
                {
                    ok(Serv->s_aliases[Alias] == NULL,
                       "[%s, %s] getservbyname returned additional alias '%s'\n",
                       Tests[i].Name, Tests[i].Protos[Proto].Proto, Serv->s_aliases[Alias]);
                    break;
                }

                 ok(!strcmp(Serv->s_aliases[Alias], Tests[i].Protos[ExpectProto].Aliases[Alias]),
                    "[%s, %s] Got alias '%s', expected '%s'\n",
                    Tests[i].Name, Tests[i].Protos[Proto].Proto, Serv->s_aliases[Alias],Tests[i].Protos[ExpectProto].Aliases[Alias]);
            }

            /* Check port */
            ok(ntohs(Serv->s_port) == Tests[i].Protos[ExpectProto].Port, "[%s, %s] s_port = %d\n",
               Tests[i].Name, Tests[i].Protos[Proto].Proto, ntohs(Serv->s_port));

            /* Check proto */
            ok(!strcmp(Serv->s_proto, Tests[i].Protos[ExpectProto].Proto), "[%s, %s] s_proto = '%s', expected '%s'\n",
               Tests[i].Name, Tests[i].Protos[Proto].Proto, Serv->s_proto, Tests[i].Protos[ExpectProto].Proto);
        /* We want to include one NULL past the last proto in the array */
        } while (Tests[i].Protos[Proto++].Proto != NULL);
    }

    Error = WSACleanup();
    ok_dec(Error, 0);
}
