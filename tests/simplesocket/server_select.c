#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define PORT 		54321
#define DIRSIZE 	8192

int main()
{
	char     	dir[DIRSIZE] = {0};
	int 	 	sd, sd_current, ret, rdlen=0;
	BOOLEAN		done = FALSE;
	int 	 	addrlen;
	struct   	sockaddr_in sin;
	struct   	sockaddr_in pin;
	
	struct timeval tv;
	struct fd_set readable, writable, exception;
	INT nbio = 1;
	
	WORD version = MAKEWORD(1,1);
	WSADATA wsaData;

	WSAStartup(version, &wsaData);
 
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "socket error\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);

	if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
	{
		fprintf(stderr, "bind error\n");
		exit(1);
	}
	
	if (listen(sd, 5) == -1)
	{
		fprintf(stderr, "listen error\n");
		exit(1);
	}
	
	addrlen = sizeof(pin); 
	if ((sd_current = accept(sd, (struct sockaddr *)  &pin, &addrlen)) == -1)
	{
		fprintf(stderr, "accept error\n");
		exit(1);
	}
	
	if (ioctlsocket(sd_current, FIONBIO, (ULONG *)&nbio))
	{
		fprintf(stderr, "ioctlsocket: %d\n", WSAGetLastError());
		exit(1);
    }
	
	fprintf(stdout, "Client connected...waiting for message\n");
	
	do
	{
		FD_ZERO(&readable);
		FD_ZERO(&writable);
		FD_ZERO(&exception);

		FD_SET(sd_current, &readable);
		FD_SET(sd_current,&writable);
		FD_SET(sd_current, &exception);

		tv.tv_sec = 1; tv.tv_usec = 0;

		ret = select(sd_current + 1, &readable, &writable, &exception, &tv );
		
		if (ret>0)
		{
			if (FD_ISSET(sd_current, &writable))
			{
				/*if (*towrite_ptr)
				{
					wrlen = send(sd_current, dir, strlen(dir), 0);
					if (wrlen > 0)
						towrite_ptr += wrlen;
					else
					{
						done = TRUE;
					}
					fprintf( stderr, "send: %d bytes\n", wrlen );
				}
				else
				{
					fprintf( stderr, "send: finished header and waiting\n" );
				}*/
			}
			if (FD_ISSET(sd_current, &readable))
			{
				fprintf(stdout, "Waiting for recv\n");
				rdlen = recv(sd_current, dir, sizeof(dir), 0);
				if (rdlen > 0)
				{
					fprintf(stdout, "Message from client: %s\n", dir);
					send(sd_current, dir, rdlen, 0);
				}
				else
				{
					fprintf(stderr, "recv error\n" );
				}
				
				done = TRUE;
			}
			if (FD_ISSET(sd_current, &exception))
			{
				fprintf(stderr, "exception\n");
				done = TRUE;
			}
		}
		else if (!ret)
		{ 
			fprintf(stderr, "timeout\r" );
			fflush(stderr);
		}
		else
		{
			fprintf(stderr, "return from select: %x\n", ret);
		}
    } while(!done);
	
	close(sd);
	close(sd_current);
	
	return 0;
}