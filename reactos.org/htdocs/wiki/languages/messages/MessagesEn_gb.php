<?php
/** British English (British English)
 *
 * @ingroup Language
 * @file
 *
 * @author Jon Harald Søby
 */

$specialPageAliases = array(
	'Uncategorizedpages'        => array( 'UncategorisedPages' ),
	'Uncategorizedcategories'   => array( 'UncategorisedCategories' ),
	'Uncategorizedimages'       => array( 'UncategorisedImages' ),
	'Uncategorizedtemplates'    => array( 'UncategorisedTemplates' ),
);

$messages = array(
# Main script and global functions
'nosuchactiontext' => 'The action specified by the URL is not recognised by the wiki',

# Miscellaneous special pages
'uncategorizedpages'      => 'Uncategorised pages',
'uncategorizedcategories' => 'Uncategorised categories',
'uncategorizedimages'     => 'Uncategorised files',
'uncategorizedtemplates'  => 'Uncategorised templates',

# Delete/protect/revert
'sessionfailure' => 'There seems to be a problem with your login session;
this action has been cancelled as a precaution against session hijacking.
Please hit "back" and reload the page you came from, then try again.',

# Block/unblock
'blockiptext' => 'Use the form below to block write access from a specific IP address or username.
This should be done only to prevent vandalism, and in accordance with [[{{MediaWiki:Policy-url}}|policy]].
Fill in a specific reason below (for example, citing particular pages that were vandalised).',

# Metadata
'metadata-help' => 'This file contains additional information, probably added from the digital camera or scanner used to create or digitise it.
If the file has been modified from its original state, some details may not fully reflect the modified file.',

# EXIF tags
'exif-ycbcrcoefficients'   => 'Colour space transformation matrix coefficients',
'exif-colorspace'          => 'Colour space',
'exif-subsectimedigitized' => 'DateTimeDigitised subseconds',
'exif-exposureprogram'     => 'Exposure programme',

'exif-exposureprogram-2' => 'Normal programme',
'exif-exposureprogram-5' => 'Creative programme (biased toward depth of field)',
'exif-exposureprogram-6' => 'Action programme (biased toward fast shutter speed)',

'exif-sensingmethod-2' => 'One-chip colour area sensor',
'exif-sensingmethod-3' => 'Two-chip colour area sensor',
'exif-sensingmethod-4' => 'Three-chip colour area sensor',
'exif-sensingmethod-5' => 'Colour sequential area sensor',
'exif-sensingmethod-8' => 'Colour sequential linear sensor',

# E-mail address confirmation
'confirmemail_invalidated' => 'E-mail address confirmation cancelled',

);
