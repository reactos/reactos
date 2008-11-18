<?php
/**
 * Contain a class for special pages
 * @file
 * @ingroup SpecialPages
 */

/**
 * List of query page classes and their associated special pages,
 * for periodic updates.
 *
 * DO NOT CHANGE THIS LIST without testing that
 * maintenance/updateSpecialPages.php still works.
 */
global $wgQueryPages; // not redundant
$wgQueryPages = array(
//         QueryPage subclass           Special page name         Limit (false for none, none for the default)
//----------------------------------------------------------------------------
	array( 'AncientPagesPage',              'Ancientpages'                  ),
	array( 'BrokenRedirectsPage',           'BrokenRedirects'               ),
	array( 'DeadendPagesPage',              'Deadendpages'                  ),
	array( 'DisambiguationsPage',           'Disambiguations'               ),
	array( 'DoubleRedirectsPage',           'DoubleRedirects'               ),
	array( 'ListredirectsPage',             'Listredirects'					),
	array( 'LonelyPagesPage',               'Lonelypages'                   ),
	array( 'LongPagesPage',                 'Longpages'                     ),
	array( 'MostcategoriesPage',            'Mostcategories'                ),
	array( 'MostimagesPage',                'Mostimages'                    ),
	array( 'MostlinkedCategoriesPage',      'Mostlinkedcategories'          ),
	array( 'SpecialMostlinkedtemplates',	'Mostlinkedtemplates'			),
	array( 'MostlinkedPage',                'Mostlinked'                    ),
	array( 'MostrevisionsPage',             'Mostrevisions'                 ),
	array( 'FewestrevisionsPage',           'Fewestrevisions'               ),
	array( 'ShortPagesPage',                'Shortpages'                    ),
	array( 'UncategorizedCategoriesPage',   'Uncategorizedcategories'       ),
	array( 'UncategorizedPagesPage',        'Uncategorizedpages'            ),
	array( 'UncategorizedImagesPage',       'Uncategorizedimages' 			),
	array( 'UncategorizedTemplatesPage',	'Uncategorizedtemplates'		),
	array( 'UnusedCategoriesPage',          'Unusedcategories'              ),
	array( 'UnusedimagesPage',              'Unusedimages'                  ),
	array( 'WantedCategoriesPage',          'Wantedcategories'              ),
	array( 'WantedPagesPage',               'Wantedpages'                   ),
	array( 'UnwatchedPagesPage',            'Unwatchedpages'                ),
	array( 'UnusedtemplatesPage',           'Unusedtemplates' 				),
	array( 'WithoutInterwikiPage',			'Withoutinterwiki'				),
);
wfRunHooks( 'wgQueryPages', array( &$wgQueryPages ) );

global $wgDisableCounters;
if ( !$wgDisableCounters )
	$wgQueryPages[] = array( 'PopularPagesPage',		'Popularpages'		);


/**
 * This is a class for doing query pages; since they're almost all the same,
 * we factor out some of the functionality into a superclass, and let
 * subclasses derive from it.
 * @ingroup SpecialPage
 */
class QueryPage {
	/**
	 * Whether or not we want plain listoutput rather than an ordered list
	 *
	 * @var bool
	 */
	var $listoutput = false;

	/**
	 * The offset and limit in use, as passed to the query() function
	 *
	 * @var integer
	 */
	var $offset = 0;
	var $limit = 0;

	/**
	 * A mutator for $this->listoutput;
	 *
	 * @param bool $bool
	 */
	function setListoutput( $bool ) {
		$this->listoutput = $bool;
	}

	/**
	 * Subclasses return their name here. Make sure the name is also
	 * specified in SpecialPage.php and in Language.php as a language message
	 * param.
	 */
	function getName() {
		return '';
	}

	/**
	 * Return title object representing this page
	 *
	 * @return Title
	 */
	function getTitle() {
		return SpecialPage::getTitleFor( $this->getName() );
	}

	/**
	 * Subclasses return an SQL query here.
	 *
	 * Note that the query itself should return the following four columns:
	 * 'type' (your special page's name), 'namespace', 'title', and 'value'
	 * *in that order*. 'value' is used for sorting.
	 *
	 * These may be stored in the querycache table for expensive queries,
	 * and that cached data will be returned sometimes, so the presence of
	 * extra fields can't be relied upon. The cached 'value' column will be
	 * an integer; non-numeric values are useful only for sorting the initial
	 * query.
	 *
	 * Don't include an ORDER or LIMIT clause, this will be added.
	 */
	function getSQL() {
		return "SELECT 'sample' as type, 0 as namespace, 'Sample result' as title, 42 as value";
	}

	/**
	 * Override to sort by increasing values
	 */
	function sortDescending() {
		return true;
	}

	function getOrder() {
		return ' ORDER BY value ' .
			($this->sortDescending() ? 'DESC' : '');
	}

	/**
	 * Is this query expensive (for some definition of expensive)? Then we
	 * don't let it run in miser mode. $wgDisableQueryPages causes all query
	 * pages to be declared expensive. Some query pages are always expensive.
	 */
	function isExpensive( ) {
		global $wgDisableQueryPages;
		return $wgDisableQueryPages;
	}

	/**
	 * Whether or not the output of the page in question is retrived from
	 * the database cache.
	 *
	 * @return bool
	 */
	function isCached() {
		global $wgMiserMode;

		return $this->isExpensive() && $wgMiserMode;
	}

	/**
	 * Sometime we dont want to build rss / atom feeds.
	 */
	function isSyndicated() {
		return true;
	}

	/**
	 * Formats the results of the query for display. The skin is the current
	 * skin; you can use it for making links. The result is a single row of
	 * result data. You should be able to grab SQL results off of it.
	 * If the function return "false", the line output will be skipped.
	 */
	function formatResult( $skin, $result ) {
		return '';
	}

	/**
	 * The content returned by this function will be output before any result
	 */
	function getPageHeader( ) {
		return '';
	}

	/**
	 * If using extra form wheely-dealies, return a set of parameters here
	 * as an associative array. They will be encoded and added to the paging
	 * links (prev/next/lengths).
	 * @return array
	 */
	function linkParameters() {
		return array();
	}

	/**
	 * Some special pages (for example SpecialListusers) might not return the
	 * current object formatted, but return the previous one instead.
	 * Setting this to return true, will call one more time wfFormatResult to
	 * be sure that the very last result is formatted and shown.
	 */
	function tryLastResult( ) {
		return false;
	}

	/**
	 * Clear the cache and save new results
	 */
	function recache( $limit, $ignoreErrors = true ) {
		$fname = get_class($this) . '::recache';
		$dbw = wfGetDB( DB_MASTER );
		$dbr = wfGetDB( DB_SLAVE, array( $this->getName(), 'QueryPage::recache', 'vslow' ) );
		if ( !$dbw || !$dbr ) {
			return false;
		}

		$querycache = $dbr->tableName( 'querycache' );

		if ( $ignoreErrors ) {
			$ignoreW = $dbw->ignoreErrors( true );
			$ignoreR = $dbr->ignoreErrors( true );
		}

		# Clear out any old cached data
		$dbw->delete( 'querycache', array( 'qc_type' => $this->getName() ), $fname );
		# Do query
		$sql = $this->getSQL() . $this->getOrder();
		if ($limit !== false)
			$sql = $dbr->limitResult($sql, $limit, 0);
		$res = $dbr->query($sql, $fname);
		$num = false;
		if ( $res ) {
			$num = $dbr->numRows( $res );
			# Fetch results
			$insertSql = "INSERT INTO $querycache (qc_type,qc_namespace,qc_title,qc_value) VALUES ";
			$first = true;
			while ( $res && $row = $dbr->fetchObject( $res ) ) {
				if ( $first ) {
					$first = false;
				} else {
					$insertSql .= ',';
				}
				if ( isset( $row->value ) ) {
					$value = $row->value;
				} else {
					$value = 0;
				}

				$insertSql .= '(' .
					$dbw->addQuotes( $row->type ) . ',' .
					$dbw->addQuotes( $row->namespace ) . ',' .
					$dbw->addQuotes( $row->title ) . ',' .
					$dbw->addQuotes( $value ) . ')';
			}

			# Save results into the querycache table on the master
			if ( !$first ) {
				if ( !$dbw->query( $insertSql, $fname ) ) {
					// Set result to false to indicate error
					$dbr->freeResult( $res );
					$res = false;
				}
			}
			if ( $res ) {
				$dbr->freeResult( $res );
			}
			if ( $ignoreErrors ) {
				$dbw->ignoreErrors( $ignoreW );
				$dbr->ignoreErrors( $ignoreR );
			}

			# Update the querycache_info record for the page
			$dbw->delete( 'querycache_info', array( 'qci_type' => $this->getName() ), $fname );
			$dbw->insert( 'querycache_info', array( 'qci_type' => $this->getName(), 'qci_timestamp' => $dbw->timestamp() ), $fname );

		}
		return $num;
	}

	/**
	 * This is the actual workhorse. It does everything needed to make a
	 * real, honest-to-gosh query page.
	 *
	 * @param $offset database query offset
	 * @param $limit database query limit
	 * @param $shownavigation show navigation like "next 200"?
	 */
	function doQuery( $offset, $limit, $shownavigation=true ) {
		global $wgUser, $wgOut, $wgLang, $wgContLang;

		$this->offset = $offset;
		$this->limit = $limit;

		$sname = $this->getName();
		$fname = get_class($this) . '::doQuery';
		$dbr = wfGetDB( DB_SLAVE );

		$wgOut->setSyndicated( $this->isSyndicated() );

		if ( !$this->isCached() ) {
			$sql = $this->getSQL();
		} else {
			# Get the cached result
			$querycache = $dbr->tableName( 'querycache' );
			$type = $dbr->strencode( $sname );
			$sql =
				"SELECT qc_type as type, qc_namespace as namespace,qc_title as title, qc_value as value
				 FROM $querycache WHERE qc_type='$type'";

			if( !$this->listoutput ) {

				# Fetch the timestamp of this update
				$tRes = $dbr->select( 'querycache_info', array( 'qci_timestamp' ), array( 'qci_type' => $type ), $fname );
				$tRow = $dbr->fetchObject( $tRes );

				if( $tRow ) {
					$updated = $wgLang->timeAndDate( $tRow->qci_timestamp, true, true );
					$wgOut->addMeta( 'Data-Cache-Time', $tRow->qci_timestamp );
					$wgOut->addInlineScript( "var dataCacheTime = '{$tRow->qci_timestamp}';" );
					$wgOut->addWikiMsg( 'perfcachedts', $updated );
				} else {
					$wgOut->addWikiMsg( 'perfcached' );
				}

				# If updates on this page have been disabled, let the user know
				# that the data set won't be refreshed for now
				global $wgDisableQueryPageUpdate;
				if( is_array( $wgDisableQueryPageUpdate ) && in_array( $this->getName(), $wgDisableQueryPageUpdate ) ) {
					$wgOut->addWikiMsg( 'querypage-no-updates' );
				}

			}

		}

		$sql .= $this->getOrder();
		$sql = $dbr->limitResult($sql, $limit, $offset);
		$res = $dbr->query( $sql );
		$num = $dbr->numRows($res);

		$this->preprocessResults( $dbr, $res );

		$wgOut->addHtml( XML::openElement( 'div', array('class' => 'mw-spcontent') ) );

		# Top header and navigation
		if( $shownavigation ) {
			$wgOut->addHtml( $this->getPageHeader() );
			if( $num > 0 ) {
				$wgOut->addHtml( '<p>' . wfShowingResults( $offset, $num ) . '</p>' );
				# Disable the "next" link when we reach the end
				$paging = wfViewPrevNext( $offset, $limit, $wgContLang->specialPage( $sname ),
					wfArrayToCGI( $this->linkParameters() ), ( $num < $limit ) );
				$wgOut->addHtml( '<p>' . $paging . '</p>' );
			} else {
				# No results to show, so don't bother with "showing X of Y" etc.
				# -- just let the user know and give up now
				$wgOut->addHtml( '<p>' . wfMsgHtml( 'specialpage-empty' ) . '</p>' );
				$wgOut->addHtml( XML::closeElement( 'div' ) );
				return;
			}
		}

		# The actual results; specialist subclasses will want to handle this
		# with more than a straight list, so we hand them the info, plus
		# an OutputPage, and let them get on with it
		$this->outputResults( $wgOut,
			$wgUser->getSkin(),
			$dbr, # Should use a ResultWrapper for this
			$res,
			$dbr->numRows( $res ),
			$offset );

		# Repeat the paging links at the bottom
		if( $shownavigation ) {
			$wgOut->addHtml( '<p>' . $paging . '</p>' );
		}

		$wgOut->addHtml( XML::closeElement( 'div' ) );

		return $num;
	}

	/**
	 * Format and output report results using the given information plus
	 * OutputPage
	 *
	 * @param OutputPage $out OutputPage to print to
	 * @param Skin $skin User skin to use
	 * @param Database $dbr Database (read) connection to use
	 * @param int $res Result pointer
	 * @param int $num Number of available result rows
	 * @param int $offset Paging offset
	 */
	protected function outputResults( $out, $skin, $dbr, $res, $num, $offset ) {
		global $wgContLang;

		if( $num > 0 ) {
			$html = array();
			if( !$this->listoutput )
				$html[] = $this->openList( $offset );

			# $res might contain the whole 1,000 rows, so we read up to
			# $num [should update this to use a Pager]
			for( $i = 0; $i < $num && $row = $dbr->fetchObject( $res ); $i++ ) {
				$line = $this->formatResult( $skin, $row );
				if( $line ) {
					$attr = ( isset( $row->usepatrol ) && $row->usepatrol && $row->patrolled == 0 )
						? ' class="not-patrolled"'
						: '';
					$html[] = $this->listoutput
						? $line
						: "<li{$attr}>{$line}</li>\n";
				}
			}

			# Flush the final result
			if( $this->tryLastResult() ) {
				$row = null;
				$line = $this->formatResult( $skin, $row );
				if( $line ) {
					$attr = ( isset( $row->usepatrol ) && $row->usepatrol && $row->patrolled == 0 )
						? ' class="not-patrolled"'
						: '';
					$html[] = $this->listoutput
						? $line
						: "<li{$attr}>{$line}</li>\n";
				}
			}

			if( !$this->listoutput )
				$html[] = $this->closeList();

			$html = $this->listoutput
				? $wgContLang->listToText( $html )
				: implode( '', $html );

			$out->addHtml( $html );
		}
	}

	function openList( $offset ) {
		return "\n<ol start='" . ( $offset + 1 ) . "' class='special'>\n";
	}

	function closeList() {
		return "</ol>\n";
	}

	/**
	 * Do any necessary preprocessing of the result object.
	 */
	function preprocessResults( $db, $res ) {}

	/**
	 * Similar to above, but packaging in a syndicated feed instead of a web page
	 */
	function doFeed( $class = '', $limit = 50 ) {
		global $wgFeed, $wgFeedClasses;

		if ( !$wgFeed ) {
			global $wgOut;
			$wgOut->addWikiMsg( 'feed-unavailable' );
			return;
		}
		
		global $wgFeedLimit;
		if( $limit > $wgFeedLimit ) {
			$limit = $wgFeedLimit;
		}

		if( isset($wgFeedClasses[$class]) ) {
			$feed = new $wgFeedClasses[$class](
				$this->feedTitle(),
				$this->feedDesc(),
				$this->feedUrl() );
			$feed->outHeader();

			$dbr = wfGetDB( DB_SLAVE );
			$sql = $this->getSQL() . $this->getOrder();
			$sql = $dbr->limitResult( $sql, $limit, 0 );
			$res = $dbr->query( $sql, 'QueryPage::doFeed' );
			while( $obj = $dbr->fetchObject( $res ) ) {
				$item = $this->feedResult( $obj );
				if( $item ) $feed->outItem( $item );
			}
			$dbr->freeResult( $res );

			$feed->outFooter();
			return true;
		} else {
			return false;
		}
	}

	/**
	 * Override for custom handling. If the titles/links are ok, just do
	 * feedItemDesc()
	 */
	function feedResult( $row ) {
		if( !isset( $row->title ) ) {
			return NULL;
		}
		$title = Title::MakeTitle( intval( $row->namespace ), $row->title );
		if( $title ) {
			$date = isset( $row->timestamp ) ? $row->timestamp : '';
			$comments = '';
			if( $title ) {
				$talkpage = $title->getTalkPage();
				$comments = $talkpage->getFullURL();
			}

			return new FeedItem(
				$title->getPrefixedText(),
				$this->feedItemDesc( $row ),
				$title->getFullURL(),
				$date,
				$this->feedItemAuthor( $row ),
				$comments);
		} else {
			return NULL;
		}
	}

	function feedItemDesc( $row ) {
		return isset( $row->comment ) ? htmlspecialchars( $row->comment ) : '';
	}

	function feedItemAuthor( $row ) {
		return isset( $row->user_text ) ? $row->user_text : '';
	}

	function feedTitle() {
		global $wgContLanguageCode, $wgSitename;
		$page = SpecialPage::getPage( $this->getName() );
		$desc = $page->getDescription();
		return "$wgSitename - $desc [$wgContLanguageCode]";
	}

	function feedDesc() {
		return wfMsg( 'tagline' );
	}

	function feedUrl() {
		$title = SpecialPage::getTitleFor( $this->getName() );
		return $title->getFullURL();
	}
}
