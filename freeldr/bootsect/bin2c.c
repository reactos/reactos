#include <stdio.h>

FILE	*in;
FILE	*out;

int main(int argc, char *argv[])
{
	unsigned char	ch;
	int				cnt = 0;

	if (argc < 4)
	{
		printf("usage: bin2c infile.bin outfile.h array_name\n");
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

	fprintf(out, "unsigned char %s[] = {\n", argv[3]);

	ch = fgetc(in);
	while (!feof(in))
	{
		if (cnt != 0)
			fprintf(out, ", ");
		if (!(cnt % 16))
			fprintf(out, "\n");
		fprintf(out, "0x%02x", (int)ch);
		cnt++;
		ch = fgetc(in);
	}

	fprintf(out, "\n};");

	fclose(in);
	fclose(out);

	return 0;
}
