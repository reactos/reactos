<?php

/**
 * Deletes all pages in the MediaWiki namespace which were last edited by 
 * "MediaWiki default".
 *
 * @file
 * @ingroup Maintenance
 */

if ( !defined( 'MEDIAWIKI' ) ) {
	require_once( 'commandLine.inc' );
	deleteDefaultMessages();
}

function deleteDefaultMessages() {
	$user = 'MediaWiki default';
	$reason = 'No longer required';

	global $wgUser;
	$wgUser = User::newFromName( $user );
	$wgUser->addGroup( 'bot' );
	
	$dbr = wfGetDB( DB_SLAVE );
	$res = $dbr->select( array( 'page', 'revision' ),
		array( 'page_namespace', 'page_title' ),
		array(
			'page_namespace' => NS_MEDIAWIKI,
			'page_latest=rev_id',
			'rev_user_text' => 'MediaWiki default',
		)
	);

	$dbw = wfGetDB( DB_MASTER );

	while ( $row = $dbr->fetchObject( $res ) ) {
		if ( function_exists( 'wfWaitForSlaves' ) ) {
			wfWaitForSlaves( 5 );
		}
		$dbw->ping();
		$title = Title::makeTitle( $row->page_namespace, $row->page_title );
		$article = new Article( $title );
		$dbw->begin();
		$article->doDeleteArticle( $reason );
		$dbw->commit();
	}
}

