<?php
/**
 * Statistics about the localisation.
 *
 * @file
 * @ingroup MaintenanceLanguage
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @author Ashar Voultoiz <thoane@altern.org>
 *
 * Output is posted from time to time on:
 * http://meta.wikimedia.org/wiki/Localization_statistics
 */
$optionsWithArgs = array( 'output' );

require_once( dirname(__FILE__).'/../commandLine.inc' );
require_once( 'languages.inc' );
require_once( dirname(__FILE__).'/StatOutputs.php' );


if ( isset( $options['help'] ) ) {
	showUsage();
}

# Default output is WikiText
if ( !isset( $options['output'] ) ) {
	$options['output'] = 'wiki';
}

/** Print a usage message*/
function showUsage() {
	print <<<END
Usage: php transstat.php [--help] [--output=csv|text|wiki]
	--help : this helpful message
	--output : select an output engine one of:
		* 'csv'      : Comma Separated Values.
		* 'wiki'     : MediaWiki syntax (default).
		* 'metawiki' : MediaWiki syntax used for Meta-Wiki.
		* 'text'     : Text with tabs.
Example: php maintenance/transstat.php --output=text

END;
	exit();
}



# Select an output engine
switch ( $options['output'] ) {
	case 'wiki':
		$wgOut = new wikiStatsOutput();
		break;
	case 'metawiki':
		$wgOut = new metawikiStatsOutput();
		break;
	case 'text':
		$wgOut = new textStatsOutput();
		break;
	case 'csv':
		$wgOut = new csvStatsOutput();
		break;
	default:
		showUsage();
}

# Languages
$wgLanguages = new languages();

# Header
$wgOut->heading();
$wgOut->blockstart();
$wgOut->element( 'Language', true );
$wgOut->element( 'Code', true );
$wgOut->element( 'Translated', true );
$wgOut->element( '%', true );
$wgOut->element( 'Obsolete', true );
$wgOut->element( '%', true );
$wgOut->element( 'Problematic', true );
$wgOut->element( '%', true );
$wgOut->blockend();

$wgGeneralMessages = $wgLanguages->getGeneralMessages();
$wgRequiredMessagesNumber = count( $wgGeneralMessages['required'] );

foreach ( $wgLanguages->getLanguages() as $code ) {
	# Don't check English or RTL English
	if ( $code == 'en' || $code == 'enRTL' ) {
		continue;
	}

	# Calculate the numbers
	$language = $wgContLang->getLanguageName( $code );
	$messages = $wgLanguages->getMessages( $code );
	$messagesNumber = count( $messages['translated'] );
	$requiredMessagesNumber = count( $messages['required'] );
	$requiredMessagesPercent = $wgOut->formatPercent( $requiredMessagesNumber, $wgRequiredMessagesNumber );
	$obsoleteMessagesNumber = count( $messages['obsolete'] );
	$obsoleteMessagesPercent = $wgOut->formatPercent( $obsoleteMessagesNumber, $messagesNumber, true );
	$messagesWithoutVariables = $wgLanguages->getMessagesWithoutVariables( $code );
	$emptyMessages = $wgLanguages->getEmptyMessages( $code );
	$messagesWithWhitespace = $wgLanguages->getMessagesWithWhitespace( $code );
	$nonXHTMLMessages = $wgLanguages->getNonXHTMLMessages( $code );
	$messagesWithWrongChars = $wgLanguages->getMessagesWithWrongChars( $code );
	$problematicMessagesNumber = count( array_unique( array_merge( $messagesWithoutVariables, $emptyMessages, $messagesWithWhitespace, $nonXHTMLMessages, $messagesWithWrongChars ) ) );
	$problematicMessagesPercent = $wgOut->formatPercent( $problematicMessagesNumber, $messagesNumber, true );

	# Output them
	$wgOut->blockstart();
	$wgOut->element( "$language" );
	$wgOut->element( "$code" );
	$wgOut->element( "$requiredMessagesNumber/$wgRequiredMessagesNumber" );
	$wgOut->element( $requiredMessagesPercent );
	$wgOut->element( "$obsoleteMessagesNumber/$messagesNumber" );
	$wgOut->element( $obsoleteMessagesPercent );
	$wgOut->element( "$problematicMessagesNumber/$messagesNumber" );
	$wgOut->element( $problematicMessagesPercent );
	$wgOut->blockend();
}

# Footer
$wgOut->footer();


