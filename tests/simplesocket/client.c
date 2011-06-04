#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define PORT        54321
#define HOST        "localhost"
#define DIRSIZE     8192

int main(int argc, char **argv)
{
	char hostname[100];
	char dir[DIRSIZE];
	int	sd;
	struct sockaddr_in sin;
	struct sockaddr_in pin;
	struct hostent *hp;
	
	WORD version = MAKEWORD(1,1);
	WSADATA wsaData;

	WSAStartup(version, &wsaData);

	strcpy(hostname, HOST);
	if (argc>2)
	{
		strcpy(hostname, argv[2]);
	}

	if ((hp = gethostbyname(hostname)) == 0)
	{
		printf("gethostbyname error\n");
		exit(1);
	}

	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	pin.sin_port = htons(PORT);

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("socket error\n");
		exit(1);
	}

	if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1)
	{
		printf("connect error\n");
		exit(1);
	}

	if (send(sd, argv[1], strlen(argv[1]), 0) == -1)
	{
		printf("send error\n");
		exit(1);
	}

	if (recv(sd, dir, DIRSIZE, 0) == -1)
	{
		printf("recv error\n");
		exit(1);
	}

	printf("Message from server: %s\n", dir);

	close(sd);
	
	return 0;
}