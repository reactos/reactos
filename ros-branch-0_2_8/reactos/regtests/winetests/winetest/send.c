/*
 * HTTP handling functions.
 *
 * Copyright 2003 Ferenc Wagner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <winsock.h>
#include <stdio.h>
#include <errno.h>

#include "winetest.h"

static SOCKET
open_http (const char *server)
{
    WSADATA wsad;
    struct sockaddr_in sa;
    SOCKET s;

    report (R_STATUS, "Opening HTTP connection to %s", server);
    if (WSAStartup (MAKEWORD (2,2), &wsad)) return INVALID_SOCKET;

    sa.sin_family = AF_INET;
    sa.sin_port = htons (80);
    sa.sin_addr.s_addr = inet_addr (server);
    if (sa.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *host = gethostbyname (server);
        if (!host) {
            report (R_ERROR, "Hostname lookup failed for %s", server);
            goto failure;
        }
        sa.sin_addr.s_addr = ((struct in_addr *)host->h_addr)->s_addr;
    }
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        report (R_ERROR, "Can't open network socket: %d",
                WSAGetLastError ());
        goto failure;
    }
    if (!connect (s, (struct sockaddr*)&sa, sizeof (struct sockaddr_in)))
        return s;

    report (R_ERROR, "Can't connect: %d", WSAGetLastError ());
    closesocket (s);
 failure:
    WSACleanup ();
    return INVALID_SOCKET;
}

static int
close_http (SOCKET s)
{
    int ret;

    ret = closesocket (s);
    return (WSACleanup () || ret);
}

static int
send_buf (SOCKET s, const char *buf, size_t length)
{
    int sent;

    while (length > 0) {
        sent = send (s, buf, length, 0);
        if (sent == SOCKET_ERROR) return 1;
        buf += sent;
        length -= sent;
    }
    return 0;
}

static int
send_str (SOCKET s, ...)
{
    va_list ap;
    char *p;
    int ret;
    size_t len;

    va_start (ap, s);
    p = vstrmake (&len, ap);
    va_end (ap);
    if (!p) return 1;
    ret = send_buf (s, p, len);
    free (p);
    return ret;
}

int
send_file (const char *name)
{
    SOCKET s;
    FILE *f;
#define BUFLEN 8192
    unsigned char buffer[BUFLEN+1];
    size_t bytes_read, total, filesize;
    char *str;
    int ret;

    /* RFC 2616 */
#define SEP "--8<--cut-here--8<--"
    static const char head[] = "POST /submit HTTP/1.0\r\n"
        "Host: test.winehq.org\r\n"
        "User-Agent: Winetest Shell\r\n"
        "Content-Type: multipart/form-data; boundary=\"" SEP "\"\r\n"
        "Content-Length: %u\r\n\r\n";
    static const char body1[] = "--" SEP "\r\n"
        "Content-Disposition: form-data; name=\"reportfile\"; filename=\"%s\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n";
    static const char body2[] = "\r\n--" SEP "\r\n"
        "Content-Disposition: form-data; name=\"submit\"\r\n\r\n"
        "Upload File\r\n"
        "--" SEP "--\r\n";

    s = open_http ("test.winehq.org");
    if (s == INVALID_SOCKET) return 1;

    f = fopen (name, "rb");
    if (!f) {
        report (R_WARNING, "Can't open file '%s': %d", name, errno);
        goto abort1;
    }
    fseek (f, 0, SEEK_END);
    filesize = ftell (f);
    if (filesize > 1024*1024) {
        report (R_WARNING,
                "File too big (%.1f MB > 1 MB); submitting partial report.",
                filesize/1024.0/1024);
        filesize = 1024*1024;
    }
    fseek (f, 0, SEEK_SET);

    report (R_STATUS, "Sending header");
    str = strmake (&total, body1, name);
    ret = send_str (s, head, filesize + total + sizeof body2 - 1) ||
        send_buf (s, str, total);
    free (str);
    if (ret) {
        report (R_WARNING, "Error sending header: %d, %d",
                errno, WSAGetLastError ());
        goto abort2;
    }

    report (R_STATUS, "Sending %u bytes of data", filesize);
    report (R_PROGRESS, 2, filesize);
    total = 0;
    while (total < filesize && (bytes_read = fread (buffer, 1, BUFLEN/2, f))) {
        if ((signed)bytes_read == -1) {
            report (R_WARNING, "Error reading log file: %d", errno);
            goto abort2;
        }
        total += bytes_read;
        if (total > filesize) bytes_read -= total - filesize;
        if (send_buf (s, buffer, bytes_read)) {
            report (R_WARNING, "Error sending body: %d, %d",
                    errno, WSAGetLastError ());
            goto abort2;
        }
        report (R_DELTA, bytes_read, "Network transfer: In progress");
    }
    fclose (f);

    if (send_buf (s, body2, sizeof body2 - 1)) {
        report (R_WARNING, "Error sending trailer: %d, %d",
                errno, WSAGetLastError ());
        goto abort2;
    }
    report (R_DELTA, 0, "Network transfer: Done");

    total = 0;
    while ((bytes_read = recv (s, buffer+total, BUFLEN-total, 0))) {
        if ((signed)bytes_read == SOCKET_ERROR) {
            report (R_WARNING, "Error receiving reply: %d, %d",
                    errno, WSAGetLastError ());
            goto abort1;
        }
        total += bytes_read;
        if (total == BUFLEN) {
            report (R_WARNING, "Buffer overflow");
            goto abort1;
        }
    }
    if (close_http (s)) {
        report (R_WARNING, "Error closing connection: %d, %d",
                errno, WSAGetLastError ());
        return 1;
    }

    str = strmake (&bytes_read, "Received %s (%d bytes).\n",
                   name, filesize);
    ret = memcmp (str, buffer + total - bytes_read, bytes_read);
    free (str);
    if (ret) {
        buffer[total] = 0;
        str = strstr (buffer, "\r\n\r\n");
        if (!str) str = buffer;
        else str = str + 4;
        report (R_ERROR, "Can't submit logfile '%s'. "
                "Server response: %s", name, str);
    }
    return ret;

 abort2:
    fclose (f);
 abort1:
    close_http (s);
    return 1;
}
