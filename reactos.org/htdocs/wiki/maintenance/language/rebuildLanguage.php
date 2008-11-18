<?php
/**
 * Rewrite the messages array in the files languages/messages/MessagesXx.php.
 *
 * @file
 * @ingroup MaintenanceLanguage
 * @defgroup MaintenanceLanguage MaintenanceLanguage
 */

require_once( dirname(__FILE__).'/../commandLine.inc' );
require_once( 'languages.inc' );
require_once( 'writeMessagesArray.inc' );

/**
 * Rewrite a messages array.
 *
 * @param $code The language code.
 * @param $write Write to the messages file?
 * @param $listUnknown List the unknown messages?
 * @param $removeUnKnown Remove the unknown messages?
 */
function rebuildLanguage( $code, $write, $listUnknown, $removeUnknown ) {
	global $wgLanguages;
	$messages = $wgLanguages->getMessages( $code );
	$messages = $messages['all'];
	MessageWriter::writeMessagesToFile( $messages, $code, $write, $listUnknown, $removeUnknown );
}

# Show help
if ( isset( $options['help'] ) ) {
	echo <<<END
Run this script to rewrite the messages array in the files languages/messages/MessagesXX.php.
Parameters:
	* lang: Language code (default: the installation default language). You can also specify "all" to check all the languages.
	* help: Show this help.
Options:
	* dry-run: Do not write the array to the file.
	* no-unknown: Do not list the unknown messages.
	* remove-unknown: Remove unknown messages.

END;
	exit();
}

# Get the language code
if ( isset( $options['lang'] ) ) {
	$wgCode = $options['lang'];
} else {
	$wgCode = $wgContLang->getCode();
}

# Get the options
$wgWriteToFile = !isset( $options['dry-run'] );
$wgListUnknownMessages = !isset( $options['no-unknown'] );
$wgRemoveUnknownMessages = isset( $options['remove-unknown'] );

# Get language objects
$wgLanguages = new languages();

# Write all the language
if ( $wgCode == 'all' ) {
	foreach ( $wgLanguages->getLanguages() as $language ) {
		rebuildLanguage( $language, $wgWriteToFile, $wgListUnknownMessages, $wgRemoveUnknownMessages );
	}
} else {
	rebuildLanguage( $wgCode, $wgWriteToFile, $wgListUnknownMessages, $wgRemoveUnknownMessages );
}
