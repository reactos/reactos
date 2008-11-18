<?php
/**
 * Convert Language files to .po files !
 *
 * Todo:
 *   - generate .po header
 *   - fix escaping of \
 *
 * @file
 * @ingroup MaintenanceLanguage
 */

$optionsWithArgs[] = 'lang';

/** This is a command line script */
require_once(dirname(__FILE__).'/../commandLine.inc');
require_once(dirname(__FILE__).'/languages.inc');

define('ALL_LANGUAGES',    true);
define('XGETTEXT_BIN',     'xgettext');
define('MSGMERGE_BIN',     'msgmerge');

// used to generate the .pot
define('XGETTEXT_OPTIONS', '-n --keyword=wfMsg --keyword=wfMsgForContent --keyword=wfMsgHtml --keyword=wfMsgWikiHtml ');
define('MSGMERGE_OPTIONS', ' -v ');

define('LOCALE_OUTPUT_DIR', $IP.'/locale');


if( isset($options['help']) ) { usage(); wfDie(); }
// default output is WikiText
if( !isset($options['lang']) ) { $options['lang'] = ALL_LANGUAGES; }

function usage() {
print <<<END
Usage: php lang2po.php [--help] [--lang=<langcode>] [--stdout]
  --help: this message.
  --lang: a lang code you want to generate a .po for (default: all languages).

END;
}


/**
 * Return a dummy header for later edition.
 * @return string A dummy header
 */
function poHeader() {
return
'# SOME DESCRIPTIVE TITLE.
# Copyright (C) 2005 MediaWiki
# This file is distributed under the same license as the MediaWiki package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\n"
"Report-Msgid-Bugs-To: bugzilllaaaaa\n"
"POT-Creation-Date: 2005-08-16 20:13+0200\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: VARIOUS <nobody>\n"
"Language-Team: LANGUAGE <nobody>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
';
}

/**
 * generate and write a file in .po format.
 *
 * @param string $langcode Code of a language it will process.
 * @param array &$messages Array containing the various messages.
 * @return string Filename where stuff got saved or false.
 */
function generatePo($langcode, $messages) {
	$data = poHeader();

	// Generate .po entries
	foreach($messages['all'] as $identifier => $content) {
		$data .= "msgid \"$identifier\"\n";

		// Escape backslashes
		$tmp = str_replace('\\', '\\\\', $content);
		// Escape doublelquotes
		$tmp = preg_replace( "/(?<!\\\\)\"/", '\"', $tmp);
		// Rewrite multilines to gettext format
		$tmp = str_replace("\n", "\"\n\"", $tmp);

		$data .= 'msgstr "'. $tmp . "\"\n\n";
	}

	// Write the content to a file in locale/XX/messages.po
	$dir = LOCALE_OUTPUT_DIR.'/'.$langcode;
	if( !is_dir($dir) ) { mkdir( $dir, 0770 ); }
	$filename = $dir.'/fromlanguagefile.po';

	$file = fopen( $filename , 'wb' );
	if( fwrite( $file, $data ) ) {
		fclose( $file );
		return $filename;
	} else {
		fclose( $file );
		return false;
	}
}

function generatePot() {
	global $IP;
	$curdir = getcwd();
	chdir($IP);
	exec( XGETTEXT_BIN
	  .' '.XGETTEXT_OPTIONS
	  .' -o '.LOCALE_OUTPUT_DIR.'/wfMsg.pot'
	  .' includes/*php'
	  );
	chdir($curdir);
}

function applyPot($langcode) {
	$langdir = LOCALE_OUTPUT_DIR.'/'.$langcode;

	$from = $langdir.'/fromlanguagefile.po';
	$pot = LOCALE_OUTPUT_DIR.'/wfMsg.pot';
	$dest = $langdir.'/messages.po';

	// Merge template and generate file to get final .po
	exec(MSGMERGE_BIN.MSGMERGE_OPTIONS." $from $pot -o $dest ");
	// delete no more needed file
//	unlink($from);
}

// Generate a template .pot based on source tree
echo "Getting 'gettext' default messages from sources:";
generatePot();
echo "done.\n";


$langTool = new languages();

if( $options['lang'] === ALL_LANGUAGES ) {
	$codes = $langTool->getLanguages();
} else {
	$codes = array( $options['lang'] );
}

// Do all languages
foreach ( $codes as $langcode) {
	echo "Loading messages for $langcode:\n";
	if( ! generatePo($langcode, $langTool->getMessages($langcode) ) ) {
		echo "ERROR: Failed to write file.\n";
	} else {
		echo "Applying template:";
		applyPot($langcode);
	}
}

