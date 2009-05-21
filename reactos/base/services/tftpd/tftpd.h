/**************************************************************************
*   Copyright (C) 2005 by Achal Dhir                                      *
*   achaldhir@gmail.com                                                   *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
// TFTPServer.cpp

#ifdef _MSC_VER
   #define strcasecmp _stricmp
   #define _CRT_SECURE_NO_WARNINGS
   #pragma comment(lib, "ws2_32.lib")
   #pragma comment(lib, "iphlpapi.lib")
#endif

//Constants
#define my_inet_addr     inet_addr
#define MAX_SERVERS 8

//Structs
struct home
{
    char alias[64];
    char target[256];
};

struct tftpConnType
{
    SOCKET sock;
    sockaddr_in addr;
    DWORD server;
    WORD port;
};

struct acknowledgement
{
    WORD opcode;
    WORD block;
};

struct message
{
    WORD opcode;
    char buffer[514];
};

struct tftperror
{
    WORD opcode;
    WORD errorcode;
    char errormessage[512];
};

struct packet
{
    WORD opcode;
    WORD block;
    char buffer;
};

struct data12
{
    DWORD rangeStart;
    DWORD rangeEnd;
};

struct request
{
    timeval tv;
    fd_set readfds;
    time_t expiry;
    SOCKET sock;
    SOCKET knock;
    BYTE sockInd;
    BYTE attempt;
    char path[256];
    FILE *file;
    char *filename;
    char *mode;
    char *alias;
    DWORD tsize;
    DWORD fblock;
    int bytesReady;
    int bytesRecd;
    int bytesRead[2];
    packet* pkt[2];
    sockaddr_in client;
    socklen_t clientsize;
    union
    {
        tftperror serverError;
        message mesout;
        acknowledgement acout;
    };
    union
    {
        tftperror clientError;
        message mesin;
        acknowledgement acin;
    };
    WORD blksize;
    WORD timeout;
    WORD block;
    WORD tblock;
};

struct data2
{
    WSADATA wsaData;
    tftpConnType tftpConn[MAX_SERVERS];
    DWORD servers[MAX_SERVERS];
    WORD ports[MAX_SERVERS];
    home homes[8];
    FILE *logfile;
    data12 hostRanges[32];
    char fileRead;
    char fileWrite;
    char fileOverwrite;
    int minport;
    int maxport;
    SOCKET maxFD;
    BYTE logLevel;
};

struct data15
{
    union
    {
        //DWORD ip;
        unsigned ip:32;
        BYTE octate[4];
    };
};

//Functions
void runProg();
void processRequest(LPVOID lpParam);
char* myGetToken(char*, BYTE);
void init();
bool cleanReq(request*);
bool getSection(char*, char*, BYTE, char*);
bool isIP(char*s);
char* myLower(char*);
char* myUpper(char*);
char* IP2String(char*, DWORD);
void printWindowsError();
void logMess(request*, BYTE);
void logMess(char*, BYTE);
