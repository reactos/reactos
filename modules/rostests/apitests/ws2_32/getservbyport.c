/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for getservbyport
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "ws2_32.h"

START_TEST(getservbyport)
{
    WSADATA WsaData;
    struct servent *Serv;
    const struct
    {
        int Port;
        struct
        {
            PCSTR Proto;
            PCSTR Name;
            PCSTR Aliases[5+1];
        } Protos[3+1];
    } Tests[] =
    {
        { 0,                                                       },
        { -1,                                                      },
        { 80,         { { "tcp", "http", { "www", "www-http" } },
                        { "udp", NULL                          },
                        { "xyz", NULL                          } } },
        { 65536 + 80, { { "tcp", "http", { "www", "www-http" } } } },
        { 0xffff0050, { { "tcp", "http", { "www", "www-http" } } } },
        { 25,         { { "tcp", "smtp", { "mail" }            } } },
        { 445,        { { "tcp", "microsoft-ds"                },
                        { "udp", "microsoft-ds"                } } },
        { 514,        { { "tcp", "cmd", { "shell" }            },
                        { "udp", "syslog"                      } } },
        { 47624,      { { "tcp", "directplaysrvr"              },
                        { "udp", "directplaysrvr"              } } },
    };
    ULONG i, Proto, Alias;
    int Error;
    ULONG ExpectProto;

    /* not yet initialized */
    Serv = getservbyport(0, NULL);
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
            Serv = getservbyport(htons(Tests[i].Port), Tests[i].Protos[Proto].Proto);
            Error = WSAGetLastError();

            /* For a NULL proto we expect the same as the first array entry */
            ExpectProto = Proto;
            if (Tests[i].Protos[Proto].Proto == NULL)
            {
                ExpectProto = 0;
            }

            if (Tests[i].Protos[ExpectProto].Name == NULL)
            {
                ok(Serv == NULL, "[%d, %s] getservbyport succeeded unexpectedly\n",
                   Tests[i].Port, Tests[i].Protos[Proto].Proto);
                ok(Error == WSANO_DATA, "[%d, %s] getservbyport returned error %d\n",
                   Tests[i].Port, Tests[i].Protos[Proto].Proto, Error);
                continue;
            }
            else
            {
                ok(Serv != NULL, "[%d, %s] getservbyport failed with %d\n",
                   Tests[i].Port, Tests[i].Protos[Proto].Proto, Error);
            }

            if (Serv == NULL)
            {
                continue;
            }

            /* Check name */
            ok(!strcmp(Serv->s_name, Tests[i].Protos[ExpectProto].Name),
               "[%d, %s] s_name = '%s', expected '%s'\n",
               Tests[i].Port, Tests[i].Protos[Proto].Proto, Serv->s_name, Tests[i].Protos[ExpectProto].Name);

            /* Check aliases */
            ok(Serv->s_aliases != NULL, "[%d, %s] s_aliases = NULL\n",
               Tests[i].Port, Tests[i].Protos[Proto].Proto);
            for (Alias = 0; Serv->s_aliases; Alias++)
            {
                if (Alias >= RTL_NUMBER_OF(Tests[i].Protos[ExpectProto].Aliases))
                {
                    ok(0, "[%d, %s] Too many aliases\n",
                       Tests[i].Port, Tests[i].Protos[Proto].Proto);
                    break;
                }
                if (Serv->s_aliases[Alias] == NULL)
                {
                    ok(Tests[i].Protos[ExpectProto].Aliases[Alias] == NULL,
                       "[%d, %s] getservbyport did not return expected alias '%s'\n",
                       Tests[i].Port, Tests[i].Protos[Proto].Proto, Tests[i].Protos[ExpectProto].Aliases[Alias]);
                    break;
                }
                if (Tests[i].Protos[ExpectProto].Aliases[Alias] == NULL)
                {
                    ok(Serv->s_aliases[Alias] == NULL,
                       "[%d, %s] getservbyport returned additional alias '%s'\n",
                       Tests[i].Port, Tests[i].Protos[Proto].Proto, Serv->s_aliases[Alias]);
                    break;
                }

                 ok(!strcmp(Serv->s_aliases[Alias], Tests[i].Protos[ExpectProto].Aliases[Alias]),
                    "[%d, %s] Got alias '%s', expected '%s'\n",
                    Tests[i].Port, Tests[i].Protos[Proto].Proto, Serv->s_aliases[Alias],Tests[i].Protos[ExpectProto].Aliases[Alias]);
            }

            /* Port should be equal (upper bits are ignored) */
            ok(ntohs(Serv->s_port) == (Tests[i].Port & 0xffff), "[%d, %s] s_port = %d\n",
               Tests[i].Port, Tests[i].Protos[Proto].Proto, ntohs(Serv->s_port));

            /* Check proto */
            ok(!strcmp(Serv->s_proto, Tests[i].Protos[ExpectProto].Proto), "[%d, %s] s_proto = '%s', expected '%s'\n",
               Tests[i].Port, Tests[i].Protos[Proto].Proto, Serv->s_proto, Tests[i].Protos[ExpectProto].Proto);
        /* We want to include one NULL past the last proto in the array */
        } while (Tests[i].Protos[Proto++].Proto != NULL);
    }

    Error = WSACleanup();
    ok_dec(Error, 0);
}
