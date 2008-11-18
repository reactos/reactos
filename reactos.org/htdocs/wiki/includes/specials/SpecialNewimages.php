<?php
/**
 * @file
 * @ingroup SpecialPage
 * FIXME: this code is crap, should use Pager and Database::select().
 */

/**
 *
 */
function wfSpecialNewimages( $par, $specialPage ) {
	global $wgUser, $wgOut, $wgLang, $wgRequest, $wgGroupPermissions, $wgMiserMode;

	$wpIlMatch = $wgRequest->getText( 'wpIlMatch' );
	$dbr = wfGetDB( DB_SLAVE );
	$sk = $wgUser->getSkin();
	$shownav = !$specialPage->including();
	$hidebots = $wgRequest->getBool('hidebots',1);

	$hidebotsql = '';
	if ($hidebots) {

		/** Make a list of group names which have the 'bot' flag
		    set.
		*/
		$botconds=array();
		foreach ($wgGroupPermissions as $groupname=>$perms) {
			if(array_key_exists('bot',$perms) && $perms['bot']) {
				$botconds[]="ug_group='$groupname'";
			}
		}

		/* If not bot groups, do not set $hidebotsql */
		if ($botconds) {
			$isbotmember=$dbr->makeList($botconds, LIST_OR);

			/** This join, in conjunction with WHERE ug_group
			    IS NULL, returns only those rows from IMAGE
		    	where the uploading user is not a member of
		    	a group which has the 'bot' permission set.
			*/
			$ug = $dbr->tableName('user_groups');
			$hidebotsql = " LEFT OUTER JOIN $ug ON img_user=ug_user AND ($isbotmember)";
		}
	}

	$image = $dbr->tableName('image');

	$sql="SELECT img_timestamp from $image";
	if ($hidebotsql) {
		$sql .= "$hidebotsql WHERE ug_group IS NULL";
	}
	$sql.=' ORDER BY img_timestamp DESC LIMIT 1';
	$res = $dbr->query($sql, 'wfSpecialNewImages');
	$row = $dbr->fetchRow($res);
	if($row!==false) {
		$ts=$row[0];
	} else {
		$ts=false;
	}
	$dbr->freeResult($res);
	$sql='';

	/** If we were clever, we'd use this to cache. */
	$latestTimestamp = wfTimestamp( TS_MW, $ts);

	/** Hardcode this for now. */
	$limit = 48;

	if ( $parval = intval( $par ) ) {
		if ( $parval <= $limit && $parval > 0 ) {
			$limit = $parval;
		}
	}

	$where = array();
	$searchpar = '';
	if ( $wpIlMatch != '' && !$wgMiserMode) {
		$nt = Title::newFromUrl( $wpIlMatch );
		if($nt ) {
			$m = $dbr->strencode( strtolower( $nt->getDBkey() ) );
			$m = str_replace( '%', "\\%", $m );
			$m = str_replace( '_', "\\_", $m );
			$where[] = "LOWER(img_name) LIKE '%{$m}%'";
			$searchpar = '&wpIlMatch=' . urlencode( $wpIlMatch );
		}
	}

	$invertSort = false;
	if( $until = $wgRequest->getVal( 'until' ) ) {
		$where[] = "img_timestamp < '" . $dbr->timestamp( $until ) . "'";
	}
	if( $from = $wgRequest->getVal( 'from' ) ) {
		$where[] = "img_timestamp >= '" . $dbr->timestamp( $from ) . "'";
		$invertSort = true;
	}
	$sql='SELECT img_size, img_name, img_user, img_user_text,'.
	     "img_description,img_timestamp FROM $image";

	if($hidebotsql) {
		$sql .= $hidebotsql;
		$where[]='ug_group IS NULL';
	}
	if(count($where)) {
		$sql.=' WHERE '.$dbr->makeList($where, LIST_AND);
	}
	$sql.=' ORDER BY img_timestamp '. ( $invertSort ? '' : ' DESC' );
	$sql.=' LIMIT '.($limit+1);
	$res = $dbr->query($sql, 'wfSpecialNewImages');

	/**
	 * We have to flip things around to get the last N after a certain date
	 */
	$images = array();
	while ( $s = $dbr->fetchObject( $res ) ) {
		if( $invertSort ) {
			array_unshift( $images, $s );
		} else {
			array_push( $images, $s );
		}
	}
	$dbr->freeResult( $res );

	$gallery = new ImageGallery();
	$firstTimestamp = null;
	$lastTimestamp = null;
	$shownImages = 0;
	foreach( $images as $s ) {
		if( ++$shownImages > $limit ) {
			# One extra just to test for whether to show a page link;
			# don't actually show it.
			break;
		}

		$name = $s->img_name;
		$ut = $s->img_user_text;

		$nt = Title::newFromText( $name, NS_IMAGE );
		$ul = $sk->makeLinkObj( Title::makeTitle( NS_USER, $ut ), $ut );

		$gallery->add( $nt, "$ul<br />\n<i>".$wgLang->timeanddate( $s->img_timestamp, true )."</i><br />\n" );

		$timestamp = wfTimestamp( TS_MW, $s->img_timestamp );
		if( empty( $firstTimestamp ) ) {
			$firstTimestamp = $timestamp;
		}
		$lastTimestamp = $timestamp;
	}

	$bydate = wfMsg( 'bydate' );
	$lt = $wgLang->formatNum( min( $shownImages, $limit ) );
	if ($shownav) {
		$text = wfMsgExt( 'imagelisttext', array('parse'), $lt, $bydate );
		$wgOut->addHTML( $text . "\n" );
	}

	$sub = wfMsg( 'ilsubmit' );
	$titleObj = SpecialPage::getTitleFor( 'Newimages' );
	$action = $titleObj->escapeLocalURL( $hidebots ? '' : 'hidebots=0' );
	if ($shownav && !$wgMiserMode) {
		$wgOut->addHTML( "<form id=\"imagesearch\" method=\"post\" action=\"" .
		  "{$action}\">" .
			Xml::input( 'wpIlMatch', 20, $wpIlMatch ) . ' ' .
		  Xml::submitButton( $sub, array( 'name' => 'wpIlSubmit' ) ) .
		  "</form>" );
	}

	/**
	 * Paging controls...
	 */

	# If we change bot visibility, this needs to be carried along.
	if(!$hidebots) {
		$botpar='&hidebots=0';
	} else {
		$botpar='';
	}
	$now = wfTimestampNow();
	$d = $wgLang->date( $now, true );
	$t = $wgLang->time( $now, true );
	$dateLink = $sk->makeKnownLinkObj( $titleObj, wfMsgHtml( 'sp-newimages-showfrom', $d, $t ), 
		'from='.$now.$botpar.$searchpar );

	$botLink = $sk->makeKnownLinkObj($titleObj, wfMsgHtml( 'showhidebots', 
		($hidebots ? wfMsgHtml('show') : wfMsgHtml('hide'))),'hidebots='.($hidebots ? '0' : '1').$searchpar);


	$opts = array( 'parsemag', 'escapenoentities' );
	$prevLink = wfMsgExt( 'prevn', $opts, $wgLang->formatNum( $limit ) );
	if( $firstTimestamp && $firstTimestamp != $latestTimestamp ) {
		$prevLink = $sk->makeKnownLinkObj( $titleObj, $prevLink, 'from=' . $firstTimestamp . $botpar . $searchpar );
	}

	$nextLink = wfMsgExt( 'nextn', $opts, $wgLang->formatNum( $limit ) );
	if( $shownImages > $limit && $lastTimestamp ) {
		$nextLink = $sk->makeKnownLinkObj( $titleObj, $nextLink, 'until=' . $lastTimestamp.$botpar.$searchpar );
	}

	$prevnext = '<p>' . $botLink . ' '. wfMsgHtml( 'viewprevnext', $prevLink, $nextLink, $dateLink ) .'</p>';

	if ($shownav)
		$wgOut->addHTML( $prevnext );

	if( count( $images ) ) {
		$wgOut->addHTML( $gallery->toHTML() );
		if ($shownav)
			$wgOut->addHTML( $prevnext );
	} else {
		$wgOut->addWikiMsg( 'noimages' );
	}
}
