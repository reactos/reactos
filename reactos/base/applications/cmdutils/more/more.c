#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define rdtscll(val) __asm__ __volatile__ ("rdtsc" : "=A" (val))

const int SELECTMODE = 14;
const int BIGDATA = 10000; // Relying on int = long
const int MHZ = 2160;
int *data;

void SelectionSort(int data[], int left, int right) {
    int i, j;
	for(i = left; i < right; i++) {
		int min = i;
		for(j=i+1; j <= right; j++)
			if(data[j] < data[min]) min = j;
		int temp = data[min];
		data[min] = data[i];
		data[i] = temp;
	}
}

int Partition( int d[], int left, int right)
{
	int val =d[left];
	int lm = left-1;
	int rm = right+1;
	for(;;) {
		do
			rm--;
		while (d[rm] > val);
		
		do 
			lm++;
		while( d[lm] < val);

		if(lm < rm) {
			int tempr = d[rm];
			d[rm] = d[lm];
			d[lm] = tempr;
			}
		else 
			return rm;
		}
}

void Quicksort( int d[], int left, int right)
{
	if(left < (right-SELECTMODE)) {
		int split_pt = Partition(d,left, right);
		Quicksort(d, left, split_pt);
		Quicksort(d, split_pt+1, right);
		}
	else SelectionSort(d, left, right);
}

int main(int argc, char* argv[]) {

	data = (int*)calloc(BIGDATA,4);
	unsigned long int timeStart;

	unsigned long int timeReadLoopStart;
	unsigned long int timeReadLoopEnd;

	unsigned long int timeSortLoopStart;
	unsigned long int timeSortLoopEnd;

	unsigned long int timeWriteLoopStart;
	unsigned long int timeWriteLoopEnd;

	unsigned long int timeEnd;

	FILE *randfile;
	FILE *sortfile;
	int i,j,thisInt,dataSize = 0;
	long sumUnsorted = 0;

	rdtscll(timeStart);

	randfile = (argc < 2) ? stdin : fopen(argv[1],"r");
	sortfile = (argc < 3) ? stdout : fopen(argv[2],"w");
	if (randfile == NULL || sortfile == NULL) {
		fprintf(stderr,"Could not open all files.\n");
		return 1;
	}

	rdtscll(timeReadLoopStart);

	i = 0;
	while (!feof(randfile)) {
		fscanf(randfile,"%d",&thisInt);
		if (feof(randfile)) { break; }
		data[i] = thisInt;
		sumUnsorted += thisInt;
		//fprintf(stdout,"[%d] Read item: %d\n",i,thisInt);
		i++;
		if (i >= BIGDATA) { 
			break;
		}
	}
	fclose(randfile);
	dataSize = i;

	rdtscll(timeReadLoopEnd);
	rdtscll(timeSortLoopStart);
	
	Quicksort(data, 0, dataSize-1);

	rdtscll(timeSortLoopEnd);
	rdtscll(timeWriteLoopStart);

	int last = -1;
	for(j = 0; j < dataSize; j++) {
		if (data[j] < last) {
			fprintf(stderr,"The data is not in order\n");
			fprintf(stderr,"Noticed the problem at j = %d\n",j);
			fclose(sortfile);
			return 1;
		} else {
			fprintf(sortfile,"%d\n",data[j]);
		}
	}
	fclose(sortfile);	

	rdtscll(timeWriteLoopEnd);

	rdtscll(timeEnd);

	fprintf(stdout,"Sorted %d items.\n",dataSize);
	fprintf(stdout,"Open Files   : %ldt.\n",(long)timeReadLoopStart - (long)timeStart);
	fprintf(stdout,"Read Data    : %ldt.\n",(long)timeReadLoopEnd - (long)timeReadLoopStart);
	fprintf(stdout,"Sort Data    : %ldt.\n",(long)timeSortLoopEnd - (long)timeSortLoopStart);
	fprintf(stdout,"Write Data   : %ldt.\n",(long)timeWriteLoopEnd - (long)timeWriteLoopStart);
	fprintf(stdout,"Total Time   : %ldt.\n",(long)timeEnd - (long)timeStart);

	return 0;
}
