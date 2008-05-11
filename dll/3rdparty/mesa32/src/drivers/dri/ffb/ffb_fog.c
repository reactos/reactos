
/* FFB fog support:
 *
 * There are two levels of support for FOG in the Creator3D series.
 * Both involve a depth cue unit and 1 or 4 slope factors and scales
 * for varying the pixel intensity.
 *
 * Chips prior to FFB2 only have a single set of such settings, FFB2
 * and later have 4 settings.
 *
 * The basic depth cueing equation is:
 *
 *	C_final = dcsf(z) * C_orig + (1 - dcsf(z)) * C_fog
 *
 * 	C_final	--	The final color passed to blend unit or frame
 *			buffer (if blending is disabled).
 *
 *	C_orig	--	The color we start with, which comes either from
 *			the raster processor or cpu writes to the smart
 *			framebuffer aperture.
 *
 *	C_fog	--	This is the "fog" color, ie. the desired color
 *			at the deepest Z.
 *
 *	dcsf(z)	--	The depth cue scale as a function of Z.
 *
 * With pre-FFB2 chips there are four parameters to control the depth
 * cue scaling.  Here is a diagram:
 *
 *		   1.0	-------------
 *			|   |   |   |
 *			|   |   |   |
 *		Sfront	XXXXX---+---+
 *			|   |X  |   |
 *	dcsf(z)		|   | X |   |
 *			|   |  X|   |
 *		Sback	+---+---XXXXX
 *			|   |   |   |
 *		   0.0	-------------
 *			0.0 Zf  Zb 1.0
 *
 *			      z
 * Therefore:
 *
 *	for Zf < z < Zb
 *
 *		dcsf(z) = Sback + ((Sfront - Sback) / (Zf - Zb)) * (Zb - z)
 *
 *	for z <= Zf
 *
 *		dcsf(z) = Sfront
 *
 *	for z >= Zb
 *
 *		dcsf(z) = Sback
 *
 * With FFB2 and later, 3 more slope regions are provided, the first of
 * them starts at the end of the region defined above and ends at a
 * specified depth value, the next slop region starts there and ends
 * at the next specified depth value, and so on.  Each of the 3 slope
 * regions also have scale and slope settings of their own.
 *
 * The C_fog color is programmed into the alpha blending unit color1
 * and color2 registers as follows:
 *
 *	color1:		-(C_fog)
 *	color2:		C_fog - bg
 *
 * If alpha blending is disabled, the bg factor is zero.  Note that
 * the alpha blending color registers specify each of the RGB values
 * as 9 bit 1:8 signed numbers in the range -1.00 to 0.ff inclusive.
 * (ie. 0x100 == -1.00 and 0x0ff == +0.ff)
 */
