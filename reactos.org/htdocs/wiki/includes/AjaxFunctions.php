<?php
/**
 * @file
 * @ingroup Ajax
 */

if( !defined( 'MEDIAWIKI' ) ) {
	die( 1 );
}

/**
 * Function converts an Javascript escaped string back into a string with
 * specified charset (default is UTF-8).
 * Modified function from http://pure-essence.net/stuff/code/utf8RawUrlDecode.phps
 *
 * @param $source String escaped with Javascript's escape() function
 * @param $iconv_to String destination character set will be used as second paramether in the iconv function. Default is UTF-8.
 * @return string
 */
function js_unescape($source, $iconv_to = 'UTF-8') {
	$decodedStr = '';
	$pos = 0;
	$len = strlen ($source);

	while ($pos < $len) {
		$charAt = substr ($source, $pos, 1);
		if ($charAt == '%') {
			$pos++;
			$charAt = substr ($source, $pos, 1);
			if ($charAt == 'u') {
				// we got a unicode character
				$pos++;
				$unicodeHexVal = substr ($source, $pos, 4);
				$unicode = hexdec ($unicodeHexVal);
				$decodedStr .= code2utf($unicode);
				$pos += 4;
			} else {
				// we have an escaped ascii character
				$hexVal = substr ($source, $pos, 2);
				$decodedStr .= chr (hexdec ($hexVal));
				$pos += 2;
			}
		} else {
			$decodedStr .= $charAt;
			$pos++;
		}
	}

	if ($iconv_to != "UTF-8") {
		$decodedStr = iconv("UTF-8", $iconv_to, $decodedStr);
	}

	return $decodedStr;
}

/**
 * Function coverts number of utf char into that character.
 * Function taken from: http://www.php.net/manual/en/function.utf8-encode.php#49336
 *
 * @param $num Integer
 * @return utf8char
 */
function code2utf($num){
   if ( $num<128 )
   	return chr($num);
   if ( $num<2048 )
   	return chr(($num>>6)+192).chr(($num&63)+128);
   if ( $num<65536 )
   	return chr(($num>>12)+224).chr((($num>>6)&63)+128).chr(($num&63)+128);
   if ( $num<2097152 )
   	return chr(($num>>18)+240).chr((($num>>12)&63)+128).chr((($num>>6)&63)+128) .chr(($num&63)+128);
   return '';
}

define( 'AJAX_SEARCH_VERSION', 2 );	//AJAX search cache version

function wfSajaxSearch( $term ) {
	global $wgContLang, $wgUser, $wgCapitalLinks, $wgMemc;
	$limit = 16;
	$sk = $wgUser->getSkin();
	$output = '';

	$term = trim( $term );
	$term = $wgContLang->checkTitleEncoding( $wgContLang->recodeInput( js_unescape( $term ) ) );
	if ( $wgCapitalLinks )
		$term = $wgContLang->ucfirst( $term );
	$term_title = Title::newFromText( $term );

	$memckey = $term_title ? wfMemcKey( 'ajaxsearch', md5( $term_title->getFullText() ) ) : wfMemcKey( 'ajaxsearch', md5( $term ) );
	$cached = $wgMemc->get($memckey);
	if( is_array( $cached ) && $cached['version'] == AJAX_SEARCH_VERSION ) {
		$response = new AjaxResponse( $cached['html'] );
		$response->setCacheDuration( 30*60 );
		return $response;
	}

	$r = $more = '';
	$canSearch = true;

	$results = PrefixSearch::titleSearch( $term, $limit + 1 );
	foreach( array_slice( $results, 0, $limit ) as $titleText ) {
		$r .= '<li>' . $sk->makeKnownLink( $titleText ) . "</li>\n";
	}

	// Hack to check for specials
	if( $results ) {
		$t = Title::newFromText( $results[0] );
		if( $t && $t->getNamespace() == NS_SPECIAL ) {
			$canSearch = false;
			if( count( $results ) > $limit ) {
				$more = '<i>' .
					$sk->makeKnownLinkObj(
						SpecialPage::getTitleFor( 'Specialpages' ),
						wfMsgHtml( 'moredotdotdot' ) ) .
					'</i>';
			}
		} else {
			if( count( $results ) > $limit ) {
				$more = '<i>' .
					$sk->makeKnownLinkObj(
						SpecialPage::getTitleFor( "Allpages", $term ),
						wfMsgHtml( 'moredotdotdot' ) ) .
					'</i>';
			}
		}
	}

	$valid = (bool) $term_title;
	$term_url = urlencode( $term );
	$term_normalized = $valid ? $term_title->getFullText() : $term;
	$term_display = htmlspecialchars( $term );
	$subtitlemsg = ( $valid ? 'searchsubtitle' : 'searchsubtitleinvalid' );
	$subtitle = wfMsgExt( $subtitlemsg, array( 'parse' ), wfEscapeWikiText( $term_normalized ) );
	$html = '<div id="searchTargetHide"><a onclick="Searching_Hide_Results();">'
		. wfMsgHtml( 'hideresults' ) . '</a></div>'
		. '<h1 class="firstHeading">'.wfMsgHtml('search')
		. '</h1><div id="contentSub">'. $subtitle . '</div>';
	if( $canSearch ) {
		$html .= '<ul><li>'
			. $sk->makeKnownLink( $wgContLang->specialPage( 'Search' ),
						wfMsgHtml( 'searchcontaining', $term_display ),
						"search={$term_url}&fulltext=Search" )
			. '</li><li>' . $sk->makeKnownLink( $wgContLang->specialPage( 'Search' ),
						wfMsgHtml( 'searchnamed', $term_display ) ,
						"search={$term_url}&go=Go" )
			. "</li></ul>";
	}
	if( $r ) {
		$html .= "<h2>" . wfMsgHtml( 'articletitles', $term_display ) . "</h2>"
			. '<ul>' .$r .'</ul>' . $more;
	}

	$wgMemc->set( $memckey, array( 'version' => AJAX_SEARCH_VERSION, 'html' => $html ), 30 * 60 );

	$response = new AjaxResponse( $html );
	$response->setCacheDuration( 30*60 );
	return $response;
}

/**
 * Called for AJAX watch/unwatch requests.
 * @param $pagename Prefixed title string for page to watch/unwatch
 * @param $watch String 'w' to watch, 'u' to unwatch
 * @return String '<w#>' or '<u#>' on successful watch or unwatch,
 *   respectively, followed by an HTML message to display in the alert box; or
 *   '<err#>' on error
 */
function wfAjaxWatch($pagename = "", $watch = "") {
	if(wfReadOnly()) {
		// redirect to action=(un)watch, which will display the database lock
		// message
		return '<err#>';
	}

	if('w' !== $watch && 'u' !== $watch) {
		return '<err#>';
	}
	$watch = 'w' === $watch;

	$title = Title::newFromDBkey($pagename);
	if(!$title) {
		// Invalid title
		return '<err#>';
	}
	$article = new Article($title);
	$watching = $title->userIsWatching();

	if($watch) {
		if(!$watching) {
			$dbw = wfGetDB(DB_MASTER);
			$dbw->begin();
			$article->doWatch();
			$dbw->commit();
		}
	} else {
		if($watching) {
			$dbw = wfGetDB(DB_MASTER);
			$dbw->begin();
			$article->doUnwatch();
			$dbw->commit();
		}
	}
	if( $watch ) {
		return '<w#>'.wfMsgExt( 'addedwatchtext', array( 'parse' ), $title->getPrefixedText() );
	} else {
		return '<u#>'.wfMsgExt( 'removedwatchtext', array( 'parse' ), $title->getPrefixedText() );
	}
}
