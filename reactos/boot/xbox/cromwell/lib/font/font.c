#include "font.h"


main()
{
	int n, n1=0;

	printf("const unsigned short waStarts[] = {\n");
	printf(" 0, 4, 12, 20, 29, 42, 53, 56, // <-- (\n");
	printf(" 62, 67, 73, 81, 85, 91, 95, 100, // <-- 0\n");
	printf(" 109, 117, 125, 133, 142, 149, 158, 166, // <-- 8\n");
	printf(" 174, 184, 189, 193, 202, 211, 219, 229, // <-- @\n");
	printf(" 243, 254, 265, 276, 288, 297, 306, 318, // <-- H\n");
	printf(" 330, 333, 343, 353, 362, 376, 386, 398, // <-- P\n");
	printf(" 407, 420, 430, 440, 449, 460, 470, 484, // <-- X\n");
	printf(" 495, 505, 514, 518, 523, 529, 535, 546, // <-- `\n");
	printf(" 550, 559, 568, 577, 585, 594, 600, 608, // <-- h\n");
	printf(" 618, 621, 626, 635, 639, 652, 662, 671, // <-- p\n");
	printf(" 680, 690, 696, 704, 709, 718, 727, 738, // <-- x\n");
	printf(" 747, 756, 763, 769, 774, 779, 788 // <-- end of bitmap\n");
	printf("};\n\nconst unsigned int uiPixelsX=%u, uiPixelsY=%u;\n\n", gimp_image.width, gimp_image.height);

	printf("const unsigned char baCharset[] = {\n");
	for(n=0;n<sizeof(gimp_image.pixel_data);n+=8) {
		unsigned char b=gimp_image.pixel_data[n]&0xf0;
		b|=gimp_image.pixel_data[n+4]>>4;
		printf("0x%02X", b^0xff);
		if((n+8)<sizeof(gimp_image.pixel_data)) printf(",");

		if((n1 & 15)==15) printf("\n");
		n1++;
	}
	printf("\n};\n\n");
	return 0;
} 
