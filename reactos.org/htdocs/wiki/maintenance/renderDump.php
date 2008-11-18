<?php
/**
 * Take page text out of an XML dump file and render basic HTML out to files.
 * This is *NOT* suitable for publishing or offline use; it's intended for
 * running comparative tests of parsing behavior using real-world data.
 *
 * Templates etc are pulled from the local wiki database, not from the dump.
 *
 * Copyright (C) 2006 Brion Vibber <brion@pobox.com>
 * http://www.mediawiki.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @file
 * @ingroup Maintenance
 */

$optionsWithArgs = array( 'report' );

require_once( 'commandLine.inc' );

class DumpRenderer {
	function __construct( $dir ) {
		$this->stderr = fopen( "php://stderr", "wt" );
		$this->outputDirectory = $dir;
		$this->count = 0;
	}

	function handleRevision( $rev ) {
		$title = $rev->getTitle();
		if (!$title) {
			fprintf( $this->stderr, "Got bogus revision with null title!" );
			return;
		}
		$display = $title->getPrefixedText();
		
		$this->count++;
		
		$sanitized = rawurlencode( $display );
		$filename = sprintf( "%s/wiki-%07d-%s.html", 
			$this->outputDirectory,
			$this->count,
			$sanitized );
		fprintf( $this->stderr, "%s\n", $filename, $display );
		
		// fixme
		$user = new User();
		$parser = new Parser();
		$options = ParserOptions::newFromUser( $user );
		
		$output = $parser->parse( $rev->getText(), $title, $options );
		
		file_put_contents( $filename,
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" " .
			"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n" .
			"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n" .
			"<head>\n" .
			"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n" .
			"<title>" . htmlspecialchars( $display ) . "</title>\n" .
			"</head>\n" . 
			"<body>\n" .
			$output->getText() .
			"</body>\n" .
			"</html>" );
	}

	function run() {
		$this->startTime = wfTime();

		$file = fopen( 'php://stdin', 'rt' );
		$source = new ImportStreamSource( $file );
		$importer = new WikiImporter( $source );

		$importer->setRevisionCallback(
			array( &$this, 'handleRevision' ) );

		return $importer->doImport();
	}
}

if( isset( $options['output-dir'] ) ) {
	$dir = $options['output-dir'];
} else {
	wfDie( "Must use --output-dir=/some/dir\n" );
}
$render = new DumpRenderer( $dir );
$render->run();


