#include <stdio.h>

char	in_filename[260];
char	out_filename[260];
FILE	*in;
FILE	*out;

int main(void)
{
	unsigned char	ch;
	int				cnt = 0;

	printf("Enter data filename: ");
	scanf("%s", in_filename);
	printf("Enter output filename: ");
	scanf("%s", out_filename);

	if ((in = fopen(in_filename, "rb")) == NULL)
	{
		printf("Couldn't open data file.\n");
		return 0;
	}
	if ((out = fopen(out_filename, "wb")) == NULL)
	{
		printf("Couldn't open output file.\n");
		return 0;
	}

	fprintf(out, "unsigned char data[] = {\n");

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