#include <stdio.h>

FILE	*in;
FILE	*out;
FILE	*new;

int main(int argc, char *argv[])
{
	unsigned char	ch;
	int				cnt;
	int				split_offset;

	if (argc < 5)
	{
		printf("usage: split infile.bin outfile.bin newfile.bin split_offset\n");
		return -1;
	}

	if ((in = fopen(argv[1], "rb")) == NULL)
	{
		printf("Couldn't open data file.\n");
		return -1;
	}
	if ((out = fopen(argv[2], "wb")) == NULL)
	{
		printf("Couldn't open output file.\n");
		return -1;
	}
	if ((new = fopen(argv[3], "wb")) == NULL)
	{
		printf("Couldn't open new file.\n");
		return -1;
	}

	split_offset = atoi(argv[4]);

	for (cnt=0; cnt<split_offset; cnt++)
	{
		ch = fgetc(in);
		fputc(ch, out);
	}

	ch = fgetc(in);
	while (!feof(in))
	{
		fputc(ch, new);
		ch = fgetc(in);
	}

	fclose(in);
	fclose(out);
	fclose(new);

	return 0;
}