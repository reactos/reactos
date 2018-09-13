/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    partit.c

Abstract:

    This module contains entry points to perform the
    'configure system partition' and 'create OS loader'
    options of the ARC installer.

Author:

    Ted Miller        (tedm)    Nov-1991

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


PCHAR
AlGetNextArcNamToken (
    IN PCHAR TokenString,
    OUT PCHAR OutputToken,
    OUT PULONG UnitNumber
    );

#define STATUS_ROW_TOP     13
#define STATUS_ROW_BOTTOM  17

char  NOMEMMSG[] = "Insufficient memory";
char  NOCNFMSG[] = "Unable to determine disk configuration (ARC status = %u)";
char  NOFMTMSG[] = "Format failed (ARC status = %u)";
char  DSKFLMSG[] = "Disk is full";
char  NOCREMSG[] = "Could not create partition (ARC status = %u)";
char  NODELMSG[] = "Could not delete partition (ARC status = %u)";
char  ALREAMSG[] = "Partition is already a system partition";
char  NOFILMSG[] = "Unable to determine filesystem on partition (ARC status = %u)";
char  NOENVMSG[] = "Error (ARC status = %u) determining environment";
char  NOEVAMSG[] = "Could not add partition to environment (ARC status = %u)";
char  NOEVDMSG[] = "Could not remove partition from environment (ARC status = %u)";
char  NOSYSMSG[] = "No system partitions defined";
char  NOPARMSG[] = "No partitions on this disk";

char  SYSPARTVAR[] = "SYSTEMPARTITION";
char  OSLOADERVAR[] = "OSLOADER";
char  OSLOADPARTVAR[] = "OSLOADPARTITION";

#define MsgNoMem()  AlStatusMsg(STATUS_ROW_TOP,STATUS_ROW_BOTTOM,TRUE,NOMEMMSG);

PCHAR SysPartMenu[] = {   "Create System Partition",
                          "Delete Partition",
                          "Make Existing Partition into System Partition",
                          "Exit"
                      };

#define SYSPARTMENU_CREATE 0
#define SYSPARTMENU_DELETE 1
#define SYSPARTMENU_ADD    2
#define SYSPARTMENU_EXIT   3

#define MENU_ROW 4

char  sprintfBuffer[256];

BOOLEAN
Confirm(
    PCHAR Warning
    )
{
    char  c;
    ULONG Count;

    AlClearStatusArea(STATUS_ROW_TOP,STATUS_ROW_BOTTOM);
    AlPrint("%s%s (y/n)?",MSGMARGIN,Warning);
    ArcRead(ARC_CONSOLE_INPUT,&c,1,&Count);
    while((c != 'y') && (c != 'Y') && (c != 'n') && (c != 'N') && (c != ASCI_ESC)) {
        ArcRead(ARC_CONSOLE_INPUT,&c,1,&Count);
    }
    AlClearStatusArea(STATUS_ROW_TOP,STATUS_ROW_BOTTOM);
    return((BOOLEAN)((c == 'y') || (c == 'Y')));
}


VOID
PrintErrorMsg(
    PCHAR   FormatString,
    ...
    )
{
    va_list ArgList;

    va_start(ArgList,FormatString);

    AlClearStatusArea(STATUS_ROW_TOP,STATUS_ROW_BOTTOM);
    vAlStatusMsg(STATUS_ROW_TOP,TRUE,FormatString,ArgList);

    AlWaitKey(NULL);
    AlClearStatusArea(STATUS_ROW_TOP,STATUS_ROW_BOTTOM);
}


VOID
PrintMsg(
    PCHAR   FormatString,
    ...
    )
{
    va_list ArgList;

    va_start(ArgList,FormatString);

    AlClearStatusArea(STATUS_ROW_TOP,STATUS_ROW_BOTTOM);
    vAlStatusMsg(STATUS_ROW_TOP,FALSE,FormatString,ArgList);

    AlWaitKey(NULL);
    AlClearStatusArea(STATUS_ROW_TOP,STATUS_ROW_BOTTOM);
}


BOOLEAN
IsBootSelectionPartition(
    PCHAR Variable,
    ULONG Disk,
    ULONG Partition,
    PULONG MatchNumber OPTIONAL
    )
{
    char  text[256];
    PCHAR Var = ArcGetEnvironmentVariable(Variable);

    if (Var == NULL) {
        return(FALSE);
    }

    sprintf(text,"%spartition(%u)",GetDiskName(Disk),Partition);
    return(AlFindNextMatchComponent(Var,text,0,MatchNumber));
}


LONG
ChooseDisk(
    VOID
    )
{
    ULONG DiskCount = GetDiskCount();
    PVOID MenuID;
    LONG Disk;
    ULONG ChosenDisk;
    PCHAR DiskName,TempDiskName;
    ULONG UnitNumber,ScsiNumber,ScsiId,DiskNumber;
    CHAR Token[32];

    if (DiskCount == 1) {
        return(0);
    }

    if (!AlNewMenu(&MenuID)) {
        MsgNoMem();
        return(-1);
    }

    for(Disk = DiskCount - 1; Disk >= 0; Disk--) {

        DiskName = GetDiskName(Disk);

        if ((strstr(DiskName, "scsi") != NULL) &&
            (strstr(DiskName, "disk") != NULL) ) {

            ScsiNumber = ScsiId = DiskNumber = 0;
            TempDiskName = DiskName;

            while (TempDiskName != NULL) {
                TempDiskName = AlGetNextArcNamToken(TempDiskName,
                                                    Token,
                                                    &UnitNumber);

                if (strcmp(Token,"scsi") == 0) {
                    ScsiNumber = UnitNumber;
                }

                if (strcmp(Token,"disk") == 0) {
                    ScsiId = UnitNumber;
                }

                if (strcmp(Token,"rdisk") == 0) {
                    DiskNumber = UnitNumber;
                }
            }

            sprintf(sprintfBuffer,
                    "Scsi bus %d, Identifier %d, Disk %d (%s)",
                    ScsiNumber,
                    ScsiId,
                    DiskNumber,
                    GetDiskName(Disk));
        } else {
            sprintf(sprintfBuffer,"Disk %d (%s)",Disk,GetDiskName(Disk));
        }


        if (!AlAddMenuItem(MenuID,sprintfBuffer,Disk,0)) {
            MsgNoMem();
            return(-1);
        }
    }

    if (!AlDisplayMenu(MenuID,FALSE,0,&ChosenDisk,MENU_ROW,"Select Disk")) {
        return(-1);
    } else {
        return(ChosenDisk);
    }
}


BOOLEAN         // true if partition was created
DoPartitionCreate(
    OUT PULONG DiskNo,
    OUT PULONG PartitionNo
    )
{
    ULONG              Disk,i;
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount,Choice;
    ULONG              ChosenSize;
    ARC_STATUS         status;
    BOOLEAN            xAny,xP,xE,xL,PrimaryExists;
    PVOID              MenuID;
    ULONG              PartitionNumber;
    char               PartitionPath[256];


    if ((Disk = ChooseDisk()) == -1) {
        return(FALSE);
    }

    if ((status = DoesAnyPrimaryExist(Disk,&PrimaryExists)) != ESUCCESS) {
        PrintErrorMsg(NOCNFMSG,status);
        return(FALSE);
    }

    if ((status = IsAnyCreationAllowed(Disk,
                                      TRUE,
                                      &xAny,
                                      &xP,
                                      &xE,
                                      &xL
                                     )
       )
      != ESUCCESS)
    {
        PrintErrorMsg(NOCNFMSG,status);
        return(FALSE);
    }

    // in order for a creation to be allowed there must be
    // - free space on the disk and a free mbr entry OR
    // - free space in an existing extended partition.

    if (!xAny) {
        PrintErrorMsg(DSKFLMSG);
        return(FALSE);
    }

    if ((status = GetFreeDiskRegions(Disk,&Regions,&RegionCount)) != ESUCCESS) {
        PrintErrorMsg(NOCNFMSG,status);
        return(FALSE);
    }

    if (!AlNewMenu(&MenuID)) {
        MsgNoMem();
        FreeRegionArray(Regions,RegionCount);
        return(FALSE);
    }

    // Present the user with a list of the free spaces
    // on the disk (and within the extended partition, if it
    // exists).

    if (RegionCount > 1) {

        for(i=0; i<RegionCount; i++) {

            sprintf(sprintfBuffer,"%u MB space",Regions[i].SizeMB);
            if (Regions[i].RegionType == REGION_LOGICAL) {
                strcat(sprintfBuffer," (in extended partition)");
            }
            if (!AlAddMenuItem(MenuID,sprintfBuffer,i,0)) {
                MsgNoMem();
                AlFreeMenu(MenuID);
                FreeRegionArray(Regions,RegionCount);
                return(FALSE);
            }
        }

        if (!AlDisplayMenu(MenuID,FALSE,0,&Choice,MENU_ROW,"Available Free Spaces")) {
            // user escaped
            AlClearStatusArea(STATUS_ROW_TOP,STATUS_ROW_BOTTOM);
            AlFreeMenu(MenuID);
            FreeRegionArray(Regions,RegionCount);
            return(FALSE);
        }
    } else {
        Choice = 0;
    }

    AlFreeMenu(MenuID);

    // now ask the user for the size of the partition to create in the chosen space.

    do {
        AlClearStatusArea(STATUS_ROW_TOP,STATUS_ROW_BOTTOM);
        AlPrint("%sEnter size in MB (1-%u): ",MSGMARGIN,Regions[Choice].SizeMB);
        if (!AlGetString(sprintfBuffer,sizeof(sprintfBuffer))) {
            FreeRegionArray(Regions,RegionCount);
            return(FALSE);
        }
        ChosenSize = atoi(sprintfBuffer);
        if (!ChosenSize || (ChosenSize > Regions[Choice].SizeMB)) {
            PrintErrorMsg("Invalid size.");
        } else {
            break;
        }
    } while(1);

    // The chosen space is either in the extended partition or not.
    // If it is, just create the requested partition.

    if (Regions[Choice].RegionType == REGION_LOGICAL) {

        status = CreatePartition(&Regions[Choice],ChosenSize,REGION_LOGICAL);
        PartitionNumber = Regions[Choice].PartitionNumber;

    } else {

        // The chosen space is not in an extended partition.
        // If there's already a primary and we can create
        // an extended partition, then first create an
        // extended partition spanning the entire space chosen
        // by the user, and then a logical volume within it of
        // size entered by the user.  Otherwise [ie, there's no primary
        // or we're not allowed to create an extended partition]
        // create a primary of the size entered by the user.


        if (PrimaryExists && xE) {

            // create extended partition first.
            status = CreatePartition(&Regions[Choice],Regions[Choice].SizeMB,REGION_EXTENDED);
            FreeRegionArray(Regions,RegionCount);

            if ((status = GetFreeLogicalDiskRegions(Disk,&Regions,&RegionCount)) != ESUCCESS) {
                PrintErrorMsg(NOCNFMSG,status);
                return(FALSE);
            }
            // since we just created the extended partition, there will be one free
            // region in it.

            status = CreatePartition(Regions,ChosenSize,REGION_LOGICAL);
            PartitionNumber = Regions[0].PartitionNumber;

        } else {

            status = CreatePartition(&Regions[Choice],ChosenSize,REGION_PRIMARY);
            PartitionNumber = Regions[Choice].PartitionNumber;
        }
    }
    FreeRegionArray(Regions,RegionCount);

    if ( (status                                 == ESUCCESS)
    && ((status = CommitPartitionChanges(Disk)) == ESUCCESS))
    {
        PrintMsg("Partition successfully created.");

#if 0
        //
        // This is bogus since this routine is called in the code path where
        // the user is creating a system partition.
        //
        if (ArcGetEnvironmentVariable(SYSPARTVAR) == NULL) {
            if (Confirm("Do you want to make this the system partition")) {
                sprintf(PartitionPath,"%spartition(%u)",GetDiskName(Disk),PartitionNumber);
                if ((status = ArcSetEnvironmentVariable(SYSPARTVAR,PartitionPath)) != ESUCCESS) {
                    PrintErrorMsg(NOEVAMSG,status);
                }
            }
        }
#endif

        *DiskNo      = Disk;
        *PartitionNo = PartitionNumber;
        return(TRUE);

    } else {
        PrintErrorMsg(NOCREMSG,status);
        return(FALSE);
    }
}


VOID
DoPartitionDelete(
    VOID
    )
{
    BOOLEAN            xAny,xPrimary,xExtended,xLogical;
    ARC_STATUS         status;
    PVOID              MenuID;
    ULONG              i,RegionCount,Choice;
    ULONG              MatchNumber,Index;
    LONG               Disk;
    PREGION_DESCRIPTOR Regions;
    BOOLEAN            err,Confirmation;


    if ((Disk = ChooseDisk()) == -1) {
        return;
    }

    if ((status = DoesAnyPartitionExist(Disk,&xAny,&xPrimary,&xExtended,&xLogical)) != ESUCCESS) {
        PrintErrorMsg(NOCNFMSG,status);
        return;
    }

    if (xAny) {

        if (!AlNewMenu(&MenuID)) {
            MsgNoMem();
            return;
        }

        if ((status = GetUsedDiskRegions(Disk,&Regions,&RegionCount)) != ESUCCESS) {

            PrintErrorMsg(NOCNFMSG,status);

        } else {

            err = FALSE;

            for(i=0; i<RegionCount; i++) {

                sprintf(sprintfBuffer,"%s%u MB ",Regions[i].RegionType == REGION_LOGICAL ? "    " : "",Regions[i].SizeMB);
                strcat(sprintfBuffer,GetSysIDName(Regions[i].SysID));
                strcat(sprintfBuffer,Regions[i].RegionType == REGION_LOGICAL ? " Logical Volume" : " Partition");
                if (IsExtended(Regions[i].SysID) && xLogical) {
                    strcat(sprintfBuffer,", also resulting in the deletion of:");
                }

                if (!AlAddMenuItem(MenuID,sprintfBuffer,i,0)) {
                    MsgNoMem();
                    err = TRUE;
                    break;
                }
            }

            if (!err) {

                if (AlDisplayMenu(MenuID,FALSE,0,&Choice,MENU_ROW,"Select Partition to Delete")) {

                    if (IsBootSelectionPartition(SYSPARTVAR,Disk,Regions[Choice].PartitionNumber,NULL)) {
                        Confirmation = Confirm("The selected partition is (or contains) a system partition.\r\n Are you sure you want to delete it");
                    } else {
                        if (IsBootSelectionPartition(OSLOADPARTVAR,Disk,Regions[Choice].PartitionNumber,NULL)) {
                            Confirmation = Confirm("The selected partition contains an operating system.\r\n Are you sure you want to delete it");
                        } else {
                            Confirmation = Confirm("Are you sure");
                        }
                    }
                    if (Confirmation) {
                        if (((status = DeletePartition(&Regions[Choice])) != ESUCCESS)
                        || ((status = CommitPartitionChanges(Disk)) != ESUCCESS))
                        {
                            PrintErrorMsg(NODELMSG,status);
                        } else {
                            PrintMsg("Partition deleted successfully.");

                            sprintf(sprintfBuffer,
                                    "%spartition(%u)",
                                    GetDiskName(Disk),
                                    Regions[Choice].PartitionNumber
                                   );

                            //
                            // If there are any boot selections and the deleted partition
                            // was used, ask if the associated boot selections should be
                            // deleted.
                            //

                            if (ArcGetEnvironmentVariable(OSLOADPARTVAR) != NULL) {
                                if ((IsBootSelectionPartition(SYSPARTVAR,Disk,Regions[Choice].PartitionNumber,NULL) ||
                                     IsBootSelectionPartition(OSLOADPARTVAR,Disk,Regions[Choice].PartitionNumber,NULL)) &&
                                     Confirm("Do you want to delete the boot selection(s) associated\r\n with the deleted partition?")) {

                                    while (IsBootSelectionPartition(SYSPARTVAR,Disk,Regions[Choice].PartitionNumber,&MatchNumber)) {
                                        for ( Index = 0 ; Index < MaximumBootVariable ; Index++ ) {
                                            JzDeleteVariableSegment(BootString[Index],MatchNumber);
                                        }
                                    }

                                    while (IsBootSelectionPartition(OSLOADPARTVAR,Disk,Regions[Choice].PartitionNumber,&MatchNumber)) {
                                        for ( Index = 0 ; Index < MaximumBootVariable ; Index++ ) {
                                            JzDeleteVariableSegment(BootString[Index],MatchNumber);
                                        }
                                    }
                                }

                            } else {

                                //
                                // There are no boot selections but delete all segments
                                // of the SystemPartition variable that pointed to the
                                // deleted partition.
                                //

                                while (IsBootSelectionPartition(SYSPARTVAR,Disk,Regions[Choice].PartitionNumber,&MatchNumber)) {
                                    JzDeleteVariableSegment(BootString[SystemPartitionVariable],MatchNumber);
                                }
                            }
                        }
                    }
                }
            }

            FreeRegionArray(Regions,RegionCount);
        }
        AlFreeMenu(MenuID);

    } else {
        PrintErrorMsg("No partitions on this disk");
    }
}


VOID
DoSystemPartitionCreate(
    VOID
    )
{
    ULONG      DiskNo,PartitionNo;
    ARC_STATUS status;

    if (DoPartitionCreate(&DiskNo,&PartitionNo)) {

        sprintf(sprintfBuffer,"%spartition(%u)",GetDiskName(DiskNo),PartitionNo);

        // add the partition to the SYSTEMPARTITION NVRAM environment variable

        if ((status = AlAddSystemPartition(sprintfBuffer)) != ESUCCESS) {
            PrintErrorMsg(NOEVAMSG,status);
            return;
        }

        AlStatusMsgNoWait(STATUS_ROW_TOP,STATUS_ROW_BOTTOM,FALSE,"Formatting %s",sprintfBuffer);
        status = FmtFatFormat(sprintfBuffer,GetHiddenSectorCount(DiskNo,PartitionNo));
        if (status == ESUCCESS) {
            SetSysID(DiskNo,PartitionNo,SYSID_BIGFAT);
            PrintMsg("Partition formatted successfully.");
        } else {
            PrintErrorMsg(NOFMTMSG,status);
        }
    }
}


VOID
DoMakePartitionSystemPartition(
    VOID
    )
{
    BOOLEAN            f,IsFAT;
    ULONG              Disk,i,Choice;
    PVOID              MenuID;
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount;
    ARC_STATUS         status;
    char               PartitionPath[256];
    ULONG              PartitionNumber;


    // take an existing partition and add it to the
    // list of system partitions.  Also make sure it's
    // formatted as FAT.

    if ((Disk = ChooseDisk()) == -1) {
        return;
    }

    status = GetUsedDiskRegions(Disk,&Regions,&RegionCount);
    if (status != ESUCCESS) {
        PrintErrorMsg(NOCNFMSG,status);
        return;
    }

    if (!RegionCount) {
        FreeRegionArray(Regions,RegionCount);
        PrintErrorMsg(NOPARMSG);
        return;
    }

    if (!AlNewMenu(&MenuID)) {
        FreeRegionArray(Regions,RegionCount);
        MsgNoMem();
        return;
    }

    for(i=0; i<RegionCount; i++) {
        if (!IsExtended(Regions[i].SysID)) {
            sprintf(sprintfBuffer,
                    "Partition %u (%u MB %s)",
                    Regions[i].PartitionNumber,
                    Regions[i].SizeMB,
                    GetSysIDName(Regions[i].SysID)
                   );
            if (!AlAddMenuItem(MenuID,sprintfBuffer,i,0)) {
                MsgNoMem();
                AlFreeMenu(MenuID);
                FreeRegionArray(Regions,RegionCount);
                return;
            }
        }
    }

    f = AlDisplayMenu(MenuID,FALSE,0,&Choice,MENU_ROW,"Choose Partition");
    AlFreeMenu(MenuID);
    if (!f) {            // user escaped
        FreeRegionArray(Regions,RegionCount);
        return;
    }

    PartitionNumber = Regions[Choice].PartitionNumber;
    sprintf(PartitionPath,"%spartition(%u)",GetDiskName(Disk),PartitionNumber);

    FreeRegionArray(Regions,RegionCount);

    if(IsBootSelectionPartition(SYSPARTVAR,Disk,PartitionNumber,NULL)) {
        PrintErrorMsg(ALREAMSG);
        return;
    }

    status = FmtIsFat(PartitionPath,&IsFAT);
    if (status != ESUCCESS) {
        PrintErrorMsg(NOFILMSG,status);
        return;
    }

    if (!IsFAT) {
        if (Confirm("System partitions must be formatted with the FAT filesystem.\r\n Do you wish to format the chosen partition")
            && Confirm("All existing data will be lost.  Are you sure"))
        {
            status = FmtFatFormat(PartitionPath,GetHiddenSectorCount(Disk,PartitionNumber));
            if (status != ESUCCESS) {
                PrintErrorMsg(NOFMTMSG,status);
                return;
            }
            SetSysID(Disk,PartitionNumber,SYSID_BIGFAT);
        } else {
            return;
        }
    }

    //
    // Add to list of system partitions.
    //
    if ((status = AlAddSystemPartition(PartitionPath)) == ESUCCESS) {
        PrintMsg("Partition added successfully.");
    } else {
        PrintErrorMsg(NOEVAMSG,status);
    }
}



VOID
ConfigureSystemPartitions(
    VOID
    )
{
    ULONG Choice=0,DiskCount=GetDiskCount();
    PVOID MenuID;

    if (!AlNewMenu(&MenuID)) {
        MsgNoMem();
        return;
    }

    if (!AlAddMenuItems(MenuID,SysPartMenu,sizeof(SysPartMenu)/sizeof(PCHAR))) {
        MsgNoMem();
        AlFreeMenu(MenuID);
        return;
    }

    while(1) {

        if ((!AlDisplayMenu(MenuID,FALSE,Choice,&Choice,MENU_ROW,"Configure System Partitions"))
            || (Choice == SYSPARTMENU_EXIT))
        {
            break;
        }

        switch(Choice) {

        case SYSPARTMENU_CREATE:
            // create system partition.
            DoSystemPartitionCreate();
            break;

        case SYSPARTMENU_DELETE:
            // delete partition
            DoPartitionDelete();
            break;

        case SYSPARTMENU_ADD:
            // make existing into a system partition
            DoMakePartitionSystemPartition();
            break;
        }
    }

    AlFreeMenu(MenuID);
}
