<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 *
 */
function wfSpecialSpecialpages() {
	global $wgOut, $wgUser, $wgMessageCache, $wgSortSpecialPages;

	$wgMessageCache->loadAllMessages();

	$wgOut->setRobotpolicy( 'noindex,nofollow' );  # Is this really needed?
	$sk = $wgUser->getSkin();

	$pages = SpecialPage::getUsablePages();

	if( count( $pages ) == 0 ) {
		# Yeah, that was pointless. Thanks for coming.
		return;
	}

	/** Put them into a sortable array */
	$groups = array();
	foreach ( $pages as $page ) {
		if ( $page->isListed() ) {
			$group = SpecialPage::getGroup( $page );
			if( !isset($groups[$group]) ) {
				$groups[$group] = array();
			}
			$groups[$group][$page->getDescription()] = array( $page->getTitle(), $page->isRestricted() );
		}
	}

	/** Sort */
	if ( $wgSortSpecialPages ) {
		foreach( $groups as $group => $sortedPages ) {
			ksort( $groups[$group] );
		}
	}

	/** Always move "other" to end */
	if( array_key_exists('other',$groups) ) {
		$other = $groups['other'];
		unset( $groups['other'] );
		$groups['other'] = $other;
	}

	/** Now output the HTML */
	foreach ( $groups as $group => $sortedPages ) {
		$middle = ceil( count($sortedPages)/2 );
		$total = count($sortedPages);
		$count = 0;

		$wgOut->addHTML( "<h4 class='mw-specialpagesgroup'>".wfMsgHtml("specialpages-group-$group")."</h4>\n" );
		$wgOut->addHTML( "<table style='width: 100%;' class='mw-specialpages-table'><tr>" );
		$wgOut->addHTML( "<td width='30%' valign='top'><ul>\n" );
		foreach( $sortedPages as $desc => $specialpage ) {
			list( $title, $restricted ) = $specialpage;
			$link = $sk->makeKnownLinkObj( $title , htmlspecialchars( $desc ) );
			if( $restricted ) {
				$wgOut->addHTML( "<li class='mw-specialpages-page mw-specialpagerestricted'>{$link}</li>\n" );
			} else {
				$wgOut->addHTML( "<li>{$link}</li>\n" );
			}

			# Split up the larger groups
			$count++;
			if( $total > 3 && $count == $middle ) {
				$wgOut->addHTML( "</ul></td><td width='10%'></td><td width='30%' valign='top'><ul>" );
			}
		}
		$wgOut->addHTML( "</ul></td><td width='30%' valign='top'></td></tr></table>\n" );
	}
	$wgOut->addHTML(
		Xml::openElement('div', array( 'class' => 'mw-specialpages-notes' )).
		wfMsgWikiHtml('specialpages-note').
		Xml::closeElement('div')
	);
}
