#include <windows.h>
#include <stdio.h>
#include <lmaccess.h>
#include <winnls.h>

void main(int argc, char *argv[])
{
	NET_API_STATUS err;

	LPWSTR domainname, username, oldpassword, newpassword;

	domainname = malloc(50*sizeof(WCHAR));
	username = malloc(50*sizeof(WCHAR));
	oldpassword = malloc(50*sizeof(WCHAR));
	newpassword = malloc(50*sizeof(WCHAR));

	if (argc != 5) {
		printf("Usage:\n\n%s Domain Username Oldpassword Newpassword\n",argv[0]);
		exit(1);
	}

	MultiByteToWideChar(CP_ACP,0,argv[1],-1,domainname,50);
	MultiByteToWideChar(CP_ACP,0,argv[2],-1,username,50);
	MultiByteToWideChar(CP_ACP,0,argv[3],-1,oldpassword,50);
	MultiByteToWideChar(CP_ACP,0,argv[4],-1,newpassword,50);

	err = NetUserChangePassword(domainname, username, oldpassword, newpassword);
	
	/*
	NET_API_STATUS NetUserChangePassword(
		LPWSTR domainname,	// pointer to server or domain name string
		LPWSTR username,	// pointer to user name string
		LPWSTR oldpassword,	// pointer to old password string
		LPWSTR newpassword	// pointer to new password string
	   );
	*/

	printf("Return value:%d\n",err);
	exit( err );

	free(domainname);
	free(username);
	free(oldpassword);
	free(newpassword);
}
