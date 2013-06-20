/*
 * Copyright 2011 Hans Leidekker for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_HTTPREQUESTID_H
#define __WINE_HTTPREQUESTID_H

#define DISPID_HTTPREQUEST_BASE                     1
#define DISPID_HTTPREQUEST_OPEN                     (DISPID_HTTPREQUEST_BASE)
#define DISPID_HTTPREQUEST_SETREQUESTHEADER         (DISPID_HTTPREQUEST_BASE + 1)
#define DISPID_HTTPREQUEST_GETRESPONSEHEADER        (DISPID_HTTPREQUEST_BASE + 2)
#define DISPID_HTTPREQUEST_GETALLRESPONSEHEADERS    (DISPID_HTTPREQUEST_BASE + 3)
#define DISPID_HTTPREQUEST_SEND                     (DISPID_HTTPREQUEST_BASE + 4)
#define DISPID_HTTPREQUEST_OPTION                   (DISPID_HTTPREQUEST_BASE + 5)
#define DISPID_HTTPREQUEST_STATUS                   (DISPID_HTTPREQUEST_BASE + 6)
#define DISPID_HTTPREQUEST_STATUSTEXT               (DISPID_HTTPREQUEST_BASE + 7)
#define DISPID_HTTPREQUEST_RESPONSETEXT             (DISPID_HTTPREQUEST_BASE + 8)
#define DISPID_HTTPREQUEST_RESPONSEBODY             (DISPID_HTTPREQUEST_BASE + 9)
#define DISPID_HTTPREQUEST_RESPONSESTREAM           (DISPID_HTTPREQUEST_BASE + 10)
#define DISPID_HTTPREQUEST_ABORT                    (DISPID_HTTPREQUEST_BASE + 11)
#define DISPID_HTTPREQUEST_SETPROXY                 (DISPID_HTTPREQUEST_BASE + 12)
#define DISPID_HTTPREQUEST_SETCREDENTIALS           (DISPID_HTTPREQUEST_BASE + 13)
#define DISPID_HTTPREQUEST_WAITFORRESPONSE          (DISPID_HTTPREQUEST_BASE + 14)
#define DISPID_HTTPREQUEST_SETTIMEOUTS              (DISPID_HTTPREQUEST_BASE + 15)
#define DISPID_HTTPREQUEST_SETCLIENTCERTIFICATE     (DISPID_HTTPREQUEST_BASE + 16)
#define DISPID_HTTPREQUEST_SETAUTOLOGONPOLICY       (DISPID_HTTPREQUEST_BASE + 17)

#endif /* __WINE_HTTPREQUESTID_H */
