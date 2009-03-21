/*
 
Name:    diff.js
Version: 0.9.5a (April 6, 2008)
Info:    http://en.wikipedia.org/wiki/User:Cacycle/diff
Code:    http://en.wikipedia.org/wiki/User:Cacycle/diff.js
 
JavaScript diff algorithm by [[en:User:Cacycle]] (http://en.wikipedia.org/wiki/User_talk:Cacycle).
Outputs html/css-formatted new text with highlighted deletions, inserts, and block moves.
 
The program uses cross-browser code and should work with all modern browsers. It has been tested with:
* Mozilla Firefox 1.5.0.1
* Mozilla SeaMonkey 1.0
* Opera 8.53
* Internet Explorer 6.0.2900.2180
* Internet Explorer 7.0.5730.11
This program is also compatibel with Greasemonkey
 
An implementation of the word-based algorithm from:
 
Communications of the ACM 21(4):264 (1978)
http://doi.acm.org/10.1145/359460.359467
 
With the following additional feature:
 
* Word types have been optimized for MediaWiki source texts
* Additional post-pass 5 code for resolving islands caused by adding
	two common words at the end of sequences of common words
* Additional detection of block borders and color coding of moved blocks and their original position
* Optional "intelligent" omission of unchanged parts from the output
 
This code is used by the MediaWiki in-browser text editors [[en:User:Cacycle/editor]] and [[en:User:Cacycle/wikEd]]
and the enhanced diff view tool wikEdDiff [[en:User:Cacycle/wikEd]].
 
Usage: var htmlText = WDiffString(oldText, newText);
 
This code has been released into the public domain.
 
Datastructures:
 
text: an object that holds all text related datastructures
	.newWords: consecutive words of the new text (N)
	.oldWords: consecutive words of the old text (O)
	.newToOld: array of corresponding word number in old text (NA)
	.oldToNew: array of corresponding word number in new text (OA)
	.message:  output message for testing purposes
 
symbol['word']: symbol table for passes 1 - 3, holds words as a hash
	.newCtr:  new word occurences counter (NC)
	.oldCtr:  old word occurences counter (OC)
	.toNew:   table last old word number
	.toOld:   last new word number (OLNA)
 
block: an object that holds block move information
	blocks indexed after new text:
	.newStart:  new text word number of start of this block
	.newLength: element number of this block including non-words
	.newWords:  true word number of this block
	.newNumber: corresponding block index in old text
	.newBlock:  moved-block-number of a block that has been moved here
	.newLeft:   moved-block-number of a block that has been moved from this border leftwards
	.newRight:  moved-block-number of a block that has been moved from this border rightwards
	.newLeftIndex:  index number of a block that has been moved from this border leftwards
	.newRightIndex: index number of a block that has been moved from this border rightwards
	blocks indexed after old text:
	.oldStart:  word number of start of this block
	.oldToNew:  corresponding new text word number of start
	.oldLength: element number of this block including non-words
	.oldWords:  true word number of this block
 
*/
 
 
// css for change indicators
if (typeof(wDiffStyleDelete) == 'undefined') { window.wDiffStyleDelete = 'font-weight: normal; text-decoration: none; color: #fff; background-color: #990033;'; }
if (typeof(wDiffStyleInsert) == 'undefined') { window.wDiffStyleInsert = 'font-weight: normal; text-decoration: none; color: #fff; background-color: #009933;'; }
if (typeof(wDiffStyleMoved)  == 'undefined') { window.wDiffStyleMoved  = 'font-weight: bold;  color: #000; vertical-align: text-bottom; font-size: xx-small; padding: 0; border: solid 1px;'; }
if (typeof(wDiffStyleBlock)  == 'undefined') { window.wDiffStyleBlock  = [
	'color: #000; background-color: #ffff80;',
	'color: #000; background-color: #c0ffff;',
	'color: #000; background-color: #ffd0f0;',
	'color: #000; background-color: #ffe080;',
	'color: #000; background-color: #aaddff;',
	'color: #000; background-color: #ddaaff;',
	'color: #000; background-color: #ffbbbb;',
	'color: #000; background-color: #d8ffa0;',
	'color: #000; background-color: #d0d0d0;'
]; }
 
// html for change indicators, {number} is replaced by the block number
// {block} is replaced by the block style, class and html comments are important for shortening the output
if (typeof(wDiffHtmlMovedRight)  == 'undefined') { window.wDiffHtmlMovedRight  = '<input class="wDiffHtmlMovedRight" type="button" value="&gt;" style="' + wDiffStyleMoved + ' {block}"><!--wDiffHtmlMovedRight-->'; }
if (typeof(wDiffHtmlMovedLeft)   == 'undefined') { window.wDiffHtmlMovedLeft   = '<input class="wDiffHtmlMovedLeft" type="button" value="&lt;" style="' + wDiffStyleMoved + ' {block}"><!--wDiffHtmlMovedLeft-->'; }
 
if (typeof(wDiffHtmlBlockStart)  == 'undefined') { window.wDiffHtmlBlockStart  = '<span class="wDiffHtmlBlock" style="{block}">'; }
if (typeof(wDiffHtmlBlockEnd)    == 'undefined') { window.wDiffHtmlBlockEnd    = '</span><!--wDiffHtmlBlock-->'; }
 
if (typeof(wDiffHtmlDeleteStart) == 'undefined') { window.wDiffHtmlDeleteStart = '<span class="wDiffHtmlDelete" style="' + wDiffStyleDelete + '">'; }
if (typeof(wDiffHtmlDeleteEnd)   == 'undefined') { window.wDiffHtmlDeleteEnd   = '</span><!--wDiffHtmlDelete-->'; }
 
if (typeof(wDiffHtmlInsertStart) == 'undefined') { window.wDiffHtmlInsertStart = '<span class="wDiffHtmlInsert" style="' + wDiffStyleInsert + '">'; }
if (typeof(wDiffHtmlInsertEnd)   == 'undefined') { window.wDiffHtmlInsertEnd   = '</span><!--wDiffHtmlInsert-->'; }
 
// minimal number of real words for a moved block (0 for always displaying block move indicators)
if (typeof(wDiffBlockMinLength) == 'undefined') { window.wDiffBlockMinLength = 3; }
 
// exclude identical sequence starts and endings from change marking
if (typeof(wDiffWordDiff) == 'undefined') { window.wDiffWordDiff = true; }
 
// enable recursive diff to resolve problematic sequences
if (typeof(wDiffRecursiveDiff) == 'undefined') { window.wDiffRecursiveDiff = true; }
 
// enable block move display
if (typeof(wDiffShowBlockMoves) == 'undefined') { window.wDiffShowBlockMoves = true; }
 
// remove unchanged parts from final output
 
// characters before diff tag to search for previous heading, paragraph, line break, cut characters
if (typeof(wDiffHeadingBefore)   == 'undefined') { window.wDiffHeadingBefore   = 1500; }
if (typeof(wDiffParagraphBefore) == 'undefined') { window.wDiffParagraphBefore = 1500; }
if (typeof(wDiffLineBeforeMax)   == 'undefined') { window.wDiffLineBeforeMax   = 1000; }
if (typeof(wDiffLineBeforeMin)   == 'undefined') { window.wDiffLineBeforeMin   =  500; }
if (typeof(wDiffBlankBeforeMax)  == 'undefined') { window.wDiffBlankBeforeMax  = 1000; }
if (typeof(wDiffBlankBeforeMin)  == 'undefined') { window.wDiffBlankBeforeMin  =  500; }
if (typeof(wDiffCharsBefore)     == 'undefined') { window.wDiffCharsBefore     =  500; }
 
// characters after diff tag to search for next heading, paragraph, line break, or characters
if (typeof(wDiffHeadingAfter)   == 'undefined') { window.wDiffHeadingAfter   = 1500; }
if (typeof(wDiffParagraphAfter) == 'undefined') { window.wDiffParagraphAfter = 1500; }
if (typeof(wDiffLineAfterMax)   == 'undefined') { window.wDiffLineAfterMax   = 1000; }
if (typeof(wDiffLineAfterMin)   == 'undefined') { window.wDiffLineAfterMin   =  500; }
if (typeof(wDiffBlankAfterMax)  == 'undefined') { window.wDiffBlankAfterMax  = 1000; }
if (typeof(wDiffBlankAfterMin)  == 'undefined') { window.wDiffBlankAfterMin  =  500; }
if (typeof(wDiffCharsAfter)     == 'undefined') { window.wDiffCharsAfter     =  500; }
 
// maximal fragment distance to join close fragments
if (typeof(wDiffFragmentJoin)  == 'undefined') { window.wDiffFragmentJoin = 1000; }
if (typeof(wDiffOmittedChars)  == 'undefined') { window.wDiffOmittedChars = '…'; }
if (typeof(wDiffOmittedLines)  == 'undefined') { window.wDiffOmittedLines = '<hr style="height: 2px; margin: 1em 10%;">'; }
if (typeof(wDiffNoChange)      == 'undefined') { window.wDiffNoChange     = '<hr style="height: 2px; margin: 1em 20%;">'; }
 
// compatibility fix for old name of main function
window.StringDiff = window.WDiffString;
 
 
// WDiffString: main program
// input: oldText, newText, strings containing the texts
// returns: html diff
 
window.WDiffString = function(oldText, newText) {
 
// IE / Mac fix
	oldText = oldText.replace(/(\r\n)/g, '\n');
	newText = newText.replace(/(\r\n)/g, '\n');
 
	var text = {};
	text.newWords = [];
	text.oldWords = [];
	text.newToOld = [];
	text.oldToNew = [];
	text.message = '';
	var block = {};
	var outText = '';
 
// trap trivial changes: no change
	if (oldText == newText) {
		outText = newText;
		outText = WDiffEscape(outText);
		outText = WDiffHtmlFormat(outText);
		return(outText);
	}
 
// trap trivial changes: old text deleted
	if ( (oldText == null) || (oldText.length == 0) ) {
		outText = newText;
		outText = WDiffEscape(outText);
		outText = WDiffHtmlFormat(outText);
		outText = wDiffHtmlInsertStart + outText + wDiffHtmlInsertEnd;
		return(outText);
	}
 
// trap trivial changes: new text deleted
	if ( (newText == null) || (newText.length == 0) ) {
		outText = oldText;
		outText = WDiffEscape(outText);
		outText = WDiffHtmlFormat(outText);
		outText = wDiffHtmlDeleteStart + outText + wDiffHtmlDeleteEnd;
		return(outText);
	}
 
// split new and old text into words
	WDiffSplitText(oldText, newText, text);
 
// calculate diff information
	WDiffText(text);
 
//detect block borders and moved blocks
	WDiffDetectBlocks(text, block);
 
// process diff data into formatted html text
	outText = WDiffToHtml(text, block);
 
// IE fix
	outText = outText.replace(/> ( *)</g, '>&nbsp;$1<');
 
	return(outText);
}
 
 
// WDiffSplitText: split new and old text into words
// input: oldText, newText, strings containing the texts
// changes: text.newWords and text.oldWords, arrays containing the texts in arrays of words
 
window.WDiffSplitText = function(oldText, newText, text) {
 
// convert strange spaces
	oldText = oldText.replace(/[\t\u000b\u00a0\u2028\u2029]+/g, ' ');
	newText = newText.replace(/[\t\u000b\u00a0\u2028\u2029]+/g, ' ');
 
// split old text into words
 
//              /     |    |    |    |    |   |  |     |   |  |  |    |    |    | /
	var pattern = /[\w]+|\[\[|\]\]|\{\{|\}\}|\n+| +|&\w+;|'''|''|=+|\{\||\|\}|\|\-|./g;
	var result;
	do {
		result = pattern.exec(oldText);
		if (result != null) {
			text.oldWords.push(result[0]);
		}
	} while (result != null);
 
// split new text into words
	do {
		result = pattern.exec(newText);
		if (result != null) {
			text.newWords.push(result[0]);
		}
	} while (result != null);
 
	return;
}
 
 
// WDiffText: calculate diff information
// input: text.newWords and text.oldWords, arrays containing the texts in arrays of words
// optionally for recursive calls: newStart, newEnd, oldStart, oldEnd, recursionLevel
// changes: text.newToOld and text.oldToNew, containing the line numbers in the other version
 
window.WDiffText = function(text, newStart, newEnd, oldStart, oldEnd, recursionLevel) {
 
	symbol = new Object();
	symbol.newCtr = [];
	symbol.oldCtr = [];
	symbol.toNew = [];
	symbol.toOld = [];
 
// set defaults
	newStart = newStart || 0;
	newEnd = newEnd || text.newWords.length;
	oldStart = oldStart || 0;
	oldEnd = oldEnd || text.oldWords.length;
	recursionLevel = recursionLevel || 0;
 
// limit recursion depth
	if (recursionLevel > 10) {
		return;
	}
 
// pass 1: parse new text into symbol table s
 
	var word;
	for (var i = newStart; i < newEnd; i ++) {
		word = text.newWords[i];
 
// add new entry to symbol table
		if ( symbol[word] == null) {
			symbol[word] = { newCtr: 0, oldCtr: 0, toNew: null, toOld: null };
		}
 
// increment symbol table word counter for new text
		symbol[word].newCtr ++;
 
// add last word number in new text
		symbol[word].toNew = i;
	}
 
// pass 2: parse old text into symbol table
 
	for (var j = oldStart; j < oldEnd; j ++) {
		word = text.oldWords[j];
 
// add new entry to symbol table
		if ( symbol[word] == null) {
			symbol[word] = { newCtr: 0, oldCtr: 0, toNew: null, toOld: null };
		}
 
// increment symbol table word counter for old text
		symbol[word].oldCtr ++;
 
// add last word number in old text
		symbol[word].toOld = j;
	}
 
// pass 3: connect unique words
 
	for (var i in symbol) {
 
// find words in the symbol table that occur only once in both versions
		if ( (symbol[i].newCtr == 1) && (symbol[i].oldCtr == 1) ) {
			var toNew = symbol[i].toNew;
			var toOld = symbol[i].toOld;
 
// do not use spaces as unique markers
			if ( ! /\s/.test( text.newWords[toNew] ) ) {
 
// connect from new to old and from old to new
				text.newToOld[toNew] = toOld;
				text.oldToNew[toOld] = toNew;
			}
		}
	}
 
// pass 4: connect adjacent identical words downwards
 
	for (var i = newStart; i < newEnd - 1; i ++) {
 
// find already connected pairs
		if (text.newToOld[i] != null) {
			j = text.newToOld[i];
 
// check if the following words are not yet connected
			if ( (text.newToOld[i + 1] == null) && (text.oldToNew[j + 1] == null) ) {
 
// if the following words are the same connect them
				if ( text.newWords[i + 1] == text.oldWords[j + 1] ) {
					text.newToOld[i + 1] = j + 1;
					text.oldToNew[j + 1] = i + 1;
				}
			}
		}
	}
 
// pass 5: connect adjacent identical words upwards
 
	for (var i = newEnd - 1; i > newStart; i --) {
 
// find already connected pairs
		if (text.newToOld[i] != null) {
			j = text.newToOld[i];
 
// check if the preceeding words are not yet connected
			if ( (text.newToOld[i - 1] == null) && (text.oldToNew[j - 1] == null) ) {
 
// if the preceeding words are the same connect them
				if ( text.newWords[i - 1] == text.oldWords[j - 1] ) {
					text.newToOld[i - 1] = j - 1;
					text.oldToNew[j - 1] = i - 1;
				}
			}
		}
	}
 
// recursively diff still unresolved regions downwards
 
	if (wDiffRecursiveDiff) {
		i = newStart;
		j = oldStart;
		while (i < newEnd) {
			if (text.newToOld[i - 1] != null) {
				j = text.newToOld[i - 1] + 1;
			}
 
// check for the start of an unresolved sequence
			if ( (text.newToOld[i] == null) && (text.oldToNew[j] == null) ) {
 
// determine the ends of the sequences
				var iStart = i;
				var iEnd = i;
				while ( (text.newToOld[iEnd] == null) && (iEnd < newEnd) ) {
					iEnd ++;
				}
				var iLength = iEnd - iStart;
 
				var jStart = j;
				var jEnd = j;
				while ( (text.oldToNew[jEnd] == null) && (jEnd < oldEnd) ) {
					jEnd ++;
				}
				var jLength = jEnd - jStart;
 
// recursively diff the unresolved sequence
				if ( (iLength > 0) && (jLength > 0) ) {
					if ( (iLength > 1) || (jLength > 1) ) {
						if ( (iStart != newStart) || (iEnd != newEnd) || (jStart != oldStart) || (jEnd != oldEnd) ) {
							WDiffText(text, iStart, iEnd, jStart, jEnd, recursionLevel + 1);
						}
					}
				}
				i = iEnd;
			}
			else {
				i ++;
			}
		}
	}
 
// recursively diff still unresolved regions upwards
 
	if (wDiffRecursiveDiff) {
		i = newEnd - 1;
		j = oldEnd - 1;
		while (i >= newStart) {
			if (text.newToOld[i + 1] != null) {
				j = text.newToOld[i + 1] - 1;
			}
 
// check for the start of an unresolved sequence
			if ( (text.newToOld[i] == null) && (text.oldToNew[j] == null) ) {
 
// determine the ends of the sequences
				var iStart = i;
				var iEnd = i + 1;
				while ( (text.newToOld[iStart - 1] == null) && (iStart >= newStart) ) {
					iStart --;
				}
				var iLength = iEnd - iStart;
 
				var jStart = j;
				var jEnd = j + 1;
				while ( (text.oldToNew[jStart - 1] == null) && (jStart >= oldStart) ) {
					jStart --;
				}
				var jLength = jEnd - jStart;
 
// recursively diff the unresolved sequence
				if ( (iLength > 0) && (jLength > 0) ) {
					if ( (iLength > 1) || (jLength > 1) ) {
						if ( (iStart != newStart) || (iEnd != newEnd) || (jStart != oldStart) || (jEnd != oldEnd) ) {
							WDiffText(text, iStart, iEnd, jStart, jEnd, recursionLevel + 1);
						}
					}
				}
				i = iStart - 1;
			}
			else {
				i --;
			}
		}
	}
	return;
}
 
 
// WDiffToHtml: process diff data into formatted html text
// input: text.newWords and text.oldWords, arrays containing the texts in arrays of words
//   text.newToOld and text.oldToNew, containing the line numbers in the other version
//   block data structure
// returns: outText, a html string
 
window.WDiffToHtml = function(text, block) {
 
	var outText = text.message;
 
	var blockNumber = 0;
	var i = 0;
	var j = 0;
	var movedAsInsertion;
 
// cycle through the new text
	do {
		var movedIndex = [];
		var movedBlock = [];
		var movedLeft = [];
		var blockText = '';
		var identText = '';
		var delText = '';
		var insText = '';
		var identStart = '';
 
// check if a block ends here and finish previous block
		if (movedAsInsertion != null) {
			if (movedAsInsertion == false) {
				identStart += wDiffHtmlBlockEnd;
			}
			else {
				identStart += wDiffHtmlInsertEnd;
			}
			movedAsInsertion = null;
		}
 
// detect block boundary
		if ( (text.newToOld[i] != j) || (blockNumber == 0 ) ) {
			if ( ( (text.newToOld[i] != null) || (i >= text.newWords.length) ) && ( (text.oldToNew[j] != null) || (j >= text.oldWords.length) ) ) {
 
// block moved right
				var moved = block.newRight[blockNumber];
				if (moved > 0) {
					var index = block.newRightIndex[blockNumber];
					movedIndex.push(index);
					movedBlock.push(moved);
					movedLeft.push(false);
				}
 
// block moved left
				moved = block.newLeft[blockNumber];
				if (moved > 0) {
					var index = block.newLeftIndex[blockNumber];
					movedIndex.push(index);
					movedBlock.push(moved);
					movedLeft.push(true);
				}
 
// check if a block starts here
				moved = block.newBlock[blockNumber];
				if (moved > 0) {
 
// mark block as inserted text
					if (block.newWords[blockNumber] < wDiffBlockMinLength) {
						identStart += wDiffHtmlInsertStart;
						movedAsInsertion = true;
					}
 
// mark block by color
					else {
						if (moved > wDiffStyleBlock.length) {
							moved = wDiffStyleBlock.length;
						}
						identStart += WDiffHtmlCustomize(wDiffHtmlBlockStart, moved - 1);
						movedAsInsertion = false;
					}
				}
 
				if (i >= text.newWords.length) {
					i ++;
				}
				else {
					j = text.newToOld[i];
					blockNumber ++;
				}
			}
		}
 
// get the correct order if moved to the left as well as to the right from here
		if (movedIndex.length == 2) {
			if (movedIndex[0] > movedIndex[1]) {
				movedIndex.reverse();
				movedBlock.reverse();
				movedLeft.reverse();
			}
		}
 
// handle left and right block moves from this position
		for (var m = 0; m < movedIndex.length; m ++) {
 
// insert the block as deleted text
			if (block.newWords[ movedIndex[m] ] < wDiffBlockMinLength) {
				var movedStart = block.newStart[ movedIndex[m] ];
				var movedLength = block.newLength[ movedIndex[m] ];
				var str = '';
				for (var n = movedStart; n < movedStart + movedLength; n ++) {
					str += text.newWords[n];
				}
				str = WDiffEscape(str);
				str = str.replace(/\n/g, '&para;<br>');
				blockText += wDiffHtmlDeleteStart + str + wDiffHtmlDeleteEnd;
			}
 
// add a placeholder / move direction indicator
			else {
				if (movedBlock[m] > wDiffStyleBlock.length) {
					movedBlock[m] = wDiffStyleBlock.length;
				}
				if (movedLeft[m]) {
					blockText += WDiffHtmlCustomize(wDiffHtmlMovedLeft, movedBlock[m] - 1);
				}
				else {
					blockText += WDiffHtmlCustomize(wDiffHtmlMovedRight, movedBlock[m] - 1);
				}
			}
		}
 
// collect consecutive identical text
		while ( (i < text.newWords.length) && (j < text.oldWords.length) ) {
			if ( (text.newToOld[i] == null) || (text.oldToNew[j] == null) ) {
				break;
			}
			if (text.newToOld[i] != j) {
				break;
			}
			identText += text.newWords[i];
			i ++;
			j ++;
		}
 
// collect consecutive deletions
		while ( (text.oldToNew[j] == null) && (j < text.oldWords.length) ) {
			delText += text.oldWords[j];
			j ++;
		}
 
// collect consecutive inserts
		while ( (text.newToOld[i] == null) && (i < text.newWords.length) ) {
			insText += text.newWords[i];
			i ++;
		}
 
// remove leading and trailing similarities betweein delText and ins from highlighting
		var preText = '';
		var postText = '';
		if (wDiffWordDiff) {
			if ( (delText != '') && (insText != '') ) {
 
// remove leading similarities
				while ( delText.charAt(0) == insText.charAt(0) && (delText != '') && (insText != '') ) {
					preText = preText + delText.charAt(0);
					delText = delText.substr(1);
					insText = insText.substr(1);
				}
 
// remove trailing similarities
				while ( delText.charAt(delText.length - 1) == insText.charAt(insText.length - 1) && (delText != '') && (insText != '') ) {
					postText = delText.charAt(delText.length - 1) + postText;
					delText = delText.substr(0, delText.length - 1);
					insText = insText.substr(0, insText.length - 1);
				}
			}
		}
 
// output the identical text, deletions and inserts
 
// moved from here indicator
		if (blockText != '') {
			outText += blockText;
		}
 
// identical text
		if (identText != '') {
			outText += identStart + WDiffEscape(identText);
		}
		outText += preText;
 
// deleted text
		if (delText != '') {
			delText = wDiffHtmlDeleteStart + WDiffEscape(delText) + wDiffHtmlDeleteEnd;
			delText = delText.replace(/\n/g, '&para;<br>');
			outText += delText;
		}
 
// inserted text
		if (insText != '') {
			insText = wDiffHtmlInsertStart + WDiffEscape(insText) + wDiffHtmlInsertEnd;
			insText = insText.replace(/\n/g, '&para;<br>');
			outText += insText;
		}
		outText += postText;
	} while (i <= text.newWords.length);
 
	outText += '\n';
	outText = WDiffHtmlFormat(outText);
 
	return(outText);
}
 
 
// WDiffEscape: replaces html-sensitive characters in output text with character entities
 
window.WDiffEscape = function(text) {
 
	text = text.replace(/&/g, '&amp;');
	text = text.replace(/</g, '&lt;');
	text = text.replace(/>/g, '&gt;');
	text = text.replace(/\"/g, '&quot;');
 
	return(text);
}
 
 
// HtmlCustomize: customize indicator html: replace {number} with the block number, {block} with the block style
 
window.WDiffHtmlCustomize = function(text, block) {
 
	text = text.replace(/\{number\}/, block);
	text = text.replace(/\{block\}/, wDiffStyleBlock[block]);
 
	return(text);
}
 
 
// HtmlFormat: replaces newlines and multiple spaces in text with html code
 
window.WDiffHtmlFormat = function(text) {
 
	text = text.replace(/  /g, ' &nbsp;');
	text = text.replace(/\n/g, '<br>');
 
	return(text);
}
 
 
// WDiffDetectBlocks: detect block borders and moved blocks
// input: text object, block object
 
window.WDiffDetectBlocks = function(text, block) {
 
	block.oldStart  = [];
	block.oldToNew  = [];
	block.oldLength = [];
	block.oldWords  = [];
	block.newStart  = [];
	block.newLength = [];
	block.newWords  = [];
	block.newNumber = [];
	block.newBlock  = [];
	block.newLeft   = [];
	block.newRight  = [];
	block.newLeftIndex  = [];
	block.newRightIndex = [];
 
	var blockNumber = 0;
	var wordCounter = 0;
	var realWordCounter = 0;
 
// get old text block order
	if (wDiffShowBlockMoves) {
		var j = 0;
		var i = 0;
		do {
 
// detect block boundaries on old text
			if ( (text.oldToNew[j] != i) || (blockNumber == 0 ) ) {
				if ( ( (text.oldToNew[j] != null) || (j >= text.oldWords.length) ) && ( (text.newToOld[i] != null) || (i >= text.newWords.length) ) ) {
					if (blockNumber > 0) {
						block.oldLength[blockNumber - 1] = wordCounter;
						block.oldWords[blockNumber - 1] = realWordCounter;
						wordCounter = 0;
						realWordCounter = 0;
					}
 
					if (j >= text.oldWords.length) {
						j ++;
					}
					else {
						i = text.oldToNew[j];
						block.oldStart[blockNumber] = j;
						block.oldToNew[blockNumber] = text.oldToNew[j];
						blockNumber ++;
					}
				}
			}
 
// jump over identical pairs
			while ( (i < text.newWords.length) && (j < text.oldWords.length) ) {
				if ( (text.newToOld[i] == null) || (text.oldToNew[j] == null) ) {
					break;
				}
				if (text.oldToNew[j] != i) {
					break;
				}
				i ++;
				j ++;
				wordCounter ++;
				if ( /\w/.test( text.newWords[i] ) ) {
					realWordCounter ++;
				}
			}
 
// jump over consecutive deletions
			while ( (text.oldToNew[j] == null) && (j < text.oldWords.length) ) {
				j ++;
			}
 
// jump over consecutive inserts
			while ( (text.newToOld[i] == null) && (i < text.newWords.length) ) {
				i ++;
			}
		} while (j <= text.oldWords.length);
 
// get the block order in the new text
		var lastMin;
		var currMinIndex;
		lastMin = null;
 
// sort the data by increasing start numbers into new text block info
		for (var i = 0; i < blockNumber; i ++) {
			currMin = null;
			for (var j = 0; j < blockNumber; j ++) {
				curr = block.oldToNew[j];
				if ( (curr > lastMin) || (lastMin == null) ) {
					if ( (curr < currMin) || (currMin == null) ) {
						currMin = curr;
						currMinIndex = j;
					}
				}
			}
			block.newStart[i] = block.oldToNew[currMinIndex];
			block.newLength[i] = block.oldLength[currMinIndex];
			block.newWords[i] = block.oldWords[currMinIndex];
			block.newNumber[i] = currMinIndex;
			lastMin = currMin;
		}
 
// detect not moved blocks
		for (var i = 0; i < blockNumber; i ++) {
			if (block.newBlock[i] == null) {
				if (block.newNumber[i] == i) {
					block.newBlock[i] = 0;
				}
			}
		}
 
// detect switches of neighbouring blocks
		for (var i = 0; i < blockNumber - 1; i ++) {
			if ( (block.newBlock[i] == null) && (block.newBlock[i + 1] == null) ) {
				if (block.newNumber[i] - block.newNumber[i + 1] == 1) {
					if ( (block.newNumber[i + 1] - block.newNumber[i + 2] != 1) || (i + 2 >= blockNumber) ) {
 
// the shorter one is declared the moved one
						if (block.newLength[i] < block.newLength[i + 1]) {
							block.newBlock[i] = 1;
							block.newBlock[i + 1] = 0;
						}
						else {
							block.newBlock[i] = 0;
							block.newBlock[i + 1] = 1;
						}
					}
				}
			}
		}
 
// mark all others as moved and number the moved blocks
		j = 1;
		for (var i = 0; i < blockNumber; i ++) {
			if ( (block.newBlock[i] == null) || (block.newBlock[i] == 1) ) {
				block.newBlock[i] = j++;
			}
		}
 
// check if a block has been moved from this block border
		for (var i = 0; i < blockNumber; i ++) {
			for (var j = 0; j < blockNumber; j ++) {
 
				if (block.newNumber[j] == i) {
					if (block.newBlock[j] > 0) {
 
// block moved right
						if (block.newNumber[j] < j) {
							block.newRight[i] = block.newBlock[j];
							block.newRightIndex[i] = j;
						}
 
// block moved left
						else {
							block.newLeft[i + 1] = block.newBlock[j];
							block.newLeftIndex[i + 1] = j;
						}
					}
				}
			}
		}
	}
	return;
}
 
 
// WDiffShortenOutput: remove unchanged parts from final output
// input: the output of WDiffString
// returns: the text with removed unchanged passages indicated by (...)
 
window.WDiffShortenOutput = function(diffText) {
 
// html <br/> to newlines
	diffText = diffText.replace(/<br[^>]*>/g, '\n');
 
// scan for diff html tags
	var regExpDiff = new RegExp('<\\w+ class=\\"(\\w+)\\"[^>]*>(.|\\n)*?<!--\\1-->', 'g');
	var tagStart = [];
	var tagEnd = [];
	var i = 0;
	var found;
	while ( (found = regExpDiff.exec(diffText)) != null ) {
 
// combine consecutive diff tags
		if ( (i > 0) && (tagEnd[i - 1] == found.index) ) {
			tagEnd[i - 1] = found.index + found[0].length;
		}
		else {
			tagStart[i] = found.index;
			tagEnd[i] = found.index + found[0].length;
			i ++;
		}
	}
 
// no diff tags detected
	if (tagStart.length == 0) {
		return(wDiffNoChange);
	}
 
// define regexps
	var regExpHeading = new RegExp('\\n=+.+?=+ *\\n|\\n\\{\\||\\n\\|\\}', 'g');
	var regExpParagraph = new RegExp('\\n\\n+', 'g');
	var regExpLine = new RegExp('\\n+', 'g');
	var regExpBlank = new RegExp('(<[^>]+>)*\\s+', 'g');
 
// determine fragment border positions around diff tags
	var rangeStart = [];
	var rangeEnd = [];
	var rangeStartType = [];
	var rangeEndType = [];
	for (var i = 0; i < tagStart.length; i ++) {
		var found;
 
// find last heading before diff tag
		var lastPos = tagStart[i] - wDiffHeadingBefore;
		if (lastPos < 0) {
			lastPos = 0;
		}
		regExpHeading.lastIndex = lastPos;
		while ( (found = regExpHeading.exec(diffText)) != null ) {
			if (found.index > tagStart[i]) {
				break;
			}
			rangeStart[i] = found.index;
			rangeStartType[i] = 'heading';
		}
 
// find last paragraph before diff tag
		if (rangeStart[i] == null) {
			lastPos = tagStart[i] - wDiffParagraphBefore;
			if (lastPos < 0) {
				lastPos = 0;
			}
			regExpParagraph.lastIndex = lastPos;
			while ( (found = regExpParagraph.exec(diffText)) != null ) {
				if (found.index > tagStart[i]) {
					break;
				}
				rangeStart[i] = found.index;
				rangeStartType[i] = 'paragraph';
			}
		}
 
// find line break before diff tag
		if (rangeStart[i] == null) {
			lastPos = tagStart[i] - wDiffLineBeforeMax;
			if (lastPos < 0) {
				lastPos = 0;
			}
			regExpLine.lastIndex = lastPos;
			while ( (found = regExpLine.exec(diffText)) != null ) {
				if (found.index > tagStart[i] - wDiffLineBeforeMin) {
					break;
				}
				rangeStart[i] = found.index;
				rangeStartType[i] = 'line';
			}
		}
 
// find blank before diff tag
		if (rangeStart[i] == null) {
			lastPos = tagStart[i] - wDiffBlankBeforeMax;
			if (lastPos < 0) {
				lastPos = 0;
			}
			regExpBlank.lastIndex = lastPos;
			while ( (found = regExpBlank.exec(diffText)) != null ) {
				if (found.index > tagStart[i] - wDiffBlankBeforeMin) {
					break;
				}
				rangeStart[i] = found.index;
				rangeStartType[i] = 'blank';
			}
		}
 
// fixed number of chars before diff tag
		if (rangeStart[i] == null) {
			rangeStart[i] = tagStart[i] - wDiffCharsBefore;
			rangeStartType[i] = 'chars';
			if (rangeStart[i] < 0) {
				rangeStart[i] = 0;
			}
		}
 
// find first heading after diff tag
		regExpHeading.lastIndex = tagEnd[i];
		if ( (found = regExpHeading.exec(diffText)) != null ) {
			if (found.index < tagEnd[i] + wDiffHeadingAfter) {
				rangeEnd[i] = found.index + found[0].length;
				rangeEndType[i] = 'heading';
			}
		}
 
// find first paragraph after diff tag
		if (rangeEnd[i] == null) {
			regExpParagraph.lastIndex = tagEnd[i];
			if ( (found = regExpParagraph.exec(diffText)) != null ) {
				if (found.index < tagEnd[i] + wDiffParagraphAfter) {
					rangeEnd[i] = found.index;
					rangeEndType[i] = 'paragraph';
				}
			}
		}
 
// find first line break after diff tag
		if (rangeEnd[i] == null) {
			regExpLine.lastIndex = tagEnd[i] + wDiffLineAfterMin;
			if ( (found = regExpLine.exec(diffText)) != null ) {
				if (found.index < tagEnd[i] + wDiffLineAfterMax) {
					rangeEnd[i] = found.index;
					rangeEndType[i] = 'break';
				}
			}
		}
 
// find blank after diff tag
		if (rangeEnd[i] == null) {
			regExpBlank.lastIndex = tagEnd[i] + wDiffBlankAfterMin;
			if ( (found = regExpBlank.exec(diffText)) != null ) {
				if (found.index < tagEnd[i] + wDiffBlankAfterMax) {
					rangeEnd[i] = found.index;
					rangeEndType[i] = 'blank';
				}
			}
		}
 
// fixed number of chars after diff tag
		if (rangeEnd[i] == null) {
			rangeEnd[i] = tagEnd[i] + wDiffCharsAfter;
			if (rangeEnd[i] > diffText.length) {
				rangeEnd[i] = diffText.length;
				rangeEndType[i] = 'chars';
			}
		}
	}
 
// remove overlaps, join close fragments
	var fragmentStart = [];
	var fragmentEnd = [];
	var fragmentStartType = [];
	var fragmentEndType = [];
	fragmentStart[0] = rangeStart[0];
	fragmentEnd[0] = rangeEnd[0];
	fragmentStartType[0] = rangeStartType[0];
	fragmentEndType[0] = rangeEndType[0];
	var j = 1;
	for (var i = 1; i < rangeStart.length; i ++) {
		if (rangeStart[i] > fragmentEnd[j - 1] + wDiffFragmentJoin) {
			fragmentStart[j] = rangeStart[i];
			fragmentEnd[j] = rangeEnd[i];
			fragmentStartType[j] = rangeStartType[i];
			fragmentEndType[j] = rangeEndType[i];
			j ++;
		}
		else {
			fragmentEnd[j - 1] = rangeEnd[i];
			fragmentEndType[j - 1] = rangeEndType[i];
		}
	}
 
// assemble the fragments
	var outText = '';
	for (var i = 0; i < fragmentStart.length; i ++) {
 
// get text fragment
		var fragment = diffText.substring(fragmentStart[i], fragmentEnd[i]);
		var fragment = fragment.replace(/^\n+|\n+$/g, '');
 
// add inline marks for omitted chars and words
		if (fragmentStart[i] > 0) {
			if (fragmentStartType[i] == 'chars') {
				fragment = wDiffOmittedChars + fragment;
			}
			else if (fragmentStartType[i] == 'blank') {
				fragment = wDiffOmittedChars + ' ' + fragment;
			}
		}
		if (fragmentEnd[i] < diffText.length) {
			if (fragmentStartType[i] == 'chars') {
				fragment = fragment + wDiffOmittedChars;
			}
			else if (fragmentStartType[i] == 'blank') {
				fragment = fragment + ' ' + wDiffOmittedChars;
			}
		}
 
// add omitted line separator
		if (fragmentStart[i] > 0) {
			outText += wDiffOmittedLines;
		}
 
// encapsulate span errors
		outText += '<div>' + fragment + '</div>';
	}
 
// add trailing omitted line separator
	if (fragmentEnd[i - 1] < diffText.length) {
		outText = outText + wDiffOmittedLines;
	}
 
// remove leading and trailing empty lines
	outText = outText.replace(/^(<div>)\n+|\n+(<\/div>)$/g, '$1$2');
 
// convert to html linebreaks
	outText = outText.replace(/\n/g, '<br />');
 
	return(outText);
}
 
