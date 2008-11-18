/*
Workaround for overflow bug in Mozilla 1.1 and earlier, where scrolling
<div>s in <td> cells collapse their height to a single line.

Known to be fixed in 1.2.1 (Gecko 20021130), but the CSS hacks I've tried
with overflow-x disable the scrolling all the way until Mozilla 1.8 / FF 1.5
and break Opera as well.

So... we check for reaaaally old Gecko and hack in an alternate rule to let
the wide cells spill instead of scrolling them. Not ideal as it won't work
if JS is disabled, of course.
*/

if (navigator && navigator.product == "Gecko" && navigator.productSub < "20021130") {
	var sheets = document.styleSheets;
	var lastSheet = sheets[sheets.length-1];
	lastSheet.insertRule(
		"table.diff td div { overflow: visible; }",
		lastSheet.cssRules.length);
}