<?php
# Copyright (C) 2003 Brion Vibber <brion@pobox.com>
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
 * @file
 * @ingroup SpecialPage
 */

function wfExportGetPagesFromCategory( $title ) {
	global $wgContLang;

	$name = $title->getDBkey();

	$dbr = wfGetDB( DB_SLAVE );

	list( $page, $categorylinks ) = $dbr->tableNamesN( 'page', 'categorylinks' );
	$sql = "SELECT page_namespace, page_title FROM $page " .
		"JOIN $categorylinks ON cl_from = page_id " .
		"WHERE cl_to = " . $dbr->addQuotes( $name );

	$pages = array();
	$res = $dbr->query( $sql, 'wfExportGetPagesFromCategory' );
	while ( $row = $dbr->fetchObject( $res ) ) {
		$n = $row->page_title;
		if ($row->page_namespace) {
			$ns = $wgContLang->getNsText( $row->page_namespace );
			$n = $ns . ':' . $n;
		}

		$pages[] = $n;
	}
	$dbr->freeResult($res);

	return $pages;
}

/**
 * Expand a list of pages to include templates used in those pages.
 * @param $inputPages array, list of titles to look up
 * @param $pageSet array, associative array indexed by titles for output
 * @return array associative array index by titles
 */
function wfExportGetTemplates( $inputPages, $pageSet ) {
	return wfExportGetLinks( $inputPages, $pageSet,
		'templatelinks',
	 	array( 'tl_namespace AS namespace', 'tl_title AS title' ),
		array( 'page_id=tl_from' ) );
}

/**
 * Expand a list of pages to include images used in those pages.
 * @param $inputPages array, list of titles to look up
 * @param $pageSet array, associative array indexed by titles for output
 * @return array associative array index by titles
 */
function wfExportGetImages( $inputPages, $pageSet ) {
	return wfExportGetLinks( $inputPages, $pageSet,
		'imagelinks',
		array( NS_IMAGE . ' AS namespace', 'il_to AS title' ),
		array( 'page_id=il_from' ) );
}

/**
 * Expand a list of pages to include items used in those pages.
 * @private
 */
function wfExportGetLinks( $inputPages, $pageSet, $table, $fields, $join ) {
	$dbr = wfGetDB( DB_SLAVE );
	foreach( $inputPages as $page ) {
		$title = Title::newFromText( $page );
		if( $title ) {
			$pageSet[$title->getPrefixedText()] = true;
			/// @fixme May or may not be more efficient to batch these
			///        by namespace when given multiple input pages.
			$result = $dbr->select(
				array( 'page', $table ),
				$fields,
				array_merge( $join,
					array(
						'page_namespace' => $title->getNamespace(),
						'page_title' => $title->getDbKey() ) ),
				__METHOD__ );
			foreach( $result as $row ) {
				$template = Title::makeTitle( $row->namespace, $row->title );
				$pageSet[$template->getPrefixedText()] = true;
			}
		}
	}
	return $pageSet;
}

/**
 * Callback function to remove empty strings from the pages array.
 */
function wfFilterPage( $page ) {
	return $page !== '' && $page !== null;
}

/**
 *
 */
function wfSpecialExport( $page = '' ) {
	global $wgOut, $wgRequest, $wgSitename, $wgExportAllowListContributors;
	global $wgExportAllowHistory, $wgExportMaxHistory;

	$curonly = true;
	$doexport = false;

	if ( $wgRequest->getCheck( 'addcat' ) ) {
		$page = $wgRequest->getText( 'pages' );
		$catname = $wgRequest->getText( 'catname' );

		if ( $catname !== '' && $catname !== NULL && $catname !== false ) {
			$t = Title::makeTitleSafe( NS_CATEGORY, $catname );
			if ( $t ) {
				/**
				 * @fixme This can lead to hitting memory limit for very large
				 * categories. Ideally we would do the lookup synchronously
				 * during the export in a single query.
				 */
				$catpages = wfExportGetPagesFromCategory( $t );
				if ( $catpages ) $page .= "\n" . implode( "\n", $catpages );
			}
		}
	}
	else if( $wgRequest->wasPosted() && $page == '' ) {
		$page = $wgRequest->getText( 'pages' );
		$curonly = $wgRequest->getCheck( 'curonly' );
		$rawOffset = $wgRequest->getVal( 'offset' );
		if( $rawOffset ) {
			$offset = wfTimestamp( TS_MW, $rawOffset );
		} else {
			$offset = null;
		}
		$limit = $wgRequest->getInt( 'limit' );
		$dir = $wgRequest->getVal( 'dir' );
		$history = array(
			'dir' => 'asc',
			'offset' => false,
			'limit' => $wgExportMaxHistory,
		);
		$historyCheck = $wgRequest->getCheck( 'history' );
		if ( $curonly ) {
			$history = WikiExporter::CURRENT;
		} elseif ( !$historyCheck ) {
			if ( $limit > 0 && $limit < $wgExportMaxHistory ) {
				$history['limit'] = $limit;
			}
			if ( !is_null( $offset ) ) {
				$history['offset'] = $offset;
			}
			if ( strtolower( $dir ) == 'desc' ) {
				$history['dir'] = 'desc';
			}
		}

		if( $page != '' ) $doexport = true;
	} else {
		// Default to current-only for GET requests
		$page = $wgRequest->getText( 'pages', $page );
		$historyCheck = $wgRequest->getCheck( 'history' );
		if( $historyCheck ) {
			$history = WikiExporter::FULL;
		} else {
			$history = WikiExporter::CURRENT;
		}

		if( $page != '' ) $doexport = true;
	}

	if( !$wgExportAllowHistory ) {
		// Override
		$history = WikiExporter::CURRENT;
	}

	$list_authors = $wgRequest->getCheck( 'listauthors' );
	if ( !$curonly || !$wgExportAllowListContributors ) $list_authors = false ;

	if ( $doexport ) {
		$wgOut->disable();

		// Cancel output buffering and gzipping if set
		// This should provide safer streaming for pages with history
		wfResetOutputBuffers();
		header( "Content-type: application/xml; charset=utf-8" );
		if( $wgRequest->getCheck( 'wpDownload' ) ) {
			// Provide a sane filename suggestion
			$filename = urlencode( $wgSitename . '-' . wfTimestampNow() . '.xml' );
			$wgRequest->response()->header( "Content-disposition: attachment;filename={$filename}" );
		}

		/* Split up the input and look up linked pages */
		$inputPages = array_filter( explode( "\n", $page ), 'wfFilterPage' );
		$pageSet = array_flip( $inputPages );

		if( $wgRequest->getCheck( 'templates' ) ) {
			$pageSet = wfExportGetTemplates( $inputPages, $pageSet );
		}

		/*
		// Enable this when we can do something useful exporting/importing image information. :)
		if( $wgRequest->getCheck( 'images' ) ) {
			$pageSet = wfExportGetImages( $inputPages, $pageSet );
		}
		*/

		$pages = array_keys( $pageSet );

		/* Ok, let's get to it... */

		$db = wfGetDB( DB_SLAVE );
		$exporter = new WikiExporter( $db, $history );
		$exporter->list_authors = $list_authors ;
		$exporter->openStream();

		foreach( $pages as $page ) {
			/*
			if( $wgExportMaxHistory && !$curonly ) {
				$title = Title::newFromText( $page );
				if( $title ) {
					$count = Revision::countByTitle( $db, $title );
					if( $count > $wgExportMaxHistory ) {
						wfDebug( __FUNCTION__ .
							": Skipped $page, $count revisions too big\n" );
						continue;
					}
				}
			}*/

			#Bug 8824: Only export pages the user can read
			$title = Title::newFromText( $page );
			if( is_null( $title ) ) continue; #TODO: perhaps output an <error> tag or something.
			if( !$title->userCanRead() ) continue; #TODO: perhaps output an <error> tag or something.

			$exporter->pageByTitle( $title );
		}

		$exporter->closeStream();
		return;
	}

	$self = SpecialPage::getTitleFor( 'Export' );
	$wgOut->addHtml( wfMsgExt( 'exporttext', 'parse' ) );

	$form = Xml::openElement( 'form', array( 'method' => 'post',
		'action' => $self->getLocalUrl( 'action=submit' ) ) );

	$form .= Xml::inputLabel( wfMsg( 'export-addcattext' )	, 'catname', 'catname', 40 ) . '&nbsp;';
	$form .= Xml::submitButton( wfMsg( 'export-addcat' ), array( 'name' => 'addcat' ) ) . '<br />';

	$form .= Xml::openElement( 'textarea', array( 'name' => 'pages', 'cols' => 40, 'rows' => 10 ) );
	$form .= htmlspecialchars( $page );
	$form .= Xml::closeElement( 'textarea' );
	$form .= '<br />';

	if( $wgExportAllowHistory ) {
		$form .= Xml::checkLabel( wfMsg( 'exportcuronly' ), 'curonly', 'curonly', true ) . '<br />';
	} else {
		$wgOut->addHtml( wfMsgExt( 'exportnohistory', 'parse' ) );
	}
	$form .= Xml::checkLabel( wfMsg( 'export-templates' ), 'templates', 'wpExportTemplates', false ) . '<br />';
	// Enable this when we can do something useful exporting/importing image information. :)
	//$form .= Xml::checkLabel( wfMsg( 'export-images' ), 'images', 'wpExportImages', false ) . '<br />';
	$form .= Xml::checkLabel( wfMsg( 'export-download' ), 'wpDownload', 'wpDownload', true ) . '<br />';

	$form .= Xml::submitButton( wfMsg( 'export-submit' ) );
	$form .= Xml::closeElement( 'form' );
	$wgOut->addHtml( $form );
}
