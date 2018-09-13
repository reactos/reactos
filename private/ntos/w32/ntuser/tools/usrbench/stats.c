/*++

Copyright (c) 1995  Microsoft Corporation

Module Name

   stats.c

Abstract:

   Perform basic statistics on a sample array of data: Average, Standard Deviation Coefficient.
   Perform simple filtering of Data to meet a specified Standard Deviation Coeficient (Optional). 

Author:

   Dan Almosnino   (danalm)  20-Nov-1995 -  Wrote it
  
Enviornment:

   User Mode

Revision History:

++*/

#include "precomp.h"
#include "stats.h"

/*++

Routine Description:

    Shell for statistic processing of data

Arguments

	double 		*array, 			   Raw data array
	long 		num_samples, 		   Number of samples taken
	const 		filter_option, 		   available options: HI_FILTER, LO_FILTER, HI_LO_FILTER, NO_FILTER
	long 		var_limit, 			   Filter data so that its variation coefficient (StdDev/Average) 
									   Won't exceed var_limit (in whole percent units)
	PTEST_STATS stats)				   Pointer to struct that contains the calculated statistical data 
  									   Members:	double	Average, StdDev(%) (variation coefficient),
  									  					Minimum_Result, Maximum_Result
									  			long	NumSamplesValid (after filtering)
									   Notes: Minimum_Result and Max_Result are always before filtering.
									  	 	A negative value of StdDev means that the average was zero, and
									   		the number returned is the negative value of the standard deviation
									   		itself and not a relative number (coeficient of variation)  
Return Value

    TRUE for Success
	FALSE for Failure 	(Indicates a failure of one of the called functions due to either:
						 Too small No. of original Samples or Valid Samples left after filtering, or
						 Zero Arithmetic Average of data (filtering cannot continue), or
						 Invalid filter option.
						   
--*/

BOOL 
Get_Stats(
	double 		*array, 			  
	long 		num_samples, 		  
	const 		filter_option, 		  
	long 		var_limit, 			   
	PTEST_STATS stats)				   

	{  

		double 	Averagetmp;
		double 	StdDevtmp;
		long 	NumSamplestmp;

		if(!GetAverage(array, num_samples, &Averagetmp))
			return FALSE; 

		SortUp(array, num_samples);

		if(!GetStdDev(array, Averagetmp, num_samples, &StdDevtmp))
			return FALSE;

		stats->Minimum_Result	= array[0];
		stats->Maximum_Result	= array[num_samples - 1];

		if(filter_option == NO_FILTER)
		{
			stats->Average = Averagetmp;
			stats->StdDev  = StdDevtmp;
			stats->NumSamplesValid	= num_samples;
			return TRUE;
		}


		NumSamplestmp 	= num_samples;
		if(!FilterResults(array,&Averagetmp,&StdDevtmp,&NumSamplestmp,var_limit,filter_option))
			return FALSE;

		stats->Average	 			= Averagetmp;
		stats->StdDev 				= StdDevtmp;
		stats->NumSamplesValid		= NumSamplestmp;
		return TRUE;

	}

/*++

Routine Description:

    Calculate the arithmetic average of an array of numbers

Arguments

    double	Sample_Array	- data array
	long	No_Samples		- number of samples to calculate the average of
	double *Average			- pointer to the result

Return Value

    TRUE 	for Success
	FALSE 	for Failure (No_Samples is zero)

--*/


BOOL 
GetAverage(
	double 	Sample_Array[], 
	long 	No_Samples, 
	double 	*Average)
	{
		long i;
		(*Average) = 0.0;
		
		if(No_Samples == 0)return FALSE;

		for(i = 0; i < No_Samples; i++)
		{
			(*Average) += Sample_Array[i];
		}

		(*Average) /= No_Samples;

		return TRUE;

	}

/*++

Routine Description:

    Calculate the standard deviation of a set of data

Arguments

    double	Sample_Array	- The data array
	double	Average			- The arithmetic Average of the Sample_Array
	long	No_Samples		- Number of Samples to Calculate
	double *varcoef			- The resulting coeficient of variation (StdDev / Average) in percent
							  (If the average is zero then the standard deviation itself is returned in 
							  negative value) 

Return Value

    TRUE for 	Success
	FALSE for 	Failure	(No. of Samples is less than two)

--*/
					 


BOOL
GetStdDev(
	double 	Sample_Array[], 
	double 	Average, 
	long 	No_Samples, 
	double 	*varcoef)
	{
		long 	i;
		double 	tmp;
		double 	Sum = 0.0;

		if(No_Samples <= 1L)
		{
			return FALSE;
		}

		for(i=0; i<No_Samples; i++)
		{
			tmp = (Sample_Array[i] - Average);
			Sum += (tmp*tmp);
		}

		Sum /= (No_Samples - 1);

		if(fabs( (float)Average) < 1.E-6)
		{
			*varcoef = (-sqrt(Sum));	// return the Standard Deviation itself in negative sign for distinction
		} 			
		else
		{
			*varcoef = (100*sqrt(Sum)/Average);	// return the variation coeficient in percents
		}

		return TRUE;
	}

/*++

Routine Description:

    Simplest Quick Sort routine (sorts-up an array of doubles)

Arguments

    double *Array		- Pointer to the array to be sorted (result is returned in the same address)
	long	array_size	- Size of array to be sorted

Return Value

    none

--*/


void
SortUp(
	double 	*array, 
	long	array_size)
	{
		long gap, i, j;
		double temp;
			
		for(gap = array_size/2; gap > 0; gap /= 2)
			for(i = gap; i < array_size; i++)
				for(j = i - gap; j >= 0 && array[j] > array[j + gap]; j -= gap)
				{
					temp = array[j];
					array[j] = array[j + gap];
					array[j + gap] = temp;
				}

	}			

/*++

Routine Description:

    Filter a set of data to meet a pre-defined coeficient of variation

Arguments

	double Sorted_Array		- A pointer to a pre-sorted up array of data to be filtered
	double *tmpAverage		- A pointer to the arithmetic average of the array of data.
							  Upon exit will contain the resulting average of the filtered data.
	double *tmpSD			- A pointer to the variation coefficient (in percent) of the input array of data.
							  Upon exit will contain the resulting variation coefficient of the filtered data
							  after the constraint was met.
	long 	*tmpNumSamples	- A pointer to the number of samples to be filtered.
							  Upon return will contain the number of valid samples left after the filtering.
	long 	limit			- The pre defined coefficient of variation (StdDev / Average)to be met 
							  (in whole percents).
	const	filter_option	- One of the following:	
								HI_FILTER	- filter eliminates the high values from top down until limit is met
								LO_FILTER	- filter eliminates the low values from bottom up until limit is met
								HI_LO_FILTER- filter eliminates both high and low values in this order until limit is met
								other	- routine will return a FALSE 

Return Value

    TRUE 	for Success
	FALSE 	for failure (Either number of samples left after sequential filtering passes is less than three,
						 or for some reason the filtered average became zero,
						 or the filter option is an invalid value)

--*/



BOOL 
FilterResults(
	double 	Sorted_Array[], 
	double 	*tmpAverage, 
	double 	*tmpSD, 
	long 	*tmpNumSamples, 
	long 	limit,
	const	filter_option)
	{


		switch (filter_option)
		{
			case HI_FILTER:
			{
				while(*tmpSD > (double)limit)
				{

					if(*tmpNumSamples <= 2)return FALSE;

					(*tmpAverage) 	*= (*tmpNumSamples);
					(*tmpAverage) 	-= Sorted_Array[(*tmpNumSamples) - 1];
					(*tmpNumSamples)--;
					(*tmpAverage) 	/= (*tmpNumSamples);

					GetStdDev(Sorted_Array, *tmpAverage, *tmpNumSamples, &(*tmpSD));
				
				}
			
				if(*tmpSD < 0)return FALSE;
			}
			break;

			case LO_FILTER:
			{

				while(*tmpSD > (double)limit)
				{

					if(*tmpNumSamples <= 2)return FALSE;

					(*tmpAverage) 	*= (*tmpNumSamples);
					(*tmpAverage) 	-= (*Sorted_Array);
					Sorted_Array++;
					(*tmpNumSamples)--;
					(*tmpAverage) 	/= (*tmpNumSamples);

					GetStdDev(Sorted_Array, *tmpAverage, *tmpNumSamples, &(*tmpSD));
				
				}
			
				if(*tmpSD < 0)return FALSE;

			}
			break;

			case HI_LO_FILTER:
			{
				
				while(*tmpSD > (double)limit)
				{

					if(*tmpNumSamples <= 2)return FALSE;

					if(fabs((*Sorted_Array) - (*tmpAverage)) > fabs(Sorted_Array[*tmpNumSamples - 1] - *tmpAverage))
					{
						(*tmpAverage) 	*= (*tmpNumSamples);
						(*tmpAverage) 	-= (*Sorted_Array);
						Sorted_Array++;
					}
					else
					{
						(*tmpAverage) 	*= (*tmpNumSamples);
						(*tmpAverage) 	-= Sorted_Array[(*tmpNumSamples) - 1];
					}						

					(*tmpNumSamples)--;
					(*tmpAverage) 	/= (*tmpNumSamples);

					GetStdDev(Sorted_Array, *tmpAverage, *tmpNumSamples, &(*tmpSD));
				
				}
			
				if(*tmpSD < 0)return FALSE;

			}
			break;

			default:
			return FALSE;

		}				//switch filter_option

		return TRUE;
						
	}	
			 
			 

