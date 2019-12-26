/*
 * gdipluscolor.h
 *
 * GDI+ color
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Markus Koenig <markus@stber-koenig.de>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __GDIPLUS_COLOR_H
#define __GDIPLUS_COLOR_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

typedef enum ColorChannelFlags {
	ColorChannelFlagsC = 0,
	ColorChannelFlagsM = 1,
	ColorChannelFlagsY = 2,
	ColorChannelFlagsK = 3,
	ColorChannelFlagsLast = 4
} ColorChannelFlags;

typedef struct Color
{
	#ifdef __cplusplus
	private:
	#endif
	ARGB Value;

	#ifdef __cplusplus
	public:
	friend class Bitmap;
	friend class Graphics;
	friend class HatchBrush;
	friend class PathGradientBrush;
	friend class Pen;
	friend class SolidBrush;

	static ARGB MakeARGB(BYTE a, BYTE r, BYTE g, BYTE b)
	{
		return (ARGB) ((((DWORD) a) << 24) | (((DWORD) r) << 16)
		             | (((DWORD) g) << 8) | ((DWORD) b));
	}

	static const ARGB AlphaMask            = 0xFF000000UL;

	static const ARGB AliceBlue            = 0xFFF0F8FFUL;
	static const ARGB AntiqueWhite         = 0xFFFAEBD7UL;
	static const ARGB Aqua                 = 0xFF00FFFFUL;
	static const ARGB Aquamarine           = 0xFF7FFFD4UL;
	static const ARGB Azure                = 0xFFF0FFFFUL;
	static const ARGB Beige                = 0xFFF5F5DCUL;
	static const ARGB Bisque               = 0xFFFFE4C4UL;
	static const ARGB Black                = 0xFF000000UL;
	static const ARGB BlanchedAlmond       = 0xFFFFEBCDUL;
	static const ARGB Blue                 = 0xFF0000FFUL;
	static const ARGB BlueViolet           = 0xFF8A2BE2UL;
	static const ARGB Brown                = 0xFFA52A2AUL;
	static const ARGB BurlyWood            = 0xFFDEB887UL;
	static const ARGB CadetBlue            = 0xFF5F9EA0UL;
	static const ARGB Chartreuse           = 0xFF7FFF00UL;
	static const ARGB Chocolate            = 0xFFD2691EUL;
	static const ARGB Coral                = 0xFFFF7F50UL;
	static const ARGB CornflowerBlue       = 0xFF6495EDUL;
	static const ARGB Cornsilk             = 0xFFFFF8DCUL;
	static const ARGB Crimson              = 0xFFDC143CUL;
	static const ARGB Cyan                 = 0xFF00FFFFUL;
	static const ARGB DarkBlue             = 0xFF00008BUL;
	static const ARGB DarkCyan             = 0xFF008B8BUL;
	static const ARGB DarkGoldenrod        = 0xFFB8860BUL;
	static const ARGB DarkGray             = 0xFFA9A9A9UL;
	static const ARGB DarkGreen            = 0xFF006400UL;
	static const ARGB DarkKhaki            = 0xFFBDB76BUL;
	static const ARGB DarkMagenta          = 0xFF8B008BUL;
	static const ARGB DarkOliveGreen       = 0xFF556B2FUL;
	static const ARGB DarkOrange           = 0xFFFF8C00UL;
	static const ARGB DarkOrchid           = 0xFF9932CCUL;
	static const ARGB DarkRed              = 0xFF8B0000UL;
	static const ARGB DarkSalmon           = 0xFFE9967AUL;
	static const ARGB DarkSeaGreen         = 0xFF8FBC8FUL;
	static const ARGB DarkSlateBlue        = 0xFF483D8BUL;
	static const ARGB DarkSlateGray        = 0xFF2F4F4FUL;
	static const ARGB DarkTurquoise        = 0xFF00CED1UL;
	static const ARGB DarkViolet           = 0xFF9400D3UL;
	static const ARGB DeepPink             = 0xFFFF1493UL;
	static const ARGB DeepSkyBlue          = 0xFF00BFFFUL;
	static const ARGB DimGray              = 0xFF696969UL;
	static const ARGB DodgerBlue           = 0xFF1E90FFUL;
	static const ARGB Firebrick            = 0xFFB22222UL;
	static const ARGB FloralWhite          = 0xFFFFFAF0UL;
	static const ARGB ForestGreen          = 0xFF228B22UL;
	static const ARGB Fuchsia              = 0xFFFF00FFUL;
	static const ARGB Gainsboro            = 0xFFDCDCDCUL;
	static const ARGB GhostWhite           = 0xFFF8F8FFUL;
	static const ARGB Gold                 = 0xFFFFD700UL;
	static const ARGB Goldenrod            = 0xFFDAA520UL;
	static const ARGB Gray                 = 0xFF808080UL;
	static const ARGB Green                = 0xFF008000UL;
	static const ARGB GreenYellow          = 0xFFADFF2FUL;
	static const ARGB Honeydew             = 0xFFF0FFF0UL;
	static const ARGB HotPink              = 0xFFFF69B4UL;
	static const ARGB IndianRed            = 0xFFCD5C5CUL;
	static const ARGB Indigo               = 0xFF4B0082UL;
	static const ARGB Ivory                = 0xFFFFFFF0UL;
	static const ARGB Khaki                = 0xFFF0E68CUL;
	static const ARGB Lavender             = 0xFFE6E6FAUL;
	static const ARGB LavenderBlush        = 0xFFFFF0F5UL;
	static const ARGB LawnGreen            = 0xFF7CFC00UL;
	static const ARGB LemonChiffon         = 0xFFFFFACDUL;
	static const ARGB LightBlue            = 0xFFADD8E6UL;
	static const ARGB LightCoral           = 0xFFF08080UL;
	static const ARGB LightCyan            = 0xFFE0FFFFUL;
	static const ARGB LightGoldenrodYellow = 0xFFFAFAD2UL;
	static const ARGB LightGray            = 0xFFD3D3D3UL;
	static const ARGB LightGreen           = 0xFF90EE90UL;
	static const ARGB LightPink            = 0xFFFFB6C1UL;
	static const ARGB LightSalmon          = 0xFFFFA07AUL;
	static const ARGB LightSeaGreen        = 0xFF20B2AAUL;
	static const ARGB LightSkyBlue         = 0xFF87CEFAUL;
	static const ARGB LightSlateGray       = 0xFF778899UL;
	static const ARGB LightSteelBlue       = 0xFFB0C4DEUL;
	static const ARGB LightYellow          = 0xFFFFFFE0UL;
	static const ARGB Lime                 = 0xFF00FF00UL;
	static const ARGB LimeGreen            = 0xFF32CD32UL;
	static const ARGB Linen                = 0xFFFAF0E6UL;
	static const ARGB Magenta              = 0xFFFF00FFUL;
	static const ARGB Maroon               = 0xFF800000UL;
	static const ARGB MediumAquamarine     = 0xFF66CDAAUL;
	static const ARGB MediumBlue           = 0xFF0000CDUL;
	static const ARGB MediumOrchid         = 0xFFBA55D3UL;
	static const ARGB MediumPurple         = 0xFF9370DBUL;
	static const ARGB MediumSeaGreen       = 0xFF3CB371UL;
	static const ARGB MediumSlateBlue      = 0xFF7B68EEUL;
	static const ARGB MediumSpringGreen    = 0xFF00FA9AUL;
	static const ARGB MediumTurquoise      = 0xFF48D1CCUL;
	static const ARGB MediumVioletRed      = 0xFFC71585UL;
	static const ARGB MidnightBlue         = 0xFF191970UL;
	static const ARGB MintCream            = 0xFFF5FFFAUL;
	static const ARGB MistyRose            = 0xFFFFE4E1UL;
	static const ARGB Moccasin             = 0xFFFFE4B5UL;
	static const ARGB NavajoWhite          = 0xFFFFDEADUL;
	static const ARGB Navy                 = 0xFF000080UL;
	static const ARGB OldLace              = 0xFFFDF5E6UL;
	static const ARGB Olive                = 0xFF808000UL;
	static const ARGB OliveDrab            = 0xFF6B8E23UL;
	static const ARGB Orange               = 0xFFFFA500UL;
	static const ARGB OrangeRed            = 0xFFFF4500UL;
	static const ARGB Orchid               = 0xFFDA70D6UL;
	static const ARGB PaleGoldenrod        = 0xFFEEE8AAUL;
	static const ARGB PaleGreen            = 0xFF98FB98UL;
	static const ARGB PaleTurquoise        = 0xFFAFEEEEUL;
	static const ARGB PaleVioletRed        = 0xFFDB7093UL;
	static const ARGB PapayaWhip           = 0xFFFFEFD5UL;
	static const ARGB PeachPuff            = 0xFFFFDAB9UL;
	static const ARGB Peru                 = 0xFFCD853FUL;
	static const ARGB Pink                 = 0xFFFFC0CBUL;
	static const ARGB Plum                 = 0xFFDDA0DDUL;
	static const ARGB PowderBlue           = 0xFFB0E0E6UL;
	static const ARGB Purple               = 0xFF800080UL;
	static const ARGB Red                  = 0xFFFF0000UL;
	static const ARGB RosyBrown            = 0xFFBC8F8FUL;
	static const ARGB RoyalBlue            = 0xFF4169E1UL;
	static const ARGB SaddleBrown          = 0xFF8B4513UL;
	static const ARGB Salmon               = 0xFFFA8072UL;
	static const ARGB SandyBrown           = 0xFFF4A460UL;
	static const ARGB SeaGreen             = 0xFF2E8B57UL;
	static const ARGB SeaShell             = 0xFFFFF5EEUL;
	static const ARGB Sienna               = 0xFFA0522DUL;
	static const ARGB Silver               = 0xFFC0C0C0UL;
	static const ARGB SkyBlue              = 0xFF87CEEBUL;
	static const ARGB SlateBlue            = 0xFF6A5ACDUL;
	static const ARGB SlateGray            = 0xFF708090UL;
	static const ARGB Snow                 = 0xFFFFFAFAUL;
	static const ARGB SpringGreen          = 0xFF00FF7FUL;
	static const ARGB SteelBlue            = 0xFF4682B4UL;
	static const ARGB Tan                  = 0xFFD2B48CUL;
	static const ARGB Teal                 = 0xFF008080UL;
	static const ARGB Thistle              = 0xFFD8BFD8UL;
	static const ARGB Tomato               = 0xFFFF6347UL;
	static const ARGB Transparent          = 0x00FFFFFFUL;
	static const ARGB Turquoise            = 0xFF40E0D0UL;
	static const ARGB Violet               = 0xFFEE82EEUL;
	static const ARGB Wheat                = 0xFFF5DEB3UL;
	static const ARGB White                = 0xFFFFFFFFUL;
	static const ARGB WhiteSmoke           = 0xFFF5F5F5UL;
	static const ARGB Yellow               = 0xFFFFFF00UL;
	static const ARGB YellowGreen          = 0xFF9ACD32UL;

	Color(): Value(0xFF000000) {}
	Color(ARGB argb): Value(argb) {}
	Color(BYTE r, BYTE g, BYTE b): Value(MakeARGB(0xFF, r, g, b)) {}
	Color(BYTE a, BYTE r, BYTE g, BYTE b): Value(MakeARGB(a, r, g, b)) {}

	BYTE GetA() const
	{
		return (BYTE) (Value >> 24);
	}
	BYTE GetAlpha() const
	{
		return (BYTE) (Value >> 24);
	}
	BYTE GetB() const
	{
		return (BYTE) Value;
	}
	BYTE GetBlue() const
	{
		return (BYTE) Value;
	}
	BYTE GetG() const
	{
		return (BYTE) (Value >> 8);
	}
	BYTE GetGreen() const
	{
		return (BYTE) (Value >> 8);
	}
	BYTE GetR() const
	{
		return (BYTE) (Value >> 16);
	}
	BYTE GetRed() const
	{
		return (BYTE) (Value >> 16);
	}
	ARGB GetValue() const
	{
		return Value;
	}
	VOID SetFromCOLORREF(COLORREF rgb)
	{
		BYTE r = (BYTE) rgb;
		BYTE g = (BYTE) (rgb >> 8);
		BYTE b = (BYTE) (rgb >> 16);
		Value = MakeARGB(0xFF, r, g, b);
	}
	VOID SetValue(ARGB argb)
	{
		Value = argb;
	}
	COLORREF ToCOLORREF() const
	{
		return RGB(GetRed(), GetGreen(), GetBlue());
	}
	#endif /* __cplusplus */
} Color;

#endif /* __GDIPLUS_COLOR_H */
