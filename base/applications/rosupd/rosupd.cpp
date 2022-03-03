/*
 * PROJECT:     ReactOS Update
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     To update or fix system files easily
 * COPYRIGHT:   Copyright 2022 Vicky Silviana         (vickyisnotaboy@gmail.com)
 */
#include<stdio.h>
#include<stdlib.h>
#include<fstream>
#include<iostream>
#include<windows.h>
#include<string.h>
#include<string>
#include<vector>
#include<map>
typedef HRESULT (WINAPI *UDProc) (LPUNKNOWN,LPCTSTR,LPCTSTR,DWORD,LPBINDSTATUSCALLBACK);
using namespace std;
//Remember to change the path!
string sysdrv,sysdir;
bool fmatch(string fname,char fmd5sum[16])
{
	ifstream fin;
	fin.open((sysdir+"/"+fname).c_str(),ios_base::in);
	fin.close();
	return 1;
}
int main(int argc, char* argv[])
{
	int choicesum,filecnt;
	ifstream fin;
	string sname,vname,fpath;
	map<string,int>mfile;
	map<int,string>rmfile;
	vector<char[16]>fmd5sum;
	vector<bool>fpass;
	HINSTANCE hurlmon;
	UDProc murldownload;
	sysdrv=getenv("SystemDrive");
	sysdir=getenv("SystemRoot");
	printf("ReactOS Update Version 0.1\n");
	printf("Before updating your system, please make sure that ");
	printf("you have enough free space on your system partition and a stable connection.\n");
	printf("Are you sure you want to continue?");
	while (choicesum=system("choice")-1)
	{
		if (choicesum==1)return 0;
	}
	printf("Stage 1: Reading source file...");
	
	fin.open((sysdir+"\\source.cfg").c_str(),ios_base::in);
	fin>>sname>>vname;
	fin.close();
	if (sname.length()<=1)
	{
		printf("Failed!\n");
		printf("Can\'t find source file!");
		return 1;
	}
	printf("Done!\n");
	printf("Stage 2: Downloading index file...");
	system(("if exist "+sysdir+"\\index.cfg "
			+"del /f /q "+sysdir+"\\index.cfg > nul").c_str());
	hurlmon=LoadLibrary("urlmon.dll");
	murldownload=(UDProc)GetProcAddress(hurlmon,"URLDownloadToFileA");
	if (S_OK!=(murldownload)(NULL,
							 (sname+vname+"/index.cfg").c_str(),
							 (sysdir+"\\index.cfg").c_str(),
							 0,
							 NULL))
	{
		printf("Failed!\n");
		printf("No internet or wrong source!");
		FreeLibrary(hurlmon);
		return 2;
	}
	printf("Done!\n");
	printf("Stage 3: Comparing files...");
	fin.open((sysdir+"\\index.cfg").c_str(),ios_base::in);
	for (filecnt=0;;filecnt++)
	{
		fin>>rmfile[filecnt];
		if (rmfile[filecnt].length()<=1)break;
		mfile[rmfile[filecnt]]=filecnt;
		for (int j=0;j<16;j++)
		{
			fin>>fmd5sum[filecnt][j];
		}
	}
	fin.close();
	system(("del /f /q "+sysdir+"\\index.cfg > nul").c_str());
	for(int i=0;i<filecnt;i++)
	{
		if(fmatch(rmfile[i].substr(8),fmd5sum[i]))
			fpass[i]=1;
		else
			fpass[i]=0;
	}
	printf("Done!\n");
	printf("Stage 4: Downloading different files...\n");
	for(int i=0;i<filecnt;i++)
	{
		if(!fpass[i])
		{
			printf("%s",rmfile[i].c_str());
			if (S_OK!=(murldownload)(NULL,
									 (sname+vname+"/"+rmfile[i]).c_str(),
									 (sysdrv+"\\update\\"+rmfile[i]).c_str(),
									 0,
									 NULL))
						   				{
						   					printf("Succeed!\n");
						   					fpass[i]=1;
						   				}
			else printf("Failed!\n");
		}
	}
	printf("Done!\n");
//	printf("Stage 5: Changing freeldr.ini...");
	printf("Choose \"Update ReactOS\" during next boot to install update.");
	return 0;
}
