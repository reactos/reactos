/*++

Copyright (c) 1995  Microsoft Corporation

Module Name

   stats.h

Abstract:

	Header file for Statistics module stats.c

Author:

   Dan Almosnino   (danalm)  20-Nov-1995
   Wrote it.

Enviornment:

   User Mode

Revision History:

++*/


#define HI_FILTER 		3
#define LO_FILTER		1
#define HI_LO_FILTER	2
#define NO_FILTER		0
#define HIGH_FILTER		HI_FILTER
#define LOW_FILTER		LO_FILTER
#define	HILO_FILTER		HI_LO_FILTER
#define HIGH_LOW_FILTER	HI_LO_FILTER


typedef struct _TEST_STATS
{
	double 	Average;
	double 	StdDev;
	double  Minimum_Result;
	double  Maximum_Result;
	long   	NumSamplesValid;

}TEST_STATS, *PTEST_STATS;	


BOOL 
Get_Stats(
	double 		*array, 
	long 		num_samples, 
	const 		filter_option, 
	long 		var_limit, 
	PTEST_STATS stats);

BOOL
GetAverage(
	double Sample_Array[],
	long No_Samples, 
	double *Average);

BOOL
GetStdDev(
	double Sample_Array[], 
	double Average, 
	long No_Samples,
	double *varcoef);

void 
SortUp(
	double *array, 
	long array_size);

BOOL 
FilterResults(
	double Sorted_Array[], 
	double *tmpAverage, 
	double *tmpSD, 
	long *tmpNumSamples, 
	long limit,
	const filter_option);




