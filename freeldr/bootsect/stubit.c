#include <stdio.h>

FILE	*in;
FILE	*in2;
FILE	*out;

int main(int argc, char *argv[])
{
	unsigned char	ch;

	if (argc < 4)
	{
		printf("usage: stubit infile1.bin infile2.sys outfile.bin\n");
		return -1;
	}

	if ((in = fopen(argv[1], "rb")) == NULL)
	{
		printf("Couldn't open data file.\n");
		return -1;
	}
	if ((in2 = fopen(argv[2], "rb")) == NULL)
	{
		printf("Couldn't open data file.\n");
		return -1;
	}
	if ((out = fopen(argv[3], "wb")) == NULL)
	{
		printf("Couldn't open output file.\n");
		return -1;
	}

	ch = fgetc(in);
	while (!feof(in))
	{
		fputc(ch, out);
		ch = fgetc(in);
	}

	ch = fgetc(in2);
	while (!feof(in2))
	{
		fputc(ch, out);
		ch = fgetc(in2);
	}

	fclose(in);
	fclose(in2);
	fclose(out);

	return 0;
}