/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling definitions for ADDJOB_INFO_* and JOB_INFO_*
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

static const MARSHALLING AddJobInfo1Marshalling = {
    sizeof(ADDJOB_INFO_1W),
    {
        { FIELD_OFFSET(ADDJOB_INFO_1W, Path), RTL_FIELD_SIZE(ADDJOB_INFO_1W, Path), RTL_FIELD_SIZE(ADDJOB_INFO_1W, Path), TRUE },
        { FIELD_OFFSET(ADDJOB_INFO_1W, JobId), RTL_FIELD_SIZE(ADDJOB_INFO_1W, JobId), RTL_FIELD_SIZE(ADDJOB_INFO_1W, JobId), FALSE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING JobInfo1Marshalling = {
    sizeof(JOB_INFO_1W),
    {
        { FIELD_OFFSET(JOB_INFO_1W, JobId), RTL_FIELD_SIZE(JOB_INFO_1W, JobId), RTL_FIELD_SIZE(JOB_INFO_1W, JobId), FALSE },
        { FIELD_OFFSET(JOB_INFO_1W, pPrinterName), RTL_FIELD_SIZE(JOB_INFO_1W, pPrinterName), RTL_FIELD_SIZE(JOB_INFO_1W, pPrinterName), TRUE },
        { FIELD_OFFSET(JOB_INFO_1W, pMachineName), RTL_FIELD_SIZE(JOB_INFO_1W, pMachineName), RTL_FIELD_SIZE(JOB_INFO_1W, pMachineName), TRUE },
        { FIELD_OFFSET(JOB_INFO_1W, pUserName), RTL_FIELD_SIZE(JOB_INFO_1W, pUserName), RTL_FIELD_SIZE(JOB_INFO_1W, pUserName), TRUE },
        { FIELD_OFFSET(JOB_INFO_1W, pDocument), RTL_FIELD_SIZE(JOB_INFO_1W, pDocument), RTL_FIELD_SIZE(JOB_INFO_1W, pDocument), TRUE },
        { FIELD_OFFSET(JOB_INFO_1W, pDatatype), RTL_FIELD_SIZE(JOB_INFO_1W, pDatatype), RTL_FIELD_SIZE(JOB_INFO_1W, pDatatype), TRUE },
        { FIELD_OFFSET(JOB_INFO_1W, pStatus), RTL_FIELD_SIZE(JOB_INFO_1W, pStatus), RTL_FIELD_SIZE(JOB_INFO_1W, pStatus), TRUE },
        { FIELD_OFFSET(JOB_INFO_1W, Status), RTL_FIELD_SIZE(JOB_INFO_1W, Status), RTL_FIELD_SIZE(JOB_INFO_1W, Status), FALSE },
        { FIELD_OFFSET(JOB_INFO_1W, Priority), RTL_FIELD_SIZE(JOB_INFO_1W, Priority), RTL_FIELD_SIZE(JOB_INFO_1W, Priority), FALSE },
        { FIELD_OFFSET(JOB_INFO_1W, Position), RTL_FIELD_SIZE(JOB_INFO_1W, Position), RTL_FIELD_SIZE(JOB_INFO_1W, Position), FALSE },
        { FIELD_OFFSET(JOB_INFO_1W, TotalPages), RTL_FIELD_SIZE(JOB_INFO_1W, TotalPages), RTL_FIELD_SIZE(JOB_INFO_1W, TotalPages), FALSE },
        { FIELD_OFFSET(JOB_INFO_1W, PagesPrinted), RTL_FIELD_SIZE(JOB_INFO_1W, PagesPrinted), RTL_FIELD_SIZE(JOB_INFO_1W, PagesPrinted), FALSE },
        { FIELD_OFFSET(JOB_INFO_1W, Submitted), RTL_FIELD_SIZE(JOB_INFO_1W, Submitted), sizeof(WORD), FALSE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING JobInfo2Marshalling = {
    sizeof(JOB_INFO_2W),
    {
        { FIELD_OFFSET(JOB_INFO_2W, JobId), RTL_FIELD_SIZE(JOB_INFO_2W, JobId), RTL_FIELD_SIZE(JOB_INFO_2W, JobId), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, pPrinterName), RTL_FIELD_SIZE(JOB_INFO_2W, pPrinterName), RTL_FIELD_SIZE(JOB_INFO_2W, pPrinterName), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pMachineName), RTL_FIELD_SIZE(JOB_INFO_2W, pMachineName), RTL_FIELD_SIZE(JOB_INFO_2W, pMachineName), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pUserName), RTL_FIELD_SIZE(JOB_INFO_2W, pUserName), RTL_FIELD_SIZE(JOB_INFO_2W, pUserName), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pDocument), RTL_FIELD_SIZE(JOB_INFO_2W, pDocument), RTL_FIELD_SIZE(JOB_INFO_2W, pDocument), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pNotifyName), RTL_FIELD_SIZE(JOB_INFO_2W, pNotifyName), RTL_FIELD_SIZE(JOB_INFO_2W, pNotifyName), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pDatatype), RTL_FIELD_SIZE(JOB_INFO_2W, pDatatype), RTL_FIELD_SIZE(JOB_INFO_2W, pDatatype), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pPrintProcessor), RTL_FIELD_SIZE(JOB_INFO_2W, pPrintProcessor), RTL_FIELD_SIZE(JOB_INFO_2W, pPrintProcessor), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pParameters), RTL_FIELD_SIZE(JOB_INFO_2W, pParameters), RTL_FIELD_SIZE(JOB_INFO_2W, pParameters), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pDriverName), RTL_FIELD_SIZE(JOB_INFO_2W, pDriverName), RTL_FIELD_SIZE(JOB_INFO_2W, pDriverName), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pDevMode), RTL_FIELD_SIZE(JOB_INFO_2W, pDevMode), RTL_FIELD_SIZE(JOB_INFO_2W, pDevMode), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pStatus), RTL_FIELD_SIZE(JOB_INFO_2W, pStatus), RTL_FIELD_SIZE(JOB_INFO_2W, pStatus), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, pSecurityDescriptor), RTL_FIELD_SIZE(JOB_INFO_2W, pSecurityDescriptor), RTL_FIELD_SIZE(JOB_INFO_2W, pSecurityDescriptor), TRUE },
        { FIELD_OFFSET(JOB_INFO_2W, Status), RTL_FIELD_SIZE(JOB_INFO_2W, Status), RTL_FIELD_SIZE(JOB_INFO_2W, Status), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, Priority), RTL_FIELD_SIZE(JOB_INFO_2W, Priority), RTL_FIELD_SIZE(JOB_INFO_2W, Priority), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, Position), RTL_FIELD_SIZE(JOB_INFO_2W, Position), RTL_FIELD_SIZE(JOB_INFO_2W, Position), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, StartTime), RTL_FIELD_SIZE(JOB_INFO_2W, StartTime), RTL_FIELD_SIZE(JOB_INFO_2W, StartTime), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, UntilTime), RTL_FIELD_SIZE(JOB_INFO_2W, UntilTime), RTL_FIELD_SIZE(JOB_INFO_2W, UntilTime), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, TotalPages), RTL_FIELD_SIZE(JOB_INFO_2W, TotalPages), RTL_FIELD_SIZE(JOB_INFO_2W, TotalPages), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, Size), RTL_FIELD_SIZE(JOB_INFO_2W, Size), RTL_FIELD_SIZE(JOB_INFO_2W, Size), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, Submitted), RTL_FIELD_SIZE(JOB_INFO_2W, Submitted), sizeof(WORD), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, Time), RTL_FIELD_SIZE(JOB_INFO_2W, Time), RTL_FIELD_SIZE(JOB_INFO_2W, Time), FALSE },
        { FIELD_OFFSET(JOB_INFO_2W, PagesPrinted), RTL_FIELD_SIZE(JOB_INFO_2W, PagesPrinted), RTL_FIELD_SIZE(JOB_INFO_2W, PagesPrinted), FALSE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING* pJobInfoMarshalling[] = {
    NULL,
    &JobInfo1Marshalling,
    &JobInfo2Marshalling
};
