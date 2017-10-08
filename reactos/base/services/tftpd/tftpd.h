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
#define MYBYTE unsigned char
#define MYWORD unsigned short
#define MYDWORD unsigned int

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
    MYDWORD server;
    MYWORD port;
    bool loaded;
    bool ready;
};

struct acknowledgement
{
    MYWORD opcode;
    MYWORD block;
};

struct message
{
    MYWORD opcode;
    char buffer[514];
};

struct tftperror
{
    MYWORD opcode;
    MYWORD errorcode;
    char errormessage[512];
};

struct packet
{
    MYWORD opcode;
    MYWORD block;
    char buffer;
};

struct data12
{
    MYDWORD rangeStart;
    MYDWORD rangeEnd;
};

struct request
{
    timeval tv;
    fd_set readfds;
    time_t expiry;
    SOCKET sock;
    SOCKET knock;
    MYBYTE sockInd;
    MYBYTE attempt;
    char path[256];
    FILE *file;
    char *filename;
    char *mode;
    char *alias;
    MYDWORD tsize;
    MYDWORD fblock;
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
    MYWORD blksize;
    MYWORD timeout;
    MYWORD block;
    MYWORD tblock;
};

struct data1
{
    tftpConnType tftpConn[MAX_SERVERS];
    MYDWORD allServers[MAX_SERVERS];
    MYDWORD staticServers[MAX_SERVERS];
    MYDWORD listenServers[MAX_SERVERS];
    MYWORD listenPorts[MAX_SERVERS];
    SOCKET maxFD;
    bool ready;
    bool busy;
};

struct data2
{
    WSADATA wsaData;
    home homes[8];
    FILE *logfile;
    data12 hostRanges[32];
    char fileRead;
    char fileWrite;
    char fileOverwrite;
    int minport;
    int maxport;
    MYDWORD failureCount;
    MYBYTE logLevel;
    bool ifspecified;
};

struct data15
{
    union
    {
        //MYDWORD ip;
        unsigned ip:32;
        MYBYTE octate[4];
    };
};

//Functions
bool detectChange();
void closeConn();
void getInterfaces(data1*);
void runProg();
void processRequest(LPVOID lpParam);
char* myGetToken(char*, MYBYTE);
char* myTrim(char*, char*);
void init(void*);
bool cleanReq(request*);
bool addServer(MYDWORD*, MYDWORD);
FILE* openSection(const char*, MYBYTE, char*);
char* readSection(char*, FILE*);
bool getSection(const char*, char*, MYBYTE, char*);
bool isIP(char*s);
char* myLower(char*);
char* myUpper(char*);
char* IP2String(char*, MYDWORD);
MYDWORD* findServer(MYDWORD*, MYDWORD);
void printWindowsError();
void logMess(request*, MYBYTE);
void logMess(char*, MYBYTE);
