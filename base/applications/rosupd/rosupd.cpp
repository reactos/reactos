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
//#include"md5.h"
#define ROSUPDVERSION "0.1"
typedef HRESULT (WINAPI *UDProc) (LPUNKNOWN,LPCTSTR,LPCTSTR,DWORD,LPBINDSTATUSCALLBACK);
//Remember to change the path!
std::string SystemDrive,SystemRoot;
bool DoFilesMatch(std::string FileNameFunc,char FileMD5Func[16])
{
    std::ifstream FileStream;
    FileStream.open((SystemRoot+"/"+FileNameFunc).c_str(),std::ios_base::binary);
//  MD5_CTX md5file;
    FileStream.close();
    return 1;
}
int main(int argc, char* argv[])
{
    short ChoiceSum;
    int FileCount;
    std::ifstream FileStream;
    std::string SourceSite,VersionName;
    std::map <int,std::string> FileNameMap;
    std::vector <char[16]> FileMD5;
    std::vector <bool> IsDone;
    HINSTANCE HandleUrlmon;
    UDProc UrlDownloadAddress;
    SystemDrive = getenv("SystemDrive");
    SystemRoot = getenv("SystemRoot");
    printf("ReactOS Update Version %s\n", ROSUPDVERSION);
    printf("Before updating your system, please make sure that ");
    printf("you have enough free space on your system partition and a stable connection.\n");
    printf("Are you sure you want to continue?");
    while (ChoiceSum = system("choice") - 1)
    {
        if (ChoiceSum == 1)return 0;
    }
    printf("Stage 1: Reading source file...");
    FileStream.open((SystemRoot+"\\source.cfg").c_str(),std::ios_base::in);
    FileStream >> SourceSite >> VersionName;
    FileStream.close();
    if (SourceSite.length() <= 1)
    {
        printf("Failed!\n");
        printf("Can\'t FileStreamd source file!\n");
        return 1;
    }
    printf("Done!\n");
    printf("Stage 2: Downloading index file...");
    system(("if exist " + SystemRoot + "\\index.cfg "
            + "del /f /q " + SystemRoot + "\\index.cfg > nul").c_str());
    HandleUrlmon=LoadLibrary("urlmon.dll");
    UrlDownloadAddress=(UDProc)GetProcAddress(HandleUrlmon,"URLDownloadToFileA");
    if (S_OK!=(UrlDownloadAddress)(NULL,
                             (SourceSite+VersionName+"/index.cfg").c_str(),
                             (SystemRoot+"\\index.cfg").c_str(),
                             0,
                             NULL))
    {
        printf("Failed!\n");
        printf("No internet or wrong source!\n");
        FreeLibrary(HandleUrlmon);
        return 2;
    }
    printf("Done!\n");
    printf("Stage 3: Comparing files...");
    FileStream.open((SystemRoot+"\\index.cfg").c_str(),std::ios_base::in);
    for (FileCount=0;;FileCount++)
    {
        FileStream>>FileNameMap[FileCount];
        if (FileNameMap[FileCount].length()<=1)break;
        for (unsigned short j=0;j<16;j++)
        {
            FileStream>>FileMD5[FileCount][j];
        }
    }
    FileStream.close();
    system(("del /f /q "+SystemRoot+"\\index.cfg > nul").c_str());
    for (int i=0;i<FileCount;i++)
    {
        if (DoFilesMatch(FileNameMap[i].substr(8),FileMD5[i]))
            IsDone[i]=1;
        else
            IsDone[i]=0;
    }
    printf("Done!\n");
    printf("Stage 4: Downloading different files...");
    for (int i=0;i<FileCount;i++)
    {
        if (!IsDone[i])
        {
            printf("%s",FileNameMap[i].c_str());
            if (S_OK!=(UrlDownloadAddress)(NULL,
                                     (SourceSite+VersionName+"/"+FileNameMap[i]).c_str(),
                                     (SystemDrive+"\\update\\"+FileNameMap[i]).c_str(),
                                     0,
                                     NULL))
                                     {
                                         printf("Succeed!\n");
                                         IsDone[i]=1;
                                     }
            else printf("Failed!\n");
        }
    }
    printf("Done!\n");
    printf("Choose \"Update ReactOS\" during next boot to install update.\n");
    return 0;
}
