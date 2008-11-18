<?php
/**
 * @file
 * @ingroup SpecialPage Watchlist
 */

/**
 * Constructor
 *
 * @param $par Parameter passed to the page
 */
function wfSpecialWatchlist( $par ) {
	global $wgUser, $wgOut, $wgLang, $wgRequest;
	global $wgRCShowWatchingUsers, $wgEnotifWatchlist, $wgShowUpdatedMarker;
	global $wgEnotifWatchlist;
	$fname = 'wfSpecialWatchlist';

	$skin = $wgUser->getSkin();
	$specialTitle = SpecialPage::getTitleFor( 'Watchlist' );
	$wgOut->setRobotPolicy( 'noindex,nofollow' );

	# Anons don't get a watchlist
	if( $wgUser->isAnon() ) {
		$wgOut->setPageTitle( wfMsg( 'watchnologin' ) );
		$llink = $skin->makeKnownLinkObj( SpecialPage::getTitleFor( 'Userlogin' ), wfMsgHtml( 'loginreqlink' ), 'returnto=' . $specialTitle->getPrefixedUrl() );
		$wgOut->addHtml( wfMsgWikiHtml( 'watchlistanontext', $llink ) );
		return;
	}

	$wgOut->setPageTitle( wfMsg( 'watchlist' ) );

	$sub  = wfMsgExt( 'watchlistfor', 'parseinline', $wgUser->getName() );
	$sub .= '<br />' . WatchlistEditor::buildTools( $wgUser->getSkin() );
	$wgOut->setSubtitle( $sub );

	if( ( $mode = WatchlistEditor::getMode( $wgRequest, $par ) ) !== false ) {
		$editor = new WatchlistEditor();
		$editor->execute( $wgUser, $wgOut, $wgRequest, $mode );
		return;
	}

	$uid = $wgUser->getId();
	if( ($wgEnotifWatchlist || $wgShowUpdatedMarker) && $wgRequest->getVal( 'reset' ) && $wgRequest->wasPosted() ) {
		$wgUser->clearAllNotifications( $uid );
		$wgOut->redirect( $specialTitle->getFullUrl() );
		return;
	}

	$defaults = array(
	/* float */ 'days' => floatval( $wgUser->getOption( 'watchlistdays' ) ), /* 3.0 or 0.5, watch further below */
	/* bool  */ 'hideOwn' => (int)$wgUser->getBoolOption( 'watchlisthideown' ),
	/* bool  */ 'hideBots' => (int)$wgUser->getBoolOption( 'watchlisthidebots' ),
	/* bool */ 'hideMinor' => (int)$wgUser->getBoolOption( 'watchlisthideminor' ),
	/* ?     */ 'namespace' => 'all',
	);

	extract($defaults);

	# Extract variables from the request, falling back to user preferences or
	# other default values if these don't exist
	$prefs['days'    ] = floatval( $wgUser->getOption( 'watchlistdays' ) );
	$prefs['hideown' ] = $wgUser->getBoolOption( 'watchlisthideown' );
	$prefs['hidebots'] = $wgUser->getBoolOption( 'watchlisthidebots' );
	$prefs['hideminor'] = $wgUser->getBoolOption( 'watchlisthideminor' );

	# Get query variables
	$days     = $wgRequest->getVal(  'days', $prefs['days'] );
	$hideOwn  = $wgRequest->getBool( 'hideOwn', $prefs['hideown'] );
	$hideBots = $wgRequest->getBool( 'hideBots', $prefs['hidebots'] );
	$hideMinor = $wgRequest->getBool( 'hideMinor', $prefs['hideminor'] );

	# Get namespace value, if supplied, and prepare a WHERE fragment
	$nameSpace = $wgRequest->getIntOrNull( 'namespace' );
	if( !is_null( $nameSpace ) ) {
		$nameSpace = intval( $nameSpace );
		$nameSpaceClause = " AND rc_namespace = $nameSpace";
	} else {
		$nameSpace = '';
		$nameSpaceClause = '';
	}

	$dbr = wfGetDB( DB_SLAVE, 'watchlist' );
	list( $page, $watchlist, $recentchanges ) = $dbr->tableNamesN( 'page', 'watchlist', 'recentchanges' );

	$watchlistCount = $dbr->selectField( 'watchlist', 'COUNT(*)',
		array( 'wl_user' => $uid ), __METHOD__ );
	// Adjust for page X, talk:page X, which are both stored separately,
	// but treated together
	$nitems = floor($watchlistCount / 2);

	if( is_null($days) || !is_numeric($days) ) {
		$big = 1000; /* The magical big */
		if($nitems > $big) {
			# Set default cutoff shorter
			$days = $defaults['days'] = (12.0 / 24.0); # 12 hours...
		} else {
			$days = $defaults['days']; # default cutoff for shortlisters
		}
	} else {
		$days = floatval($days);
	}

	// Dump everything here
	$nondefaults = array();

	wfAppendToArrayIfNotDefault('days'     , $days         , $defaults, $nondefaults);
	wfAppendToArrayIfNotDefault('hideOwn'  , (int)$hideOwn , $defaults, $nondefaults);
	wfAppendToArrayIfNotDefault('hideBots' , (int)$hideBots, $defaults, $nondefaults);
	wfAppendToArrayIfNotDefault( 'hideMinor', (int)$hideMinor, $defaults, $nondefaults );
	wfAppendToArrayIfNotDefault('namespace', $nameSpace    , $defaults, $nondefaults);

	$hookSql = "";
	if( ! wfRunHooks('BeforeWatchlist', array($nondefaults, $wgUser, &$hookSql)) ) {
		return;
	}

	if($nitems == 0) {
		$wgOut->addWikiMsg( 'nowatchlist' );
		return;
	}

	if ( $days <= 0 ) {
		$andcutoff = '';
	} else {
		$andcutoff = "AND rc_timestamp > '".$dbr->timestamp( time() - intval( $days * 86400 ) )."'";
		/*
		$sql = "SELECT COUNT(*) AS n FROM $page, $revision  WHERE rev_timestamp>'$cutoff' AND page_id=rev_page";
		$res = $dbr->query( $sql, $fname );
		$s = $dbr->fetchObject( $res );
		$npages = $s->n;
		*/
	}

	# If the watchlist is relatively short, it's simplest to zip
	# down its entirety and then sort the results.

	# If it's relatively long, it may be worth our while to zip
	# through the time-sorted page list checking for watched items.

	# Up estimate of watched items by 15% to compensate for talk pages...

	# Toggles
	$andHideOwn = $hideOwn ? "AND (rc_user <> $uid)" : '';
	$andHideBots = $hideBots ? "AND (rc_bot = 0)" : '';
	$andHideMinor = $hideMinor ? 'AND rc_minor = 0' : '';

	# Show watchlist header
	$header = '';
	if( $wgUser->getOption( 'enotifwatchlistpages' ) && $wgEnotifWatchlist) {
		$header .= wfMsg( 'wlheader-enotif' ) . "\n";
	}
	if ( $wgShowUpdatedMarker ) {
		$header .= wfMsg( 'wlheader-showupdated' ) . "\n";
	}

  # Toggle watchlist content (all recent edits or just the latest)
	if( $wgUser->getOption( 'extendwatchlist' )) {
		$andLatest='';
 		$limitWatchlist = 'LIMIT ' . intval( $wgUser->getOption( 'wllimit' ) );
	} else {
	# Top log Ids for a page are not stored
		$andLatest = 'AND (rc_this_oldid=page_latest OR rc_type=' . RC_LOG . ') ';
		$limitWatchlist = '';
	}

	$header .= wfMsgExt( 'watchlist-details', array( 'parsemag' ), $wgLang->formatNum( $nitems ) );
	$wgOut->addWikiText( $header );

	# Show a message about slave lag, if applicable
	if( ( $lag = $dbr->getLag() ) > 0 )
		$wgOut->showLagWarning( $lag );

	if ( $wgShowUpdatedMarker ) {
		$wgOut->addHTML( '<form action="' .
			$specialTitle->escapeLocalUrl() .
			'" method="post"><input type="submit" name="dummy" value="' .
			htmlspecialchars( wfMsg( 'enotif_reset' ) ) .
			'" /><input type="hidden" name="reset" value="all" /></form>' .
			"\n\n" );
	}
	if ( $wgShowUpdatedMarker ) {
		$wltsfield = ", ${watchlist}.wl_notificationtimestamp ";
	} else {
		$wltsfield = '';
	}
	$sql = "SELECT ${recentchanges}.* ${wltsfield}
	  FROM $watchlist,$recentchanges
	  LEFT JOIN $page ON rc_cur_id=page_id
	  WHERE wl_user=$uid
	  AND wl_namespace=rc_namespace
	  AND wl_title=rc_title
	  $andcutoff
	  $andLatest
	  $andHideOwn
	  $andHideBots
	  $andHideMinor
	  $nameSpaceClause
	  $hookSql
	  ORDER BY rc_timestamp DESC
	  $limitWatchlist";

	$res = $dbr->query( $sql, $fname );
	$numRows = $dbr->numRows( $res );

	/* Start bottom header */
	$wgOut->addHTML( "<hr />\n" );

	if($days >= 1) {
		$wgOut->addHTML(
			wfMsgExt( 'rcnote', 'parseinline',
				$wgLang->formatNum( $numRows ),
				$wgLang->formatNum( $days ),
				$wgLang->timeAndDate( wfTimestampNow(), true ),
				$wgLang->date( wfTimestampNow(), true ),
				$wgLang->time( wfTimestampNow(), true )
			) . '<br />'
		);
	} elseif($days > 0) {
		$wgOut->addHtml(
			wfMsgExt( 'wlnote', 'parseinline',
				$wgLang->formatNum( $numRows ),
				$wgLang->formatNum( round($days*24) )
			) . '<br />'
		);
	}

	$wgOut->addHTML( "\n" . wlCutoffLinks( $days, 'Watchlist', $nondefaults ) . "<br />\n" );

	# Spit out some control panel links
	$thisTitle = SpecialPage::getTitleFor( 'Watchlist' );
	$skin = $wgUser->getSkin();

	# Hide/show bot edits
	$label = $hideBots ? wfMsgHtml( 'watchlist-show-bots' ) : wfMsgHtml( 'watchlist-hide-bots' );
	$linkBits = wfArrayToCGI( array( 'hideBots' => 1 - (int)$hideBots ), $nondefaults );
	$links[] = $skin->makeKnownLinkObj( $thisTitle, $label, $linkBits );

	# Hide/show own edits
	$label = $hideOwn ? wfMsgHtml( 'watchlist-show-own' ) : wfMsgHtml( 'watchlist-hide-own' );
	$linkBits = wfArrayToCGI( array( 'hideOwn' => 1 - (int)$hideOwn ), $nondefaults );
	$links[] = $skin->makeKnownLinkObj( $thisTitle, $label, $linkBits );

	# Hide/show minor edits
	$label = $hideMinor ? wfMsgHtml( 'watchlist-show-minor' ) : wfMsgHtml( 'watchlist-hide-minor' );
	$linkBits = wfArrayToCGI( array( 'hideMinor' => 1 - (int)$hideMinor ), $nondefaults );
	$links[] = $skin->makeKnownLinkObj( $thisTitle, $label, $linkBits );

	$wgOut->addHTML( implode( ' | ', $links ) );

	# Form for namespace filtering
	$form  = Xml::openElement( 'form', array( 'method' => 'post', 'action' => $thisTitle->getLocalUrl() ) );
	$form .= '<p>';
	$form .= Xml::label( wfMsg( 'namespace' ), 'namespace' ) . '&nbsp;';
	$form .= Xml::namespaceSelector( $nameSpace, '' ) . '&nbsp;';
	$form .= Xml::submitButton( wfMsg( 'allpagessubmit' ) ) . '</p>';
	$form .= Xml::hidden( 'days', $days );
	if( $hideOwn )
		$form .= Xml::hidden( 'hideOwn', 1 );
	if( $hideBots )
		$form .= Xml::hidden( 'hideBots', 1 );
	if( $hideMinor )
		$form .= Xml::hidden( 'hideMinor', 1 );
	$form .= Xml::closeElement( 'form' );
	$wgOut->addHtml( $form );

	# If there's nothing to show, stop here
	if( $numRows == 0 ) {
		$wgOut->addWikiMsg( 'watchnochange' );
		return;
	}

	/* End bottom header */

	/* Do link batch query */
	$linkBatch = new LinkBatch;
	while ( $row = $dbr->fetchObject( $res ) ) {
		$userNameUnderscored = str_replace( ' ', '_', $row->rc_user_text );
		if ( $row->rc_user != 0 ) {
			$linkBatch->add( NS_USER, $userNameUnderscored );
		}
		$linkBatch->add( NS_USER_TALK, $userNameUnderscored );
	}
	$linkBatch->execute();
	$dbr->dataSeek( $res, 0 );

	$list = ChangesList::newFromUser( $wgUser );

	$s = $list->beginRecentChangesList();
	$counter = 1;
	while ( $obj = $dbr->fetchObject( $res ) ) {
		# Make RC entry
		$rc = RecentChange::newFromRow( $obj );
		$rc->counter = $counter++;

		if ( $wgShowUpdatedMarker ) {
			$updated = $obj->wl_notificationtimestamp;
		} else {
			$updated = false;
		}

		if ($wgRCShowWatchingUsers && $wgUser->getOption( 'shownumberswatching' )) {
			$rc->numberofWatchingusers = $dbr->selectField( 'watchlist',
				'COUNT(*)',
				array(
					'wl_namespace' => $obj->rc_namespace,
					'wl_title' => $obj->rc_title,
				),
				__METHOD__ );
		} else {
			$rc->numberofWatchingusers = 0;
		}

		$s .= $list->recentChangesLine( $rc, $updated );
	}
	$s .= $list->endRecentChangesList();

	$dbr->freeResult( $res );
	$wgOut->addHTML( $s );

}

function wlHoursLink( $h, $page, $options = array() ) {
	global $wgUser, $wgLang, $wgContLang;
	$sk = $wgUser->getSkin();
	$s = $sk->makeKnownLink(
	  $wgContLang->specialPage( $page ),
	  $wgLang->formatNum( $h ),
	  wfArrayToCGI( array('days' => ($h / 24.0)), $options ) );
	return $s;
}

function wlDaysLink( $d, $page, $options = array() ) {
	global $wgUser, $wgLang, $wgContLang;
	$sk = $wgUser->getSkin();
	$s = $sk->makeKnownLink(
	  $wgContLang->specialPage( $page ),
	  ($d ? $wgLang->formatNum( $d ) : wfMsgHtml( 'watchlistall2' ) ),
	  wfArrayToCGI( array('days' => $d), $options ) );
	return $s;
}

/**
 * Returns html
 */
function wlCutoffLinks( $days, $page = 'Watchlist', $options = array() ) {
	$hours = array( 1, 2, 6, 12 );
	$days = array( 1, 3, 7 );
	$i = 0;
	foreach( $hours as $h ) {
		$hours[$i++] = wlHoursLink( $h, $page, $options );
	}
	$i = 0;
	foreach( $days as $d ) {
		$days[$i++] = wlDaysLink( $d, $page, $options );
	}
	return wfMsgExt('wlshowlast',
		array('parseinline', 'replaceafter'),
		implode(' | ', $hours),
		implode(' | ', $days),
		wlDaysLink( 0, $page, $options ) );
}

/**
 * Count the number of items on a user's watchlist
 *
 * @param $talk Include talk pages
 * @return integer
 */
function wlCountItems( &$user, $talk = true ) {
	$dbr = wfGetDB( DB_SLAVE, 'watchlist' );

	# Fetch the raw count
	$res = $dbr->select( 'watchlist', 'COUNT(*) AS count', array( 'wl_user' => $user->mId ), 'wlCountItems' );
	$row = $dbr->fetchObject( $res );
	$count = $row->count;
	$dbr->freeResult( $res );

	# Halve to remove talk pages if needed
	if( !$talk )
		$count = floor( $count / 2 );

	return( $count );
}
