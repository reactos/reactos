/*
 * PROJECT:     ReactOS Tools
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Check Registry Utility hive check analysis code
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "chkreg.h"

/* GLOBALS ******************************************************************/

BOOLEAN FixBrokenHive = FALSE;

/* DEFINES  *****************************************************************/

#define CMHIVE_TAG 'iHmC'
#define GET_HHIVE(CmHive) (&((CmHive)->Hive))

/* FUNCTIONS ****************************************************************/

static
BOOLEAN
ChkRegAnalyzeHiveHeader(
    IN PHBASE_BLOCK BaseBlock)
{
    ULONG CheckSum;

    /* A corrupt hive header block signature is fatal */
    if (BaseBlock->Signature != HV_HBLOCK_SIGNATURE)
    {
        printf("The registry hive header has a corrupt block signature and it could not be repaired!\n");
        return FALSE;
    }

    /* We do not support any other hives other than what we actually support */
    if (BaseBlock->Major != HSYS_MAJOR ||
        BaseBlock->Minor < HSYS_MINOR)
    {
        printf("The registry hive header has unsupported versions (Major 0x%x - Minor 0x%x)!\n", BaseBlock->Major, BaseBlock->Minor);
        return FALSE;
    }

    /* This hive has to be a primary hive otherwise this is wrong */
    if (BaseBlock->Type != HFILE_TYPE_PRIMARY)
    {
        printf("The registry hive header has a different file type (Type 0x%x, Expected 0x%x)!\n", BaseBlock->Type, HFILE_TYPE_PRIMARY);
        return FALSE;
    }

    /* This hive must be within a memory format otherwise this is fatal */
    if (BaseBlock->Format != HBASE_FORMAT_MEMORY)
    {
        printf("The registry hive header has an invalid base format and it could not be repaired!\n");
        return FALSE;
    }

    /* This hive must have a sane cluster size */
    if (BaseBlock->Cluster != 1)
    {
        printf("The registry hive header has an invalid cluster size (Cluster 0x%x)!\n", BaseBlock->Cluster);
        return FALSE;
    }

    /* Check the integrity of primary and secondary sequences, fix them if necessary */
    if (BaseBlock->Sequence1 != BaseBlock->Sequence2)
    {
        if (!FixBrokenHive)
        {
            printf("The registry hive header has mismatching sequences (Sequence1 0x%x - Sequence2 0x%x)!\n", BaseBlock->Sequence1, BaseBlock->Sequence2);
            return FALSE;
        }

        BaseBlock->Sequence2 = BaseBlock->Sequence1;
    }

    /* Check the hive checksum, fix it if necessary */
    CheckSum = HvpHiveHeaderChecksum(BaseBlock);
    if (BaseBlock->CheckSum != CheckSum)
    {
        if (!FixBrokenHive)
        {
            printf("The registry hive header has invalid checksum (BaseBlock->CheckSum 0x%x - CheckSum 0x%x)!\n", BaseBlock->CheckSum, CheckSum);
            return FALSE;
        }

        BaseBlock->CheckSum = CheckSum;
    }

    return TRUE;
}

BOOLEAN
ChkRegAnalyzeHive(
    IN PCSTR HiveName)
{
    CM_CHECK_REGISTRY_STATUS CmStatusCode;
    NTSTATUS Status;
    BOOLEAN Success;
    INT Confirm;
    PVOID HiveData;
    PCMHIVE CmHive;

    /* Ask the user whether he wants its hive fixed during analyzation */
    printf("\n"
           "The ReactOS Check Registry Utility is about to analyze %s. By default any damaged registry data is left unfixed!\n"
           "Do you want it fixed? [Y/N]\n\n", HiveName);
    Confirm = getchar();
    if (Confirm == 'Y' || Confirm == 'y')
    {
        /* Cache the request */
        FixBrokenHive = TRUE;
    }

    /* Open the hive */
    Success = ChkRegOpenHive(HiveName,
                             FixBrokenHive,
                             &HiveData);
    if (!Success)
    {
        printf("Failed to open the registry hive!\n");
        return FALSE;
    }

    /* Analyze the registry header block */
    printf("Analyzing the registry header...\n");
    Success = ChkRegAnalyzeHiveHeader((PHBASE_BLOCK)HiveData);
    if (!Success)
    {
        printf("The registry hive header has corrupt data!\n");
        CmpFree(HiveData, 0);
        return FALSE;
    }

    /* Now initialize the hive so that we can continue with further analyzation */
    CmHive = CmpAllocate(sizeof(CMHIVE),
                         TRUE,
                         CMHIVE_TAG);
    if (CmHive == NULL)
    {
        printf("Failed to allocate memory for CM hive!\n");
        return FALSE;
    }

    /* Initialize it */
    Status = ChkRegInitializeHive(GET_HHIVE(CmHive),
                                  HiveData);
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to initialize hive (Status 0x%lx)!\n", Status);
        HvFree(GET_HHIVE(CmHive));
        CmpFree(CmHive, 0);
        CmpFree(HiveData, 0);
        return FALSE;
    }

    /* Analyze the registry hive bins and underlying cells */
    printf("Analyzing the registry hive bins and cells...\n");
    CmStatusCode = HvValidateHive(GET_HHIVE(CmHive));
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        printf("The registry hive has corrupt bins or cells (status code %lu)!\n", CmStatusCode);
        HvFree(GET_HHIVE(CmHive));
        CmpFree(CmHive, 0);
        CmpFree(HiveData, 0);
        return FALSE;
    }

    /* Analyze the rest of the hive */
    printf("Analyzing the registry keys...\n");
    CmStatusCode = CmCheckRegistry(CmHive,
                                   (FixBrokenHive == TRUE) ? CM_CHECK_REGISTRY_PURGE_VOLATILES | CM_CHECK_REGISTRY_FIX_HIVE : CM_CHECK_REGISTRY_PURGE_VOLATILES);
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        printf("The registry hive has corrupt data (status code %lu)!\n", CmStatusCode);
        HvFree(GET_HHIVE(CmHive));
        CmpFree(CmHive, 0);
        CmpFree(HiveData, 0);
        return FALSE;
    }

    if (FixBrokenHive)
    {
        printf("Hive analyzation done. Any potential damaged registry data that were found are fixed!\n");
    }
    else
    {
        printf("Hive analyzation done.\n");
    }

    HvFree(GET_HHIVE(CmHive));
    CmpFree(CmHive, 0);
    CmpFree(HiveData, 0);
    return TRUE;
}

/* EOF */
