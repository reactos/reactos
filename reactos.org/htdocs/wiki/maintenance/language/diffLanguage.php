<?php
# MediaWiki web-based config/installation
# Copyright (C) 2004 Ashar Voultoiz <thoane@altern.org> and others
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

/**
 * Usage: php DiffLanguage.php [lang [file]]
 *
 * lang: Enter the language code following "Language" of the LanguageXX.php you
 * want to check. If using linux you might need to follow case aka Zh and not
 * zh.
 *
 * file: A php language file you want to include to compare mediawiki
 * Language{Lang}.php against (for example Special:Allmessages PHP output).
 *
 * The goal is to get a list of messages not yet localised in a languageXX.php
 * file using the language.php file as reference.
 *
 * The script then print a list of wgAllMessagesXX keys that aren't localised, a
 * percentage of messages correctly localised and the number of messages to be
 * translated.
 *
 * @file
 * @ingroup MaintenanceLanguage
 */

/** This script run from the commandline */
require_once( dirname(__FILE__).'/../parserTests.inc' );
require_once( dirname(__FILE__).'/../commandLine.inc' );

if( isset($options['help']) ) { usage(); wfDie(); }

$wgLanguageCode = ucfirstlcrest($wgLanguageCode);
/** Language messages we will use as reference. By default 'en' */
$referenceMessages = $wgAllMessagesEn;
$referenceLanguage = 'En';
$referenceFilename = 'Language'.$referenceLanguage.'.php';
/** Language messages we will test. */
$testMessages = array();
$testLanguage = '';
/** whereas we use an external language file */
$externalRef = false;

# FUNCTIONS
/** @todo more informations !! */
function usage() {
echo 'php DiffLanguage.php [lang [file]] [--color=(yes|no|light)]'."\n";
}

/** Return a given string with first letter upper case, the rest lowercase */
function ucfirstlcrest($string) {
	return strtoupper(substr($string,0,1)).strtolower(substr($string,1));
}

/**
 * Return a $wgAllmessages array shipped in MediaWiki
 * @param string $languageCode Formated language code
 * @return array The MediaWiki default $wgAllMessages array requested
 */
function getMediawikiMessages($languageCode = 'En') {

	$foo = "wgAllMessages$languageCode";
	global $$foo;
	global $wgSkinNamesEn; // potentially unused global declaration?

	// it might already be loaded in LocalSettings.php
	if(!isset($$foo)) {
		global $IP;
		$langFile = $IP.'/languages/classes/Language'.$languageCode.'.php';
		if (file_exists( $langFile ) ) {
			print "Including $langFile\n";
			global $wgNamespaceNamesEn;  // potentially unused global declaration?
			include($langFile);
		} else wfDie("ERROR: The file $langFile does not exist !\n");
	}
	return $$foo;
}

/**
 * Return a $wgAllmessages array in a given file. Language of the array
 * need to be given cause we can not detect which language it provides
 * @param string $filename Filename of the file containing a message array
 * @param string $languageCode Language of the external array
 * @return array A $wgAllMessages array from an external file.
 */
function getExternalMessages($filename, $languageCode) {
	print "Including external file $filename.\n";
	include($filename);
	$foo = "wgAllMessages$languageCode";
	return $$foo;
}

# MAIN ENTRY
if ( isset($args[0]) ) {
	$lang = ucfirstlcrest($args[0],1);

	// eventually against another language file we will use as reference instead
	// of the default english language.
	if( isset($args[1])) {
		// we assume the external file contain an array of messages for the
		// lang we are testing
		$referenceMessages = getExternalMessages( $args[1], $lang );
		$referenceLanguage = $lang;
		$referenceFilename = $args[1];
		$externalRef = true;
	}

	// Load datas from MediaWiki
	$testMessages = getMediawikiMessages($lang);
	$testLanguage = $lang;
} else {
	usage();
	wfDie();
}

/** parsertest is used to do differences */
$myParserTest = new ParserTest();

# Get all references messages and check if they exist in the tested language
$i = 0;

$msg = "MW Language{$testLanguage}.php against ";
if($externalRef) { $msg .= 'external file '; }
else { $msg .= 'internal file '; }
$msg .= $referenceFilename.' ('.$referenceLanguage."):\n----\n";
echo $msg;

// process messages
foreach($referenceMessages as $index => $ref)
{
	// message is not localized
	if(!(isset($testMessages[$index]))) {
		$i++;
		print "'$index' => \"$ref\",\n";
	// Messages in the same language differs
	} elseif( ($lang == $referenceLanguage) AND ($testMessages[$index] != $ref)) {
		print "\n$index differs:\n";
		print $myParserTest->quickDiff($testMessages[$index],$ref,'tested','reference');
	}
}

echo "\n----\n".$msg;
echo "$referenceLanguage language is complete at ".number_format((100 - $i/count($wgAllMessagesEn) * 100),2)."%\n";
echo "$i unlocalised messages of the ".count($wgAllMessagesEn)." messages available.\n";

