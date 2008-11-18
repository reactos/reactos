<?php

/**
 * Special page lists various statistics, including the contents of
 * `site_stats`, plus page view details if enabled
 *
 * @file
 * @ingroup SpecialPage
 */

/**
 * Show the special page
 *
 * @param mixed $par (not used)
 */
function wfSpecialStatistics( $par = '' ) {
	global $wgOut, $wgLang, $wgRequest;
	$dbr = wfGetDB( DB_SLAVE );

	$views = SiteStats::views();
	$edits = SiteStats::edits();
	$good = SiteStats::articles();
	$images = SiteStats::images();
	$total = SiteStats::pages();
	$users = SiteStats::users();
	$admins = SiteStats::admins();
	$numJobs = SiteStats::jobs();

	if( $wgRequest->getVal( 'action' ) == 'raw' ) {
		$wgOut->disable();
		header( 'Pragma: nocache' );
		echo "total=$total;good=$good;views=$views;edits=$edits;users=$users;admins=$admins;images=$images;jobs=$numJobs\n";
		return;
	} else {
		$text = "__NOTOC__\n";
		$text .= '==' . wfMsgNoTrans( 'sitestats' ) . "==\n";
		$text .= wfMsgExt( 'sitestatstext', array( 'parsemag' ),
			$wgLang->formatNum( $total ),
			$wgLang->formatNum( $good ),
			$wgLang->formatNum( $views ),
			$wgLang->formatNum( $edits ),
			$wgLang->formatNum( sprintf( '%.2f', $total ? $edits / $total : 0 ) ),
			$wgLang->formatNum( sprintf( '%.2f', $edits ? $views / $edits : 0 ) ),
			$wgLang->formatNum( $numJobs ),
			$wgLang->formatNum( $images )
	   	)."\n";

		$text .= "==" . wfMsgNoTrans( 'userstats' ) . "==\n";
		$text .= wfMsgExt( 'userstatstext', array ( 'parsemag' ),
			$wgLang->formatNum( $users ),
			$wgLang->formatNum( $admins ),
			'[[' . wfMsgForContent( 'grouppage-sysop' ) . ']]', # TODO somehow remove, kept for backwards compatibility
			$wgLang->formatNum( @sprintf( '%.2f', $admins / $users * 100 ) ),
			User::makeGroupLinkWiki( 'sysop' )
		)."\n";

		global $wgDisableCounters, $wgMiserMode, $wgUser, $wgLang, $wgContLang;
		if( !$wgDisableCounters && !$wgMiserMode ) {
			$res = $dbr->select(
				'page',
				array(
					'page_namespace',
					'page_title',
					'page_counter',
				),
				array(
					'page_is_redirect' => 0,
					'page_counter > 0',
				),
				__METHOD__,
				array(
					'ORDER BY' => 'page_counter DESC',
					'LIMIT' => 10,
				)
			);
			if( $res->numRows() > 0 ) {
				$text .= "==" . wfMsgNoTrans( 'statistics-mostpopular' ) . "==\n";
				while( $row = $res->fetchObject() ) {
					$title = Title::makeTitleSafe( $row->page_namespace, $row->page_title );
					if( $title instanceof Title )
						$text .= '* [[:' . $title->getPrefixedText() . ']] (' . $wgLang->formatNum( $row->page_counter ) . ")\n";
				}
				$res->free();
			}
		}

		$footer = wfMsgNoTrans( 'statistics-footer' );
		if( !wfEmptyMsg( 'statistics-footer', $footer ) && $footer != '' )
			$text .= "\n" . $footer;

		$wgOut->addWikiText( $text );
	}
}
