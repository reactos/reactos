<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * Entry point : initialise variables and call subfunctions.
 * @param $par String: becomes "FOO" when called like Special:Prefixindex/FOO (default NULL)
 * @param $specialPage SpecialPage object.
 */
function wfSpecialPrefixIndex( $par=NULL, $specialPage ) {
	global $wgRequest, $wgOut, $wgContLang;

	# GET values
	$from = $wgRequest->getVal( 'from' );
	$prefix = $wgRequest->getVal( 'prefix' );
	$namespace = $wgRequest->getInt( 'namespace' );
	$namespaces = $wgContLang->getNamespaces();

	$indexPage = new SpecialPrefixIndex();

	$wgOut->setPagetitle( ( $namespace > 0 && in_array( $namespace, array_keys( $namespaces ) ) )
		? wfMsg( 'allinnamespace', str_replace( '_', ' ', $namespaces[$namespace] ) )
		: wfMsg( 'allarticles' )
	);

	if ( isset($par) ) {
		$indexPage->showChunk( $namespace, $par, $specialPage->including(), $from );
	} elseif ( isset($prefix) ) {
		$indexPage->showChunk( $namespace, $prefix, $specialPage->including(), $from );
	} elseif ( isset($from) ) {
		$indexPage->showChunk( $namespace, $from, $specialPage->including(), $from );
	} else {
		$wgOut->addHtml($indexPage->namespaceForm ( $namespace, null ));
	}
}

/**
 * implements Special:Prefixindex
 * @ingroup SpecialPage
 */
class SpecialPrefixindex extends SpecialAllpages {
	// Inherit $maxPerPage

	// Define other properties
	protected $name = 'Prefixindex';
	protected $nsfromMsg = 'allpagesprefix';

	/**
	 * @param integer $namespace (Default NS_MAIN)
	 * @param string $from list all pages from this name (default FALSE)
	 */
	function showChunk( $namespace = NS_MAIN, $prefix, $including = false, $from = null ) {
		global $wgOut, $wgUser, $wgContLang;

		$fname = 'indexShowChunk';

		$sk = $wgUser->getSkin();

		if (!isset($from)) $from = $prefix;

		$fromList = $this->getNamespaceKeyAndText($namespace, $from);
		$prefixList = $this->getNamespaceKeyAndText($namespace, $prefix);
		$namespaces = $wgContLang->getNamespaces();
		$align = $wgContLang->isRtl() ? 'left' : 'right';

		if ( !$prefixList || !$fromList ) {
			$out = wfMsgWikiHtml( 'allpagesbadtitle' );
		} elseif ( !in_array( $namespace, array_keys( $namespaces ) ) ) {
			// Show errormessage and reset to NS_MAIN
			$out = wfMsgExt( 'allpages-bad-ns', array( 'parseinline' ), $namespace );
			$namespace = NS_MAIN;
		} else {
			list( $namespace, $prefixKey, $prefix ) = $prefixList;
			list( /* $fromNs */, $fromKey, $from ) = $fromList;

			### FIXME: should complain if $fromNs != $namespace

			$dbr = wfGetDB( DB_SLAVE );

			$res = $dbr->select( 'page',
				array( 'page_namespace', 'page_title', 'page_is_redirect' ),
				array(
					'page_namespace' => $namespace,
					'page_title LIKE \'' . $dbr->escapeLike( $prefixKey ) .'%\'',
					'page_title >= ' . $dbr->addQuotes( $fromKey ),
				),
				$fname,
				array(
					'ORDER BY'  => 'page_title',
					'LIMIT'     => $this->maxPerPage + 1,
					'USE INDEX' => 'name_title',
				)
			);

			### FIXME: side link to previous

			$n = 0;
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
					$out .= "<td>$link</td>";
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
			$nsForm = $this->namespaceForm ( $namespace, $prefix );
			$out2 = '<table style="background: inherit;" width="100%" cellpadding="0" cellspacing="0" border="0">';
			$out2 .= '<tr valign="top"><td>' . $nsForm;
			$out2 .= '</td><td align="' . $align . '" style="font-size: smaller; margin-bottom: 1em;">' .
					$sk->makeKnownLink( $wgContLang->specialPage( $this->name ),
						wfMsg ( 'allpages' ) );
			if ( isset($dbr) && $dbr && ($n == $this->maxPerPage) && ($s = $dbr->fetchObject( $res )) ) {
				$namespaceparam = $namespace ? "&namespace=$namespace" : "";
				$out2 .= " | " . $sk->makeKnownLink(
					$wgContLang->specialPage( $this->name ),
					wfMsgHtml( 'nextpage', htmlspecialchars( $s->page_title ) ),
					"from=" . wfUrlEncode ( $s->page_title ) .
					"&prefix=" . wfUrlEncode ( $prefix ) . $namespaceparam );
			}
			$out2 .= "</td></tr></table><hr />";
		}

		$wgOut->addHtml( $out2 . $out );
	}
}
