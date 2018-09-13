#ifndef RUBY_DEFINED
#define RUBY_DEFINED

#include	"lsimeth.h"

/* Only valid version number for Ruby initialization */
#define RUBY_VERSION 0x300

/* Used for intialization to tell Ruby object which line comes first */
typedef enum rubysyntax { RubyPronunciationLineFirst, RubyMainLineFirst } RUBYSYNTAX;


/* Type of Adjustment for Ruby text returned by FetchRubyPosition callback */
enum rubycharjust { 
	rcjCenter, 		/* Centering occurs on longer text line */

	rcj010, 		/* Difference in space between longer text
					 * and shorter is distributed in the shorter
					 * string evenly between each character 
					 */

	rcj121,			/* Difference in space between longer string
					 * and shorter is distributed in the shorter
					 * using a ratio of 1:2:1 which corresponds
					 * to lead : inter-character : end.
					 */

	rcjLeft,		/* Align ruby with the left of the main line.
					 */

	rcjRight		/* Align ruby with the right of the main line.
					 */
};

/* Location of character input to FetchRubyWidthAdjust callback */
enum rubycharloc {
	rubyBefore,		/* Character preceeds Ruby object */
	rubyAfter		/* Character follows Ruby object */
};

/*
 *
 *	Ruby Object callbacks to client application
 *
 */
typedef struct RUBYCBK
{
	LSERR (WINAPI *pfnFetchRubyPosition)(
		POLS pols,
		LSCP cp,
		LSTFLOW lstflow,
		DWORD cdwMainRuns,
		const PLSRUN *pplsrunMain,
		PCHEIGHTS pcheightsRefMain,
		PCHEIGHTS pcheightsPresMain,
		DWORD cdwRubyRuns,
		const PLSRUN *pplsrunRuby,
		PCHEIGHTS pcheightsRefRuby,
		PCHEIGHTS pcheightsPresRuby,
		PHEIGHTS pheightsRefRubyObj,
		PHEIGHTS pheightsPresRubyObj,
		long *pdvpOffsetMainBaseline,
		long *pdvrOffsetRubyBaseline,
		long *pdvpOffsetRubyBaseline,
		enum rubycharjust *prubycharjust,
		BOOL *pfSpecialLineStartEnd);

	/* FetchRubyPosition
	 *  pols (IN): The client context for the request.
	 *
	 *  cp (IN): the cp of the Ruby object.
	 *
	 *  lstflow (IN): the lstflow of Ruby parent subline
	 *
	 *	pplsrunMain	(IN): array of PLSRUNs created by the client application 
	 *			for each of the runs in the main text for the Ruby object.
	 *
	 *	pcheightsRefMain (IN): height of the line of the main text in reference 
	 *			device units.
	 *
	 *	pcheightsPresMain (IN): height of the line of the main text in presentation
	 *			device units.
	 *
	 *	cdwRubyRuns	(IN): count of pronunciation runs supplied in the following 
	 *			parameter.
	 *
	 *	pplsrunRuby	(IN): array of PLSRUNS created by the client application for 
	 *			each of the runs in the pronunciation text for the Ruby object.
	 *
	 *	pcheightsRefRuby (IN): height of the line of the Ruby pronunciation text in 
	 *			reference device units.
	 *
	 *	pcheightsPresRuby (IN): height of the line of the Ruby pronunciation text in 
	 *			presentation device units.
	 *
	 *	pheightsRefRubyObj	(OUT): returned height values in reference device units
	 *			that ruby object will report back to line services.
	 *
	 *	pheightsPresRubyObj	(OUT): returned height values in presentation device units
	 *			that ruby object will report back to line services.
	 *
	 *	pdvpOffsetMainBaseline (OUT): offset of baseline of main line of Ruby
	 *			text from base line of Ruby object in presentation units. Note
	 *			a negative value puts the baseline of the main line below the 
	 *			base line of the Ruby object.
	 *
	 *	pdvrOffsetRubyBaseline (OUT): offset of baseline of pronunciation line
	 *			of Ruby text from base line of Ruby object in reference units. 
	 *			Note a negative value puts the baseline of the pronunciation line 
	 *			below the base line of the Ruby object.
	 *
	 *	pdvpOffsetRubyBaseline (OUT): offset of baseline of pronunciation line
	 *			of Ruby text from base line of Ruby object in presentation units. 
	 *			Note a negative value puts the baseline of the pronunciation line 
	 *			below the base line of the Ruby object.
	 *
	 *	prubycharjust (OUT): type of justification to use for Ruby Object.
	 *
	 *	pfSpecialLineStartEnd (OUT): specifies that the optional alignment that 
	 *			overrides the usual centering algorithm when the Ruby is the 
	 *			first or last character of the line.
	 *
	 */

	LSERR (WINAPI *pfnFetchRubyWidthAdjust)(
		POLS pols,
		LSCP cp,
		PLSRUN plsrunForChar,
		WCHAR wch,
		MWCLS mwclsForChar,
		PLSRUN plsrunForRuby,
		enum rubycharloc rcl,
		long durMaxOverhang,
		long *pdurAdjustChar,
		long *pdurAdjustRuby);

	/* FetchRubyWidthAdjust
	 *  pols (IN): The client context for the request.
	 *
	 *  cp (IN): the cp of the Ruby object.
	 *
	 *	plsrunForChar (IN): the run that is either previous or following the 
	 *			Ruby object.
	 *
	 *	wch (IN): character that is either before or after the Ruby object.
	 *
	 *	mwcls (IN): mod width class for the character.
	 *
	 *	plsrunForRuby (IN): plsrun for entire ruby object.
	 *
	 *	rcl	(IN): tells the location of the character.
	 *
	 *	durMaxOverhang (IN): designates the maximum amount of overhang that is 
	 *			possible following the JIS spec with respect to overhang. 
	 *			Adjusting the Ruby object by a negative value whose absolute 
	 *			value is greater than durMaxOverhang will cause part of the 
	 *			main text to be clipped. If the value of this parameter is 0, 
	 *			this indicates that there is no possible overhang.
	 *
	 *	pdurAdjustChar (OUT): designates the amount to adjust the width of the 
	 *			character prior to or following the Ruby object. Returing a negative 
	 *			value will decrease the size of the character preceeding or following 
	 *			the Ruby while returning a positive value will increase the size 
	 *			of that character.
	 *
	 *	pdurAdjustRuby (OUT): designates the amount adjust the width of the Ruby 
	 *			object. Returing a negative value will decrease the size of the 
	 *			Ruby object and potentially cause the Ruby pronunciation text to 
	 *			overhang the preceeding or following character while returning a 
	 *			positive value will increase the size of the Ruby object.
	 */

	LSERR (WINAPI* pfnRubyEnum)(
		POLS pols,
		PLSRUN plsrun,		
		PCLSCHP plschp,	
		LSCP cp,		
		LSDCP dcp,		
		LSTFLOW lstflow,	
		BOOL fReverse,		
		BOOL fGeometryNeeded,	
		const POINT* pt,		
		PCHEIGHTS pcheights,	
		long dupRun,		
		const POINT *ptMain,	
		PCHEIGHTS pcheightsMain,
		long dupMain,		
		const POINT *ptRuby,	
		PCHEIGHTS pcheightsRuby,
		long dupRuby,	
		PLSSUBL plssublMain,	
		PLSSUBL plssublRuby);	

	/* RubyEnum
	 * 
	 *	pols (IN): client context.
	 *
	 *  plsrun (IN): plsrun for the entire Ruby Object.
	 *
	 *	plschp (IN): is lschp for lead character of Ruby Object.
	 *
	 *	cp (IN): is cp of first character of Ruby Object.
	 *
	 *	dcp (IN): is number of characters in Ruby Object
	 *
	 *	lstflow (IN): is text flow at Ruby Object.
	 *
	 *	fReverse (IN): is whether text should be reversed for visual order.
	 *
	 *	fGeometryNeeded (IN): is whether Geometry should be returned.
	 *
	 *	pt (IN): is starting position , iff fGeometryNeeded .
	 *
	 *	pcheights (IN):	is height of Ruby object, iff fGeometryNeeded.
	 *
	 *	dupRun (IN): is length of Ruby Object, iff fGeometryNeeded.
	 *
	 *	ptMain (IN): is starting point for main line iff fGeometryNeeded
	 *
	 *	pcheightsMain (IN): is height of main line iff fGeometryNeeded
	 *
	 *	dupMain (IN): is length of main line iff fGeometryNeeded
	 *
	 *	ptRuby (IN): is point for Ruby pronunciation line iff fGeometryNeeded
	 *
	 *	pcheightsRuby (IN): is height for ruby line iff fGeometryNeeded
	 *
	 *	dupRuby (IN): is length of Ruby line iff fGeometryNeeded
	 *
	 *	plssublMain (IN): is main subline.
	 *
	 *	plssublRuby (IN): is Ruby subline.
	 *
	 */

} RUBYCBK;

/*
 *
 *	Ruby Object initialization data that the client application must return
 *	when the Ruby object handler calls the GetObjectHandlerInfo callback.
 *
 */
typedef struct RUBYINIT
{
	DWORD				dwVersion;		/* Version of the structure */
	RUBYSYNTAX			rubysyntax;		/* Used to determine order of lines during format */
	WCHAR				wchEscRuby;		/* Escape char for end of Ruby pronunciation line */
	WCHAR				wchEscMain;		/* Escape char for end of main text */
	WCHAR				wchUnused1;		/* For aligment */
	WCHAR				wchUnused2;		/* For aligment */
	RUBYCBK				rcbk;			/* Ruby callbacks */
} RUBYINIT;

LSERR WINAPI LsGetRubyLsimethods(
	LSIMETHODS *plsim);

/* GetRubyLsimethods
 *
 *	plsim (OUT): Ruby object methods for Line Services.
 *
 */

#endif /* RUBY_DEFINED */

