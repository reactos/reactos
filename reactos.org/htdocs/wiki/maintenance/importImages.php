<?php

/**
 * Maintenance script to import one or more images from the local file system into
 * the wiki without using the web-based interface
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

$optionsWithArguments = array( 'extensions', 'overwrite' );
require_once( 'commandLine.inc' );
require_once( 'importImages.inc.php' );
$added = $skipped = $overwritten = 0;

echo( "Import Images\n\n" );

# Need a path
if( count( $args ) > 0 ) {

	$dir = $args[0];

	# Prepare the list of allowed extensions
	global $wgFileExtensions;
	$extensions = isset( $options['extensions'] )
		? explode( ',', strtolower( $options['extensions'] ) )
		: $wgFileExtensions;

	# Search the path provided for candidates for import
	$files = findFiles( $dir, $extensions );

	# Initialise the user for this operation
	$user = isset( $options['user'] )
		? User::newFromName( $options['user'] )
		: User::newFromName( 'Maintenance script' );
	if( !$user instanceof User )
		$user = User::newFromName( 'Maintenance script' );
	$wgUser = $user;

	# Get the upload comment
	$comment = isset( $options['comment'] )
		? $options['comment']
		: 'Importing image file';

	# Get the license specifier
	$license = isset( $options['license'] ) ? $options['license'] : '';

	# Batch "upload" operation
	if( ( $count = count( $files ) ) > 0 ) {
	
		foreach( $files as $file ) {
			$base = wfBaseName( $file );
	
			# Validate a title
			$title = Title::makeTitleSafe( NS_IMAGE, $base );
			if( !is_object( $title ) ) {
				echo( "{$base} could not be imported; a valid title cannot be produced\n" );
				continue;
			}
	
			# Check existence
			$image = wfLocalFile( $title );
			if( $image->exists() ) {
				if( isset( $options['overwrite'] ) ) {
					echo( "{$base} exists, overwriting..." );
					$svar = 'overwritten';
				} else {
					echo( "{$base} exists, skipping\n" );
					$skipped++;
					continue;
				}
			} else {
				echo( "Importing {$base}..." );
				$svar = 'added';
			}

			# Import the file	
			$archive = $image->publish( $file );
			if( WikiError::isError( $archive ) || !$archive->isGood() ) {
				echo( "failed.\n" );
				continue;
			}

			$$svar++;
			if ( $image->recordUpload( $archive->value, $comment, $license ) ) {
				# We're done!
				echo( "done.\n" );
			} else {
				echo( "failed.\n" );
			}
			
		}
		
		# Print out some statistics
		echo( "\n" );
		foreach( array( 'count' => 'Found', 'added' => 'Added',
			'skipped' => 'Skipped', 'overwritten' => 'Overwritten' ) as $var => $desc ) {
			if( $$var > 0 )
				echo( "{$desc}: {$$var}\n" );
		}
		
	} else {
		echo( "No suitable files could be found for import.\n" );
	}

} else {
	showUsage();
}

exit();

function showUsage( $reason = false ) {
	if( $reason ) {
		echo( $reason . "\n" );
	}

	echo <<<END
Imports images and other media files into the wiki
USAGE: php importImages.php [options] <dir>

<dir> : Path to the directory containing images to be imported

Options:
--extensions=<exts>	Comma-separated list of allowable extensions, defaults to \$wgFileExtensions
--overwrite			Overwrite existing images if a conflicting-named image is found
--user=<username> 	Set username of uploader, default 'Maintenance script'
--comment=<text>  	Set upload summary comment, default 'Importing image file'
--license=<code>  	Use an optional license template

END;
	exit();
}