<?php
/**
 * @todo document
 * @file
 * @ingroup MaintenanceLanguage
 */

/** */
require_once( dirname(__FILE__).'/../commandLine.inc' );
$wgMessageCache->disableTransform();
$messages = array();
$wgEnglishMessages = array_keys( Language::getMessagesFor( 'en' ) );
foreach ( $wgEnglishMessages as $key )
{
	$messages[$key] = wfMsg( $key );
}
print "MediaWiki $wgVersion language file\n";
print serialize( $messages );


