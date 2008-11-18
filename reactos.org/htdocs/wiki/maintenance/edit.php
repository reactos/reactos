<?php
/**
 * @file
 * @ingroup Maintenance
 */

$optionsWithArgs = array( 'u', 's' );

require_once( 'commandLine.inc' );

if ( count( $args ) == 0 || isset( $options['help'] ) ) {
	print <<<EOT
Edit an article from the command line

Usage: php edit.php [options...] <title>

Options:
  -u <user>         Username
  -s <summary>      Edit summary
  -m                Minor edit
  -b                Bot (hidden) edit
  -a                Enable autosummary
  --no-rc           Do not show the change in recent changes

If the specified user does not exist, it will be created. 
The text for the edit will be read from stdin.

EOT;
	exit( 1 );
}

$userName = isset( $options['u'] ) ? $options['u'] : 'Maintenance script';
$summary = isset( $options['s'] ) ? $options['s'] : '';
$minor = isset( $options['m'] );
$bot = isset( $options['b'] );
$autoSummary = isset( $options['a'] );
$noRC = isset( $options['no-rc'] );

$wgUser = User::newFromName( $userName );
if ( !$wgUser ) {
	print "Invalid username\n";
	exit( 1 );
}
if ( $wgUser->isAnon() ) {
	$wgUser->addToDatabase();
}

$wgTitle = Title::newFromText( $args[0] );
if ( !$wgTitle ) {
	print "Invalid title\n";
	exit( 1 );
}

$wgArticle = new Article( $wgTitle );

# Read the text
$text = file_get_contents( 'php://stdin' );

# Do the edit
print "Saving... ";
$success = $wgArticle->doEdit( $text, $summary, 
	( $minor ? EDIT_MINOR : 0 ) |
	( $bot ? EDIT_FORCE_BOT : 0 ) | 
	( $autoSummary ? EDIT_AUTOSUMMARY : 0 ) |
	( $noRC ? EDIT_SUPPRESS_RC : 0 ) );
if ( $success ) {
	print "done\n";
} else {
	print "failed\n";
	exit( 1 );
}

