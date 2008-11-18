<?php
# Copyright (C) 2004 Brion Vibber <brion@pobox.com>
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

/**
 * Run text & title search and display the output
 * @file
 * @ingroup SpecialPage
 */

/**
 * Entry point
 *
 * @param $par String: (default '')
 */
function wfSpecialSearch( $par = '' ) {
	global $wgRequest, $wgUser;

	$search = str_replace( "\n", " ", $wgRequest->getText( 'search', $par ) );
	$searchPage = new SpecialSearch( $wgRequest, $wgUser );
	if( $wgRequest->getVal( 'fulltext' ) 
		|| !is_null( $wgRequest->getVal( 'offset' )) 
		|| !is_null( $wgRequest->getVal( 'searchx' ))) {
		$searchPage->showResults( $search, 'search' );
	} else {
		$searchPage->goResult( $search );
	}
}

/**
 * implements Special:Search - Run text & title search and display the output
 * @ingroup SpecialPage
 */
class SpecialSearch {

	/**
	 * Set up basic search parameters from the request and user settings.
	 * Typically you'll pass $wgRequest and $wgUser.
	 *
	 * @param WebRequest $request
	 * @param User $user
	 * @public
	 */
	function SpecialSearch( &$request, &$user ) {
		list( $this->limit, $this->offset ) = $request->getLimitOffset( 20, 'searchlimit' );

		$this->namespaces = $this->powerSearch( $request );
		if( empty( $this->namespaces ) ) {
			$this->namespaces = SearchEngine::userNamespaces( $user );
		}

		$this->searchRedirects = $request->getcheck( 'redirs' ) ? true : false;
	}

	/**
	 * If an exact title match can be found, jump straight ahead to it.
	 * @param string $term
	 * @public
	 */
	function goResult( $term ) {
		global $wgOut;
		global $wgGoToEdit;

		$this->setupPage( $term );

		# Try to go to page as entered.
		$t = Title::newFromText( $term );

		# If the string cannot be used to create a title
		if( is_null( $t ) ){
			return $this->showResults( $term );
		}

		# If there's an exact or very near match, jump right there.
		$t = SearchEngine::getNearMatch( $term );
		if( !is_null( $t ) ) {
			$wgOut->redirect( $t->getFullURL() );
			return;
		}

		# No match, generate an edit URL
		$t = Title::newFromText( $term );
		if( ! is_null( $t ) ) {
			wfRunHooks( 'SpecialSearchNogomatch', array( &$t ) );
			# If the feature is enabled, go straight to the edit page
			if ( $wgGoToEdit ) {
				$wgOut->redirect( $t->getFullURL( 'action=edit' ) );
				return;
			}
		}

		$wgOut->wrapWikiMsg( "==$1==\n", 'notitlematches' );
		if( $t->quickUserCan( 'create' ) && $t->quickUserCan( 'edit' ) ) {
			$wgOut->addWikiMsg( 'noexactmatch', wfEscapeWikiText( $term ) );
		} else {
			$wgOut->addWikiMsg( 'noexactmatch-nocreate', wfEscapeWikiText( $term ) );
		}

		return $this->showResults( $term );
	}

	/**
	 * @param string $term
	 * @public
	 */
	function showResults( $term ) {
		$fname = 'SpecialSearch::showResults';
		wfProfileIn( $fname );
		global $wgOut, $wgUser;
		$sk = $wgUser->getSkin();

		$this->setupPage( $term );

		$wgOut->addWikiMsg( 'searchresulttext' );

		if( '' === trim( $term ) ) {
			// Empty query -- straight view of search form
			$wgOut->setSubtitle( '' );
			$wgOut->addHTML( $this->powerSearchBox( $term ) );
			$wgOut->addHTML( $this->powerSearchFocus() );
			wfProfileOut( $fname );
			return;
		}

		global $wgDisableTextSearch;
		if ( $wgDisableTextSearch ) {
			global $wgSearchForwardUrl;
			if( $wgSearchForwardUrl ) {
				$url = str_replace( '$1', urlencode( $term ), $wgSearchForwardUrl );
				$wgOut->redirect( $url );
				return;
			}
			global $wgInputEncoding;
			$wgOut->addHTML(
				Xml::openElement( 'fieldset' ) .
				Xml::element( 'legend', null, wfMsg( 'search-external' ) ) .
				Xml::element( 'p', array( 'class' => 'mw-searchdisabled' ), wfMsg( 'searchdisabled' ) ) .
				wfMsg( 'googlesearch',
					htmlspecialchars( $term ),
					htmlspecialchars( $wgInputEncoding ),
					htmlspecialchars( wfMsg( 'searchbutton' ) )
				) .
				Xml::closeElement( 'fieldset' )
			);
			wfProfileOut( $fname );
			return;
		}

		$wgOut->addHTML( $this->shortDialog( $term ) );

		$search = SearchEngine::create();
		$search->setLimitOffset( $this->limit, $this->offset );
		$search->setNamespaces( $this->namespaces );
		$search->showRedirects = $this->searchRedirects;
		$rewritten = $search->replacePrefixes($term);

		$titleMatches = $search->searchTitle( $rewritten );

		// Sometimes the search engine knows there are too many hits
		if ($titleMatches instanceof SearchResultTooMany) {
			$wgOut->addWikiText( '==' . wfMsg( 'toomanymatches' ) . "==\n" );
			$wgOut->addHTML( $this->powerSearchBox( $term ) );
			$wgOut->addHTML( $this->powerSearchFocus() );
			wfProfileOut( $fname );
			return;
		}
		
		$textMatches = $search->searchText( $rewritten );

		// did you mean... suggestions
		if($textMatches && $textMatches->hasSuggestion()){
			$st = SpecialPage::getTitleFor( 'Search' );			
			$stParams = wfArrayToCGI( array( 
					'search' 	=> $textMatches->getSuggestionQuery(), 
					'fulltext' 	=> wfMsg('search')),
					$this->powerSearchOptions());
					
			$suggestLink = '<a href="'.$st->escapeLocalURL($stParams).'">'.
					$textMatches->getSuggestionSnippet().'</a>';
			 		
			$wgOut->addHTML('<div class="searchdidyoumean">'.wfMsg('search-suggest',$suggestLink).'</div>');
		}

		// show number of results
		$num = ( $titleMatches ? $titleMatches->numRows() : 0 )
			+ ( $textMatches ? $textMatches->numRows() : 0);
		$totalNum = 0;
		if($titleMatches && !is_null($titleMatches->getTotalHits()))
			$totalNum += $titleMatches->getTotalHits();
		if($textMatches && !is_null($textMatches->getTotalHits()))
			$totalNum += $textMatches->getTotalHits();
		if ( $num > 0 ) {
			if ( $totalNum > 0 ){
				$top = wfMsgExt('showingresultstotal', array( 'parseinline' ), 
					$this->offset+1, $this->offset+$num, $totalNum );
			} elseif ( $num >= $this->limit ) {
				$top = wfShowingResults( $this->offset, $this->limit );
			} else {
				$top = wfShowingResultsNum( $this->offset, $this->limit, $num );
			}
			$wgOut->addHTML( "<p class='mw-search-numberresults'>{$top}</p>\n" );
		}

		// prev/next links
		if( $num || $this->offset ) {
			$prevnext = wfViewPrevNext( $this->offset, $this->limit,
				SpecialPage::getTitleFor( 'Search' ),
				wfArrayToCGI(
					$this->powerSearchOptions(),
					array( 'search' => $term ) ),
					($num < $this->limit) );
			$wgOut->addHTML( "<p class='mw-search-pager-top'>{$prevnext}</p>\n" );
			wfRunHooks( 'SpecialSearchResults', array( $term, &$titleMatches, &$textMatches ) );
		} else {
			wfRunHooks( 'SpecialSearchNoResults', array( $term ) );
		}

		if( $titleMatches ) {
			if( $titleMatches->numRows() ) {
				$wgOut->wrapWikiMsg( "==$1==\n", 'titlematches' );
				$wgOut->addHTML( $this->showMatches( $titleMatches ) );
			}
			$titleMatches->free();
		}

		if( $textMatches ) {
			// output appropriate heading
			if( $textMatches->numRows() ) {
				if($titleMatches)
					$wgOut->wrapWikiMsg( "==$1==\n", 'textmatches' );
				else // if no title matches the heading is redundant
					$wgOut->addHTML("<hr/>");								
			} elseif( $num == 0 ) {
				# Don't show the 'no text matches' if we received title matches
				$wgOut->wrapWikiMsg( "==$1==\n", 'notextmatches' );
			}
			// show interwiki results if any
			if( $textMatches->hasInterwikiResults() )
				$wgOut->addHtml( $this->showInterwiki( $textMatches->getInterwikiResults(), $term ));
			// show results
			if( $textMatches->numRows() )
				$wgOut->addHTML( $this->showMatches( $textMatches ) );

			$textMatches->free();
		}

		if ( $num == 0 ) {
			$wgOut->addWikiMsg( 'nonefound' );
		}
		if( $num || $this->offset ) {
			$wgOut->addHTML( "<p class='mw-search-pager-bottom'>{$prevnext}</p>\n" );
		}
		$wgOut->addHTML( $this->powerSearchBox( $term ) );
		wfProfileOut( $fname );
	}

	#------------------------------------------------------------------
	# Private methods below this line
	
	/**
	 *
	 */
	function setupPage( $term ) {
		global $wgOut;
		if( !empty( $term ) )
			$wgOut->setPageTitle( wfMsg( 'searchresults' ) );			
		$subtitlemsg = ( Title::newFromText( $term ) ? 'searchsubtitle' : 'searchsubtitleinvalid' );
		$wgOut->setSubtitle( $wgOut->parse( wfMsg( $subtitlemsg, wfEscapeWikiText($term) ) ) );
		$wgOut->setArticleRelated( false );
		$wgOut->setRobotpolicy( 'noindex,nofollow' );
	}

	/**
	 * Extract "power search" namespace settings from the request object,
	 * returning a list of index numbers to search.
	 *
	 * @param WebRequest $request
	 * @return array
	 * @private
	 */
	function powerSearch( &$request ) {
		$arr = array();
		foreach( SearchEngine::searchableNamespaces() as $ns => $name ) {
			if( $request->getCheck( 'ns' . $ns ) ) {
				$arr[] = $ns;
			}
		}
		return $arr;
	}

	/**
	 * Reconstruct the 'power search' options for links
	 * @return array
	 * @private
	 */
	function powerSearchOptions() {
		$opt = array();
		foreach( $this->namespaces as $n ) {
			$opt['ns' . $n] = 1;
		}
		$opt['redirs'] = $this->searchRedirects ? 1 : 0;
		return $opt;
	}

	/**
	 * Show whole set of results 
	 * 
	 * @param SearchResultSet $matches
	 */
	function showMatches( &$matches ) {
		$fname = 'SpecialSearch::showMatches';
		wfProfileIn( $fname );

		global $wgContLang;
		$terms = $wgContLang->convertForSearchResult( $matches->termMatches() );

		$out = "";
		
		$infoLine = $matches->getInfo();
		if( !is_null($infoLine) )
			$out .= "\n<!-- {$infoLine} -->\n";
			
		
		$off = $this->offset + 1;
		$out .= "<ul class='mw-search-results'>\n";

		while( $result = $matches->next() ) {
			$out .= $this->showHit( $result, $terms );
		}
		$out .= "</ul>\n";

		// convert the whole thing to desired language variant
		global $wgContLang;
		$out = $wgContLang->convert( $out );
		wfProfileOut( $fname );
		return $out;
	}

	/**
	 * Format a single hit result
	 * @param SearchResult $result
	 * @param array $terms terms to highlight
	 */
	function showHit( $result, $terms ) {
		$fname = 'SpecialSearch::showHit';
		wfProfileIn( $fname );
		global $wgUser, $wgContLang, $wgLang;
		
		if( $result->isBrokenTitle() ) {
			wfProfileOut( $fname );
			return "<!-- Broken link in search result -->\n";
		}
		
		$t = $result->getTitle();
		$sk = $wgUser->getSkin();

		$link = $sk->makeKnownLinkObj( $t, $result->getTitleSnippet($terms));

		//If page content is not readable, just return the title.
		//This is not quite safe, but better than showing excerpts from non-readable pages
		//Note that hiding the entry entirely would screw up paging.
		if (!$t->userCanRead()) {
			wfProfileOut( $fname );
			return "<li>{$link}</li>\n";
		}

		// If the page doesn't *exist*... our search index is out of date.
		// The least confusing at this point is to drop the result.
		// You may get less results, but... oh well. :P
		if( $result->isMissingRevision() ) {
			wfProfileOut( $fname );
			return "<!-- missing page " .
				htmlspecialchars( $t->getPrefixedText() ) . "-->\n";
		}

		// format redirects / relevant sections
		$redirectTitle = $result->getRedirectTitle();
		$redirectText = $result->getRedirectSnippet($terms);
		$sectionTitle = $result->getSectionTitle();
		$sectionText = $result->getSectionSnippet($terms);
		$redirect = '';
		if( !is_null($redirectTitle) )
			$redirect = "<span class='searchalttitle'>"
				.wfMsg('search-redirect',$sk->makeKnownLinkObj( $redirectTitle, $redirectText))
				."</span>";
		$section = '';
		if( !is_null($sectionTitle) )
			$section = "<span class='searchalttitle'>" 
				.wfMsg('search-section', $sk->makeKnownLinkObj( $sectionTitle, $sectionText))
				."</span>";

		// format text extract
		$extract = "<div class='searchresult'>".$result->getTextSnippet($terms)."</div>";
		
		// format score
		if( is_null( $result->getScore() ) ) {
			// Search engine doesn't report scoring info
			$score = '';
		} else {
			$percent = sprintf( '%2.1f', $result->getScore() * 100 );
			$score = wfMsg( 'search-result-score', $wgLang->formatNum( $percent ) )
				. ' - ';
		}

		// format description
		$byteSize = $result->getByteSize();
		$wordCount = $result->getWordCount();
		$timestamp = $result->getTimestamp();
		$size = wfMsgExt( 'search-result-size', array( 'parsemag', 'escape' ),
			$sk->formatSize( $byteSize ),
			$wordCount );
		$date = $wgLang->timeanddate( $timestamp );

		// link to related articles if supported
		$related = '';
		if( $result->hasRelated() ){
			$st = SpecialPage::getTitleFor( 'Search' );
			$stParams = wfArrayToCGI( $this->powerSearchOptions(),
				array('search'    => wfMsgForContent('searchrelated').':'.$t->getPrefixedText(),
				      'fulltext'  => wfMsg('search') ));
			
			$related = ' -- <a href="'.$st->escapeLocalURL($stParams).'">'. 
				wfMsg('search-relatedarticle').'</a>';
		}
				
		// Include a thumbnail for media files...
		if( $t->getNamespace() == NS_IMAGE ) {
			$img = wfFindFile( $t );
			if( $img ) {
				$thumb = $img->getThumbnail( 120, 120 );
				if( $thumb ) {
					$desc = $img->getShortDesc();
					wfProfileOut( $fname );
					// Ugly table. :D
					// Float doesn't seem to interact well with the bullets.
					// Table messes up vertical alignment of the bullet, but I'm
					// not sure what more I can do about that. :(
					return "<li>" .
						'<table class="searchResultImage">' .
						'<tr>' .
						'<td width="120" align="center">' .
						$thumb->toHtml( array( 'desc-link' => true ) ) .
						'</td>' .
						'<td valign="top">' .
						$link .
						$extract .
						"<div class='mw-search-result-data'>{$score}{$desc} - {$date}{$related}</div>" .
						'</td>' .
						'</tr>' .
						'</table>' .
						"</li>\n";
				}
			}
		}

		wfProfileOut( $fname );
		return "<li>{$link} {$redirect} {$section} {$extract}\n" .
			"<div class='mw-search-result-data'>{$score}{$size} - {$date}{$related}</div>" .
			"</li>\n";

	}

	/**
	 * Show results from other wikis
	 * 
	 * @param SearchResultSet $matches
	 */
	function showInterwiki( &$matches, $query ) {
		$fname = 'SpecialSearch::showInterwiki';
		wfProfileIn( $fname );

		global $wgContLang;
		$terms = $wgContLang->convertForSearchResult( $matches->termMatches() );

		$out = "<div id='mw-search-interwiki'><div id='mw-search-interwiki-caption'>".wfMsg('search-interwiki-caption')."</div>\n";		
		$off = $this->offset + 1;
		$out .= "<ul start='{$off}' class='mw-search-iwresults'>\n";

		// work out custom project captions
		$customCaptions = array();
		$customLines = explode("\n",wfMsg('search-interwiki-custom')); // format per line <iwprefix>:<caption>
		foreach($customLines as $line){
			$parts = explode(":",$line,2);
			if(count($parts) == 2) // validate line
				$customCaptions[$parts[0]] = $parts[1]; 
		}
		
		
		$prev = null;
		while( $result = $matches->next() ) {
			$out .= $this->showInterwikiHit( $result, $prev, $terms, $query, $customCaptions );
			$prev = $result->getInterwikiPrefix();
		}
		// FIXME: should support paging in a non-confusing way (not sure how though, maybe via ajax)..
		$out .= "</ul></div>\n";

		// convert the whole thing to desired language variant
		global $wgContLang;
		$out = $wgContLang->convert( $out );
		wfProfileOut( $fname );
		return $out;
	}
	
	/**
	 * Show single interwiki link
	 *
	 * @param SearchResult $result
	 * @param string $lastInterwiki
	 * @param array $terms
	 * @param string $query 
	 * @param array $customCaptions iw prefix -> caption
	 */
	function showInterwikiHit( $result, $lastInterwiki, $terms, $query, $customCaptions){
		$fname = 'SpecialSearch::showInterwikiHit';
		wfProfileIn( $fname );
		global $wgUser, $wgContLang, $wgLang;
		
		if( $result->isBrokenTitle() ) {
			wfProfileOut( $fname );
			return "<!-- Broken link in search result -->\n";
		}
		
		$t = $result->getTitle();
		$sk = $wgUser->getSkin();
		
		$link = $sk->makeKnownLinkObj( $t, $result->getTitleSnippet($terms));
				
		// format redirect if any
		$redirectTitle = $result->getRedirectTitle();
		$redirectText = $result->getRedirectSnippet($terms);
		$redirect = '';
		if( !is_null($redirectTitle) )
			$redirect = "<span class='searchalttitle'>"
				.wfMsg('search-redirect',$sk->makeKnownLinkObj( $redirectTitle, $redirectText))
				."</span>";

		$out = "";
		// display project name 
		if(is_null($lastInterwiki) || $lastInterwiki != $t->getInterwiki()){
			if( key_exists($t->getInterwiki(),$customCaptions) )
				// captions from 'search-interwiki-custom'
				$caption = $customCaptions[$t->getInterwiki()];
			else{
				// default is to show the hostname of the other wiki which might suck 
				// if there are many wikis on one hostname
				$parsed = parse_url($t->getFullURL());
				$caption = wfMsg('search-interwiki-default', $parsed['host']); 
			}		
			// "more results" link (special page stuff could be localized, but we might not know target lang)
			$searchTitle = Title::newFromText($t->getInterwiki().":Special:Search");   			
			$searchLink = $sk->makeKnownLinkObj( $searchTitle, wfMsg('search-interwiki-more'),
				wfArrayToCGI(array('search' => $query, 'fulltext' => 'Search'))); 
			$out .= "</ul><div class='mw-search-interwiki-project'><span class='mw-search-interwiki-more'>{$searchLink}</span>{$caption}</div>\n<ul>";
		}

		$out .= "<li>{$link} {$redirect}</li>\n"; 
		wfProfileOut( $fname );
		return $out;
	}
	

	/**
	 * Generates the power search box at bottom of [[Special:Search]]
	 * @param $term string: search term
	 * @return $out string: HTML form
	 */
	function powerSearchBox( $term ) {
		global $wgScript;

		$namespaces = '';
		foreach( SearchEngine::searchableNamespaces() as $ns => $name ) {
			$name = str_replace( '_', ' ', $name );
			if( '' == $name ) {
				$name = wfMsg( 'blanknamespace' );
			}
			$namespaces .= Xml::openElement( 'span', array( 'style' => 'white-space: nowrap' ) ) .
					Xml::checkLabel( $name, "ns{$ns}", "mw-search-ns{$ns}", in_array( $ns, $this->namespaces ) ) .
					Xml::closeElement( 'span' ) . "\n";
		}

		$redirect = Xml::check( 'redirs', $this->searchRedirects, array( 'value' => '1', 'id' => 'redirs' ) );
		$redirectLabel = Xml::label( wfMsg( 'powersearch-redir' ), 'redirs' );
		$searchField = Xml::input( 'search', 50, $term, array( 'type' => 'text', 'id' => 'powerSearchText' ) );
		$searchButton = Xml::submitButton( wfMsg( 'powersearch' ), array( 'name' => 'fulltext' ) ) . "\n";

		$out = Xml::openElement( 'form', array(	'id' => 'powersearch', 'method' => 'get', 'action' => $wgScript ) ) .
			Xml::fieldset( wfMsg( 'powersearch-legend' ),
				Xml::hidden( 'title', 'Special:Search' ) .
				"<p>" .
				wfMsgExt( 'powersearch-ns', array( 'parseinline' ) ) .
				"<br />" .
				$namespaces .
				"</p>" .
				"<p>" .
				$redirect . " " . $redirectLabel .
				"</p>" .
				wfMsgExt( 'powersearch-field', array( 'parseinline' ) ) .
				"&nbsp;" .
				$searchField .
				"&nbsp;" .
				$searchButton ) .
			"</form>";

		return $out;
	}

	function powerSearchFocus() {
		global $wgJsMimeType;
		return "<script type=\"$wgJsMimeType\">" .
			"hookEvent(\"load\", function(){" .
				"document.getElementById('powerSearchText').focus();" .
			"});" .
			"</script>";
	}

	function shortDialog($term) {
		global $wgScript;

		$out  = Xml::openElement( 'form', array(
			'id' => 'search',
			'method' => 'get',
			'action' => $wgScript
		));
		$out .= Xml::hidden( 'title', 'Special:Search' );
		$out .= Xml::input( 'search', 50, $term, array( 'type' => 'text', 'id' => 'searchText' ) ) . ' ';
		foreach( SearchEngine::searchableNamespaces() as $ns => $name ) {
			if( in_array( $ns, $this->namespaces ) ) {
				$out .= Xml::hidden( "ns{$ns}", '1' );
			}
		}
		$out .= Xml::submitButton( wfMsg( 'searchbutton' ), array( 'name' => 'fulltext' ) );
		$out .= Xml::closeElement( 'form' );

		return $out;
	}
}
