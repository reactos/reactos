<?php
/**
 * A special page to search for files by hash value as defined in the
 * img_sha1 field in the image table
 *
 * @file
 * @ingroup SpecialPage
 *
 * @author Raimond Spekking, based on Special:MIMESearch by Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

/**
 * Searches the database for files of the requested hash, comparing this with the
 * 'img_sha1' field in the image table.
 * @ingroup SpecialPage
 */
class FileDuplicateSearchPage extends QueryPage {
	var $hash, $filename;

	function FileDuplicateSearchPage( $hash, $filename ) {
		$this->hash = $hash;
		$this->filename = $filename;
	}

	function getName() { return 'FileDuplicateSearch'; }
	function isExpensive() { return false; }
	function isSyndicated() { return false; }

	function linkParameters() {
		return array( 'filename' => $this->filename );
	}

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		$image = $dbr->tableName( 'image' );
		$hash = $dbr->addQuotes( $this->hash );

		return "SELECT 'FileDuplicateSearch' AS type,
				img_name AS title,
				img_sha1 AS value,
				img_user_text,
				img_timestamp
			FROM $image
			WHERE img_sha1 = $hash
			";
	}

	function formatResult( $skin, $result ) {
		global $wgContLang, $wgLang;

		$nt = Title::makeTitle( NS_IMAGE, $result->title );
		$text = $wgContLang->convert( $nt->getText() );
		$plink = $skin->makeLink( $nt->getPrefixedText(), $text );

		$user = $skin->makeLinkObj( Title::makeTitle( NS_USER, $result->img_user_text ), $result->img_user_text );
		$time = $wgLang->timeanddate( $result->img_timestamp );

		return "$plink . . $user . . $time";
	}
}

/**
 * Output the HTML search form, and constructs the FileDuplicateSearch object.
 */
function wfSpecialFileDuplicateSearch( $par = null ) {
	global $wgRequest, $wgTitle, $wgOut, $wgLang, $wgContLang;

	$hash = '';
	$filename =  isset( $par ) ?  $par : $wgRequest->getText( 'filename' );

	$title = Title::newFromText( $filename );
	if( $title && $title->getText() != '' ) {
		$dbr = wfGetDB( DB_SLAVE );
		$image = $dbr->tableName( 'image' );
		$encFilename = $dbr->addQuotes( htmlspecialchars( $title->getDbKey() ) );
		$sql = "SELECT img_sha1 from $image where img_name = $encFilename";
		$res = $dbr->query( $sql );
		$row = $dbr->fetchRow( $res );
		if( $row !== false ) {
			$hash = $row[0];
		}
		$dbr->freeResult( $res );
	}

	# Create the input form
	$wgOut->addHTML(
		Xml::openElement( 'form', array( 'id' => 'fileduplicatesearch', 'method' => 'get', 'action' => $wgTitle->getLocalUrl() ) ) .
		Xml::openElement( 'fieldset' ) .
		Xml::element( 'legend', null, wfMsg( 'fileduplicatesearch-legend' ) ) .
		Xml::inputLabel( wfMsg( 'fileduplicatesearch-filename' ), 'filename', 'filename', 50, $filename ) . ' ' .
		Xml::submitButton( wfMsg( 'fileduplicatesearch-submit' ) ) .
		Xml::closeElement( 'fieldset' ) .
		Xml::closeElement( 'form' )
	);

	if( $hash != '' ) {
		$align = $wgContLang->isRtl() ? 'left' : 'right';

		# Show a thumbnail of the file
		$img = wfFindFile( $title );
		if ( $img ) {
			$thumb = $img->getThumbnail( 120, 120 );
			if( $thumb ) {
				$wgOut->addHTML( '<div style="float:' . $align . '" id="mw-fileduplicatesearch-icon">' .
					$thumb->toHtml( array( 'desc-link' => false ) ) . '<br />' .
					wfMsgExt( 'fileduplicatesearch-info', array( 'parse' ),
						$wgLang->formatNum( $img->getWidth() ),
						$wgLang->formatNum( $img->getHeight() ),
						$wgLang->formatSize( $img->getSize() ),
						$img->getMimeType()
					) .
					'</div>' );
			}
		}

		# Do the query
		$wpp = new FileDuplicateSearchPage( $hash, $filename );
		list( $limit, $offset ) = wfCheckLimits();
		$count = $wpp->doQuery( $offset, $limit );

		# Show a short summary
		if( $count == 1 ) {
			$wgOut->addHTML( '<p class="mw-fileduplicatesearch-result-1">' .
				wfMsgHtml( 'fileduplicatesearch-result-1', $filename ) .
				'</p>'
			);
		} elseif ( $count > 1 ) {
			$wgOut->addHTML( '<p class="mw-fileduplicatesearch-result-n">' .
				wfMsgExt( 'fileduplicatesearch-result-n', array( 'parseinline' ), $filename, $wgLang->formatNum( $count - 1 ) ) .
				'</p>'
			);
		}
	}
}
