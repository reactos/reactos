<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * Entry point : initialise variables and call subfunctions.
 * @param $par String: becomes "FOO" when called like Special:Allpages/FOO (default NULL)
 * @param $specialPage See the SpecialPage object.
 */
function wfSpecialAllpages( $par=NULL, $specialPage ) {
	global $wgRequest, $wgOut, $wgContLang;

	# GET values
	$from = $wgRequest->getVal( 'from' );
	$namespace = $wgRequest->getInt( 'namespace' );

	$namespaces = $wgContLang->getNamespaces();

	$indexPage = new SpecialAllpages();

	$wgOut->setPagetitle( ( $namespace > 0 && in_array( $namespace, array_keys( $namespaces) ) )  ?
		wfMsg( 'allinnamespace', str_replace( '_', ' ', $namespaces[$namespace] ) ) :
		wfMsg( 'allarticles' )
		);

	if ( isset($par) ) {
		$indexPage->showChunk( $namespace, $par, $specialPage->including() );
	} elseif ( isset($from) ) {
		$indexPage->showChunk( $namespace, $from, $specialPage->including() );
	} else {
		$indexPage->showToplevel ( $namespace, $specialPage->including() );
	}
}

/**
 * Implements Special:Allpages
 * @ingroup SpecialPage
 */
class SpecialAllpages {
	/**
	 * Maximum number of pages to show on single subpage.
	 */
	protected $maxPerPage = 960;

	/**
	 * Name of this special page. Used to make title objects that reference back
	 * to this page.
	 */
	protected $name = 'Allpages';

	/**
	 * Determines, which message describes the input field 'nsfrom'.
	 */
	protected $nsfromMsg = 'allpagesfrom';

/**
 * HTML for the top form
 * @param integer $namespace A namespace constant (default NS_MAIN).
 * @param string $from Article name we are starting listing at.
 */
function namespaceForm ( $namespace = NS_MAIN, $from = '' ) {
	global $wgScript;
	$t = SpecialPage::getTitleFor( $this->name );

	$out  = Xml::openElement( 'div', array( 'class' => 'namespaceoptions' ) );
	$out .= Xml::openElement( 'form', array( 'method' => 'get', 'action' => $wgScript ) );
	$out .= Xml::hidden( 'title', $t->getPrefixedText() );
	$out .= Xml::openElement( 'fieldset' );
	$out .= Xml::element( 'legend', null, wfMsg( 'allpages' ) );
	$out .= Xml::openElement( 'table', array( 'id' => 'nsselect', 'class' => 'allpages' ) );
	$out .= "<tr>
			<td class='mw-label'>" .
				Xml::label( wfMsg( $this->nsfromMsg ), 'nsfrom' ) .
			"</td>
			<td class='mw-input'>" .
				Xml::input( 'from', 20, $from, array( 'id' => 'nsfrom' ) ) .
			"</td>
		</tr>
		<tr>
			<td class='mw-label'>" .
				Xml::label( wfMsg( 'namespace' ), 'namespace' ) .
			"</td>
			<td class='mw-input'>" .
				Xml::namespaceSelector( $namespace, null ) . ' ' .
				Xml::submitButton( wfMsg( 'allpagessubmit' ) ) .
			"</td>
			</tr>";
	$out .= Xml::closeElement( 'table' );
	$out .= Xml::closeElement( 'fieldset' );
	$out .= Xml::closeElement( 'form' );
	$out .= Xml::closeElement( 'div' );
	return $out;
}

/**
 * @param integer $namespace (default NS_MAIN)
 */
function showToplevel ( $namespace = NS_MAIN, $including = false ) {
	global $wgOut, $wgContLang;
	$align = $wgContLang->isRtl() ? 'left' : 'right';

	# TODO: Either make this *much* faster or cache the title index points
	# in the querycache table.

	$dbr = wfGetDB( DB_SLAVE );
	$out = "";
	$where = array( 'page_namespace' => $namespace );

	global $wgMemc;
	$key = wfMemcKey( 'allpages', 'ns', $namespace );
	$lines = $wgMemc->get( $key );

	if( !is_array( $lines ) ) {
		$options = array( 'LIMIT' => 1 );
		if ( ! $dbr->implicitOrderby() ) {
			$options['ORDER BY'] = 'page_title';
		}
		$firstTitle = $dbr->selectField( 'page', 'page_title', $where, __METHOD__, $options );
		$lastTitle = $firstTitle;

		# This array is going to hold the page_titles in order.
		$lines = array( $firstTitle );

		# If we are going to show n rows, we need n+1 queries to find the relevant titles.
		$done = false;
		for( $i = 0; !$done; ++$i ) {
			// Fetch the last title of this chunk and the first of the next
			$chunk = is_null( $lastTitle )
				? ''
				: 'page_title >= ' . $dbr->addQuotes( $lastTitle );
			$res = $dbr->select(
				'page', /* FROM */
				'page_title', /* WHAT */
				$where + array($chunk),
				__METHOD__,
				array ('LIMIT' => 2, 'OFFSET' => $this->maxPerPage - 1, 'ORDER BY' => 'page_title') );

			if ( $s = $dbr->fetchObject( $res ) ) {
				array_push( $lines, $s->page_title );
			} else {
				// Final chunk, but ended prematurely. Go back and find the end.
				$endTitle = $dbr->selectField( 'page', 'MAX(page_title)',
					array(
						'page_namespace' => $namespace,
						$chunk
					), __METHOD__ );
				array_push( $lines, $endTitle );
				$done = true;
			}
			if( $s = $dbr->fetchObject( $res ) ) {
				array_push( $lines, $s->page_title );
				$lastTitle = $s->page_title;
			} else {
				// This was a final chunk and ended exactly at the limit.
				// Rare but convenient!
				$done = true;
			}
			$dbr->freeResult( $res );
		}
		$wgMemc->add( $key, $lines, 3600 );
	}

	// If there are only two or less sections, don't even display them.
	// Instead, display the first section directly.
	if( count( $lines ) <= 2 ) {
		$this->showChunk( $namespace, '', $including );
		return;
	}

	# At this point, $lines should contain an even number of elements.
	$out .= "<table class='allpageslist' style='background: inherit;'>";
	while ( count ( $lines ) > 0 ) {
		$inpoint = array_shift ( $lines );
		$outpoint = array_shift ( $lines );
		$out .= $this->showline ( $inpoint, $outpoint, $namespace, false );
	}
	$out .= '</table>';
	$nsForm = $this->namespaceForm( $namespace, '', false );

	# Is there more?
	if ( $including ) {
		$out2 = '';
	} else {
		$morelinks = '';
		if ( $morelinks != '' ) {
			$out2 = '<table style="background: inherit;" width="100%" cellpadding="0" cellspacing="0" border="0">';
			$out2 .= '<tr valign="top"><td>' . $nsForm;
			$out2 .= '</td><td align="' . $align . '" style="font-size: smaller; margin-bottom: 1em;">';
			$out2 .= $morelinks . '</td></tr></table><hr />';
		} else {
			$out2 = $nsForm . '<hr />';
		}
	}

	$wgOut->addHtml( $out2 . $out );
}

/**
 * @todo Document
 * @param string $from
 * @param integer $namespace (Default NS_MAIN)
 */
function showline( $inpoint, $outpoint, $namespace = NS_MAIN ) {
	global $wgContLang;
	$align = $wgContLang->isRtl() ? 'left' : 'right';
	$inpointf = htmlspecialchars( str_replace( '_', ' ', $inpoint ) );
	$outpointf = htmlspecialchars( str_replace( '_', ' ', $outpoint ) );
	$queryparams = ($namespace ? "namespace=$namespace" : '');
	$special = SpecialPage::getTitleFor( $this->name, $inpoint );
	$link = $special->escapeLocalUrl( $queryparams );

	$out = wfMsgHtml(
		'alphaindexline',
		"<a href=\"$link\">$inpointf</a></td><td><a href=\"$link\">",
		"</a></td><td><a href=\"$link\">$outpointf</a>"
	);
	return '<tr><td align="' . $align . '">'.$out.'</td></tr>';
}

/**
 * @param integer $namespace (Default NS_MAIN)
 * @param string $from list all pages from this name (default FALSE)
 */
function showChunk( $namespace = NS_MAIN, $from, $including = false ) {
	global $wgOut, $wgUser, $wgContLang;

	$sk = $wgUser->getSkin();

	$fromList = $this->getNamespaceKeyAndText($namespace, $from);
	$namespaces = $wgContLang->getNamespaces();
	$align = $wgContLang->isRtl() ? 'left' : 'right';

	$n = 0;

	if ( !$fromList ) {
		$out = wfMsgWikiHtml( 'allpagesbadtitle' );
	} elseif ( !in_array( $namespace, array_keys( $namespaces ) ) ) {
		// Show errormessage and reset to NS_MAIN
		$out = wfMsgExt( 'allpages-bad-ns', array( 'parseinline' ), $namespace );
		$namespace = NS_MAIN;
	} else {
		list( $namespace, $fromKey, $from ) = $fromList;

		$dbr = wfGetDB( DB_SLAVE );
		$res = $dbr->select( 'page',
			array( 'page_namespace', 'page_title', 'page_is_redirect' ),
			array(
				'page_namespace' => $namespace,
				'page_title >= ' . $dbr->addQuotes( $fromKey )
			),
			__METHOD__,
			array(
				'ORDER BY'  => 'page_title',
				'LIMIT'     => $this->maxPerPage + 1,
				'USE INDEX' => 'name_title',
			)
		);

		if( $res->numRows() > 0 ) {
			$out = '<table style="background: inherit;" border="0" width="100%">';
	
			while( ($n < $this->maxPerPage) && ($s = $dbr->fetchObject( $res )) ) {
				$t = Title::makeTitle( $s->page_namespace, $s->page_title );
				if( $t ) {
					$link = ($s->page_is_redirect ? '<div class="allpagesredirect">' : '' ) .
						$sk->makeKnownLinkObj( $t, htmlspecialchars( $t->getText() ), false, false ) .
						($s->page_is_redirect ? '</div>' : '' );
				} else {
					$link = '[[' . htmlspecialchars( $s->page_title ) . ']]';
				}
				if( $n % 3 == 0 ) {
					$out .= '<tr>';
				}
				$out .= "<td width=\"33%\">$link</td>";
				$n++;
				if( $n % 3 == 0 ) {
					$out .= '</tr>';
				}
			}
			if( ($n % 3) != 0 ) {
				$out .= '</tr>';
			}
			$out .= '</table>';
		} else {
			$out = '';
		}
	}

	if ( $including ) {
		$out2 = '';
	} else {
		if( $from == '' ) {
			// First chunk; no previous link.
			$prevTitle = null;
		} else {
			# Get the last title from previous chunk
			$dbr = wfGetDB( DB_SLAVE );
			$res_prev = $dbr->select(
				'page',
				'page_title',
				array( 'page_namespace' => $namespace, 'page_title < '.$dbr->addQuotes($from) ),
				__METHOD__,
				array( 'ORDER BY' => 'page_title DESC', 'LIMIT' => $this->maxPerPage, 'OFFSET' => ($this->maxPerPage - 1 ) )
			);

			# Get first title of previous complete chunk
			if( $dbr->numrows( $res_prev ) >= $this->maxPerPage ) {
				$pt = $dbr->fetchObject( $res_prev );
				$prevTitle = Title::makeTitle( $namespace, $pt->page_title );
			} else {
				# The previous chunk is not complete, need to link to the very first title
				# available in the database
				$options = array( 'LIMIT' => 1 );
				if ( ! $dbr->implicitOrderby() ) {
					$options['ORDER BY'] = 'page_title';
				}
				$reallyFirstPage_title = $dbr->selectField( 'page', 'page_title', array( 'page_namespace' => $namespace ), __METHOD__, $options );
				# Show the previous link if it s not the current requested chunk
				if( $from != $reallyFirstPage_title ) {
					$prevTitle =  Title::makeTitle( $namespace, $reallyFirstPage_title );
				} else {
					$prevTitle = null;
				}
			}
		}

		$nsForm = $this->namespaceForm( $namespace, $from );
		$out2 = '<table style="background: inherit;" width="100%" cellpadding="0" cellspacing="0" border="0">';
		$out2 .= '<tr valign="top"><td>' . $nsForm;
		$out2 .= '</td><td align="' . $align . '" style="font-size: smaller; margin-bottom: 1em;">' .
				$sk->makeKnownLink( $wgContLang->specialPage( "Allpages" ),
					wfMsgHtml ( 'allpages' ) );

		$self = SpecialPage::getTitleFor( 'Allpages' );

		# Do we put a previous link ?
		if( isset( $prevTitle ) &&  $pt = $prevTitle->getText() ) {
			$q = 'from=' . $prevTitle->getPartialUrl()
				. ( $namespace ? '&namespace=' . $namespace : '' );
			$prevLink = $sk->makeKnownLinkObj( $self,
				wfMsgHTML( 'prevpage', htmlspecialchars( $pt ) ), $q );
			$out2 .= ' | ' . $prevLink;
		}

		if( $n == $this->maxPerPage && $s = $dbr->fetchObject($res) ) {
			# $s is the first link of the next chunk
			$t = Title::MakeTitle($namespace, $s->page_title);
			$q = 'from=' . $t->getPartialUrl()
				. ( $namespace ? '&namespace=' . $namespace : '' );
			$nextLink = $sk->makeKnownLinkObj( $self,
				wfMsgHtml( 'nextpage', htmlspecialchars( $t->getText() ) ), $q );
			$out2 .= ' | ' . $nextLink;
		}
		$out2 .= "</td></tr></table><hr />";
	}

	$wgOut->addHtml( $out2 . $out );
	if( isset($prevLink) or isset($nextLink) ) {
		$wgOut->addHtml( '<hr /><p style="font-size: smaller; float: ' . $align . '">' );
		if( isset( $prevLink ) ) {
			$wgOut->addHTML( $prevLink );
		}
		if( isset( $prevLink ) && isset( $nextLink ) ) {
			$wgOut->addHTML( ' | ' );
		}
		if( isset( $nextLink ) ) {
			$wgOut->addHTML( $nextLink );
		}
		$wgOut->addHTML( '</p>' );

	}

}

/**
 * @param int $ns the namespace of the article
 * @param string $text the name of the article
 * @return array( int namespace, string dbkey, string pagename ) or NULL on error
 * @static (sort of)
 * @access private
 */
function getNamespaceKeyAndText ($ns, $text) {
	if ( $text == '' )
		return array( $ns, '', '' ); # shortcut for common case

	$t = Title::makeTitleSafe($ns, $text);
	if ( $t && $t->isLocal() ) {
		return array( $t->getNamespace(), $t->getDBkey(), $t->getText() );
	} else if ( $t ) {
		return NULL;
	}

	# try again, in case the problem was an empty pagename
	$text = preg_replace('/(#|$)/', 'X$1', $text);
	$t = Title::makeTitleSafe($ns, $text);
	if ( $t && $t->isLocal() ) {
		return array( $t->getNamespace(), '', '' );
	} else {
		return NULL;
	}
}
}
