<?php
/**
 * MediaWiki page data importer
 * Copyright (C) 2003,2005 Brion Vibber <brion@pobox.com>
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
 * @ingroup SpecialPage
 */

/**
 * Constructor
 */
function wfSpecialImport( $page = '' ) {
	global $wgUser, $wgOut, $wgRequest, $wgTitle, $wgImportSources;
	global $wgImportTargetNamespace;

	$interwiki = false;
	$namespace = $wgImportTargetNamespace;
	$frompage = '';
	$history = true;

	if ( wfReadOnly() ) {
		$wgOut->readOnlyPage();
		return;
	}

	if( $wgRequest->wasPosted() && $wgRequest->getVal( 'action' ) == 'submit') {
		$isUpload = false;
		$namespace = $wgRequest->getIntOrNull( 'namespace' );

		switch( $wgRequest->getVal( "source" ) ) {
		case "upload":
			$isUpload = true;
			if( $wgUser->isAllowed( 'importupload' ) ) {
				$source = ImportStreamSource::newFromUpload( "xmlimport" );
			} else {
				return $wgOut->permissionRequired( 'importupload' );
			}
			break;
		case "interwiki":
			$interwiki = $wgRequest->getVal( 'interwiki' );
			$history = $wgRequest->getCheck( 'interwikiHistory' );
			$frompage = $wgRequest->getText( "frompage" );
			$source = ImportStreamSource::newFromInterwiki(
				$interwiki,
				$frompage,
				$history );
			break;
		default:
			$source = new WikiErrorMsg( "importunknownsource" );
		}

		if( WikiError::isError( $source ) ) {
			$wgOut->wrapWikiMsg( '<p class="error">$1</p>', array( 'importfailed', $source->getMessage() ) );
		} else {
			$wgOut->addWikiMsg( "importstart" );

			$importer = new WikiImporter( $source );
			if( !is_null( $namespace ) ) {
				$importer->setTargetNamespace( $namespace );
			}
			$reporter = new ImportReporter( $importer, $isUpload, $interwiki );

			$reporter->open();
			$result = $importer->doImport();
			$resultCount = $reporter->close();

			if( WikiError::isError( $result ) ) {
				# No source or XML parse error
				$wgOut->wrapWikiMsg( '<p class="error">$1</p>', array( 'importfailed', $result->getMessage() ) );
			} elseif( WikiError::isError( $resultCount ) ) {
				# Zero revisions
				$wgOut->wrapWikiMsg( '<p class="error">$1</p>', array( 'importfailed', $resultCount->getMessage() ) );
			} else {
				# Success!
				$wgOut->addWikiMsg( 'importsuccess' );
			}
			$wgOut->addWikiText( '<hr />' );
		}
	}

	$action = $wgTitle->getLocalUrl( 'action=submit' );

	if( $wgUser->isAllowed( 'importupload' ) ) {
		$wgOut->addWikiMsg( "importtext" );
		$wgOut->addHTML(
			Xml::openElement( 'fieldset' ).
			Xml::element( 'legend', null, wfMsg( 'import-upload' ) ) .
			Xml::openElement( 'form', array( 'enctype' => 'multipart/form-data', 'method' => 'post', 'action' => $action ) ) .
			Xml::hidden( 'action', 'submit' ) .
			Xml::hidden( 'source', 'upload' ) .
			Xml::input( 'xmlimport', 50, '', array( 'type' => 'file' ) ) . ' ' .
			Xml::submitButton( wfMsg( 'uploadbtn' ) ) .
			Xml::closeElement( 'form' ) .
			Xml::closeElement( 'fieldset' )
		);
	} else {
		if( empty( $wgImportSources ) ) {
			$wgOut->addWikiMsg( 'importnosources' );
		}
	}

	if( !empty( $wgImportSources ) ) {
		$wgOut->addHTML(
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'importinterwiki' ) ) .
			Xml::openElement( 'form', array( 'method' => 'post', 'action' => $action ) ) .
			wfMsgExt( 'import-interwiki-text', array( 'parse' ) ) .
			Xml::hidden( 'action', 'submit' ) .
			Xml::hidden( 'source', 'interwiki' ) .
			Xml::openElement( 'table', array( 'id' => 'mw-import-table' ) ) .
			"<tr>
				<td>" .
					Xml::openElement( 'select', array( 'name' => 'interwiki' ) )
		);
		foreach( $wgImportSources as $prefix ) {
			$selected = ( $interwiki === $prefix ) ? ' selected="selected"' : '';
			$wgOut->addHTML( Xml::option( $prefix, $prefix, $selected ) );
		}
		$wgOut->addHTML(
					Xml::closeElement( 'select' ) .
				"</td>
				<td>" .
					Xml::input( 'frompage', 50, $frompage ) .
				"</td>
			</tr>
			<tr>
				<td>
				</td>
				<td>" .
					Xml::checkLabel( wfMsg( 'import-interwiki-history' ), 'interwikiHistory', 'interwikiHistory', $history ) .
				"</td>
			</tr>
			<tr>
				<td>
				</td>
				<td>" .
					Xml::label( wfMsg( 'import-interwiki-namespace' ), 'namespace' ) .
					Xml::namespaceSelector( $namespace, '' ) .
				"</td>
			</tr>
			<tr>
				<td>
				</td>
				<td>" .
					Xml::submitButton( wfMsg( 'import-interwiki-submit' ) ) .
				"</td>
			</tr>" .
			Xml::closeElement( 'table' ).
			Xml::closeElement( 'form' ) .
			Xml::closeElement( 'fieldset' )
		);
	}
}

/**
 * Reporting callback
 * @ingroup SpecialPage
 */
class ImportReporter {
	function __construct( $importer, $upload, $interwiki ) {
		$importer->setPageOutCallback( array( $this, 'reportPage' ) );
		$this->mPageCount = 0;
		$this->mIsUpload = $upload;
		$this->mInterwiki = $interwiki;
	}

	function open() {
		global $wgOut;
		$wgOut->addHtml( "<ul>\n" );
	}

	function reportPage( $title, $origTitle, $revisionCount, $successCount ) {
		global $wgOut, $wgUser, $wgLang, $wgContLang;

		$skin = $wgUser->getSkin();

		$this->mPageCount++;

		$localCount = $wgLang->formatNum( $successCount );
		$contentCount = $wgContLang->formatNum( $successCount );

		if( $successCount > 0 ) {
			$wgOut->addHtml( "<li>" . $skin->makeKnownLinkObj( $title ) . " " .
				wfMsgExt( 'import-revision-count', array( 'parsemag', 'escape' ), $localCount ) .
				"</li>\n"
			);

			$log = new LogPage( 'import' );
			if( $this->mIsUpload ) {
				$detail = wfMsgExt( 'import-logentry-upload-detail', array( 'content', 'parsemag' ),
					$contentCount );
				$log->addEntry( 'upload', $title, $detail );
			} else {
				$interwiki = '[[:' . $this->mInterwiki . ':' .
					$origTitle->getPrefixedText() . ']]';
				$detail = wfMsgExt( 'import-logentry-interwiki-detail', array( 'content', 'parsemag' ),
					$contentCount, $interwiki );
				$log->addEntry( 'interwiki', $title, $detail );
			}

			$comment = $detail; // quick
			$dbw = wfGetDB( DB_MASTER );
			$nullRevision = Revision::newNullRevision( $dbw, $title->getArticleId(), $comment, true );
			$nullRevision->insertOn( $dbw );
			$article = new Article( $title );
			# Update page record
			$article->updateRevisionOn( $dbw, $nullRevision );
			wfRunHooks( 'NewRevisionFromEditComplete', array($article, $nullRevision, false) );
		} else {
			$wgOut->addHtml( '<li>' . wfMsgHtml( 'import-nonewrevisions' ) . '</li>' );
		}
	}

	function close() {
		global $wgOut;
		if( $this->mPageCount == 0 ) {
			$wgOut->addHtml( "</ul>\n" );
			return new WikiErrorMsg( "importnopages" );
		}
		$wgOut->addHtml( "</ul>\n" );

		return $this->mPageCount;
	}
}

/**
 *
 * @ingroup SpecialPage
 */
class WikiRevision {
	var $title = null;
	var $id = 0;
	var $timestamp = "20010115000000";
	var $user = 0;
	var $user_text = "";
	var $text = "";
	var $comment = "";
	var $minor = false;

	function setTitle( $title ) {
		if( is_object( $title ) ) {
			$this->title = $title;
		} elseif( is_null( $title ) ) {
			throw new MWException( "WikiRevision given a null title in import. You may need to adjust \$wgLegalTitleChars." );
		} else {
			throw new MWException( "WikiRevision given non-object title in import." );
		}
	}

	function setID( $id ) {
		$this->id = $id;
	}

	function setTimestamp( $ts ) {
		# 2003-08-05T18:30:02Z
		$this->timestamp = wfTimestamp( TS_MW, $ts );
	}

	function setUsername( $user ) {
		$this->user_text = $user;
	}

	function setUserIP( $ip ) {
		$this->user_text = $ip;
	}

	function setText( $text ) {
		$this->text = $text;
	}

	function setComment( $text ) {
		$this->comment = $text;
	}

	function setMinor( $minor ) {
		$this->minor = (bool)$minor;
	}

	function setSrc( $src ) {
		$this->src = $src;
	}

	function setFilename( $filename ) {
		$this->filename = $filename;
	}

	function setSize( $size ) {
		$this->size = intval( $size );
	}

	function getTitle() {
		return $this->title;
	}

	function getID() {
		return $this->id;
	}

	function getTimestamp() {
		return $this->timestamp;
	}

	function getUser() {
		return $this->user_text;
	}

	function getText() {
		return $this->text;
	}

	function getComment() {
		return $this->comment;
	}

	function getMinor() {
		return $this->minor;
	}

	function getSrc() {
		return $this->src;
	}

	function getFilename() {
		return $this->filename;
	}

	function getSize() {
		return $this->size;
	}

	function importOldRevision() {
		$dbw = wfGetDB( DB_MASTER );

		# Sneak a single revision into place
		$user = User::newFromName( $this->getUser() );
		if( $user ) {
			$userId = intval( $user->getId() );
			$userText = $user->getName();
		} else {
			$userId = 0;
			$userText = $this->getUser();
		}

		// avoid memory leak...?
		$linkCache = LinkCache::singleton();
		$linkCache->clear();

		$article = new Article( $this->title );
		$pageId = $article->getId();
		if( $pageId == 0 ) {
			# must create the page...
			$pageId = $article->insertOn( $dbw );
			$created = true;
		} else {
			$created = false;

			$prior = Revision::loadFromTimestamp( $dbw, $this->title, $this->timestamp );
			if( !is_null( $prior ) ) {
				// FIXME: this could fail slightly for multiple matches :P
				wfDebug( __METHOD__ . ": skipping existing revision for [[" .
					$this->title->getPrefixedText() . "]], timestamp " .
					$this->timestamp . "\n" );
				return false;
			}
		}

		# FIXME: Use original rev_id optionally
		# FIXME: blah blah blah

		#if( $numrows > 0 ) {
		#	return wfMsg( "importhistoryconflict" );
		#}

		# Insert the row
		$revision = new Revision( array(
			'page'       => $pageId,
			'text'       => $this->getText(),
			'comment'    => $this->getComment(),
			'user'       => $userId,
			'user_text'  => $userText,
			'timestamp'  => $this->timestamp,
			'minor_edit' => $this->minor,
			) );
		$revId = $revision->insertOn( $dbw );
		$changed = $article->updateIfNewerOn( $dbw, $revision );

		if( $created ) {
			wfDebug( __METHOD__ . ": running onArticleCreate\n" );
			Article::onArticleCreate( $this->title );

			wfDebug( __METHOD__ . ": running create updates\n" );
			$article->createUpdates( $revision );

		} elseif( $changed ) {
			wfDebug( __METHOD__ . ": running onArticleEdit\n" );
			Article::onArticleEdit( $this->title );

			wfDebug( __METHOD__ . ": running edit updates\n" );
			$article->editUpdates(
				$this->getText(),
				$this->getComment(),
				$this->minor,
				$this->timestamp,
				$revId );
		}

		return true;
	}

	function importUpload() {
		wfDebug( __METHOD__ . ": STUB\n" );

		/**
			// from file revert...
			$source = $this->file->getArchiveVirtualUrl( $this->oldimage );
			$comment = $wgRequest->getText( 'wpComment' );
			// TODO: Preserve file properties from database instead of reloading from file
			$status = $this->file->upload( $source, $comment, $comment );
			if( $status->isGood() ) {
		*/

		/**
			// from file upload...
		$this->mLocalFile = wfLocalFile( $nt );
		$this->mDestName = $this->mLocalFile->getName();
		//....
			$status = $this->mLocalFile->upload( $this->mTempPath, $this->mComment, $pageText,
			File::DELETE_SOURCE, $this->mFileProps );
			if ( !$status->isGood() ) {
				$resultDetails = array( 'internal' => $status->getWikiText() );
		*/

		// @fixme upload() uses $wgUser, which is wrong here
		// it may also create a page without our desire, also wrong potentially.
		// and, it will record a *current* upload, but we might want an archive version here

		$file = wfLocalFile( $this->getTitle() );
		if( !$file ) {
			var_dump( $file );
			wfDebug( "IMPORT: Bad file. :(\n" );
			return false;
		}

		$source = $this->downloadSource();
		if( !$source ) {
			wfDebug( "IMPORT: Could not fetch remote file. :(\n" );
			return false;
		}

		$status = $file->upload( $source,
			$this->getComment(),
			$this->getComment(), // Initial page, if none present...
			File::DELETE_SOURCE,
			false, // props...
			$this->getTimestamp() );

		if( $status->isGood() ) {
			// yay?
			wfDebug( "IMPORT: is ok?\n" );
			return true;
		}

		wfDebug( "IMPORT: is bad? " . $status->getXml() . "\n" );
		return false;

	}

	function downloadSource() {
		global $wgEnableUploads;
		if( !$wgEnableUploads ) {
			return false;
		}

		$tempo = tempnam( wfTempDir(), 'download' );
		$f = fopen( $tempo, 'wb' );
		if( !$f ) {
			wfDebug( "IMPORT: couldn't write to temp file $tempo\n" );
			return false;
		}

		// @fixme!
		$src = $this->getSrc();
		$data = Http::get( $src );
		if( !$data ) {
			wfDebug( "IMPORT: couldn't fetch source $src\n" );
			fclose( $f );
			unlink( $tempo );
			return false;
		}

		fwrite( $f, $data );
		fclose( $f );

		return $tempo;
	}

}

/**
 * implements Special:Import
 * @ingroup SpecialPage
 */
class WikiImporter {
	var $mDebug = false;
	var $mSource = null;
	var $mPageCallback = null;
	var $mPageOutCallback = null;
	var $mRevisionCallback = null;
	var $mUploadCallback = null;
	var $mTargetNamespace = null;
	var $lastfield;
	var $tagStack = array();

	function __construct( $source ) {
		$this->setRevisionCallback( array( $this, "importRevision" ) );
		$this->setUploadCallback( array( $this, "importUpload" ) );
		$this->mSource = $source;
	}

	function throwXmlError( $err ) {
		$this->debug( "FAILURE: $err" );
		wfDebug( "WikiImporter XML error: $err\n" );
	}

	# --------------

	function doImport() {
		if( empty( $this->mSource ) ) {
			return new WikiErrorMsg( "importnotext" );
		}

		$parser = xml_parser_create( "UTF-8" );

		# case folding violates XML standard, turn it off
		xml_parser_set_option( $parser, XML_OPTION_CASE_FOLDING, false );

		xml_set_object( $parser, $this );
		xml_set_element_handler( $parser, "in_start", "" );

		$offset = 0; // for context extraction on error reporting
		do {
			$chunk = $this->mSource->readChunk();
			if( !xml_parse( $parser, $chunk, $this->mSource->atEnd() ) ) {
				wfDebug( "WikiImporter::doImport encountered XML parsing error\n" );
				return new WikiXmlError( $parser, wfMsgHtml( 'import-parse-failure' ), $chunk, $offset );
			}
			$offset += strlen( $chunk );
		} while( $chunk !== false && !$this->mSource->atEnd() );
		xml_parser_free( $parser );

		return true;
	}

	function debug( $data ) {
		if( $this->mDebug ) {
			wfDebug( "IMPORT: $data\n" );
		}
	}

	function notice( $data ) {
		global $wgCommandLineMode;
		if( $wgCommandLineMode ) {
			print "$data\n";
		} else {
			global $wgOut;
			$wgOut->addHTML( "<li>" . htmlspecialchars( $data ) . "</li>\n" );
		}
	}

	/**
	 * Set debug mode...
	 */
	function setDebug( $debug ) {
		$this->mDebug = $debug;
	}

	/**
	 * Sets the action to perform as each new page in the stream is reached.
	 * @param $callback callback
	 * @return callback
	 */
	function setPageCallback( $callback ) {
		$previous = $this->mPageCallback;
		$this->mPageCallback = $callback;
		return $previous;
	}

	/**
	 * Sets the action to perform as each page in the stream is completed.
	 * Callback accepts the page title (as a Title object), a second object
	 * with the original title form (in case it's been overridden into a
	 * local namespace), and a count of revisions.
	 *
	 * @param $callback callback
	 * @return callback
	 */
	function setPageOutCallback( $callback ) {
		$previous = $this->mPageOutCallback;
		$this->mPageOutCallback = $callback;
		return $previous;
	}

	/**
	 * Sets the action to perform as each page revision is reached.
	 * @param $callback callback
	 * @return callback
	 */
	function setRevisionCallback( $callback ) {
		$previous = $this->mRevisionCallback;
		$this->mRevisionCallback = $callback;
		return $previous;
	}

	/**
	 * Sets the action to perform as each file upload version is reached.
	 * @param $callback callback
	 * @return callback
	 */
	function setUploadCallback( $callback ) {
		$previous = $this->mUploadCallback;
		$this->mUploadCallback = $callback;
		return $previous;
	}

	/**
	 * Set a target namespace to override the defaults
	 */
	function setTargetNamespace( $namespace ) {
		if( is_null( $namespace ) ) {
			// Don't override namespaces
			$this->mTargetNamespace = null;
		} elseif( $namespace >= 0 ) {
			// FIXME: Check for validity
			$this->mTargetNamespace = intval( $namespace );
		} else {
			return false;
		}
	}

	/**
	 * Default per-revision callback, performs the import.
	 * @param $revision WikiRevision
	 * @private
	 */
	function importRevision( $revision ) {
		$dbw = wfGetDB( DB_MASTER );
		return $dbw->deadlockLoop( array( $revision, 'importOldRevision' ) );
	}

	/**
	 * Dummy for now...
	 */
	function importUpload( $revision ) {
		//$dbw = wfGetDB( DB_MASTER );
		//return $dbw->deadlockLoop( array( $revision, 'importUpload' ) );
		return false;
	}

	/**
	 * Alternate per-revision callback, for debugging.
	 * @param $revision WikiRevision
	 * @private
	 */
	function debugRevisionHandler( &$revision ) {
		$this->debug( "Got revision:" );
		if( is_object( $revision->title ) ) {
			$this->debug( "-- Title: " . $revision->title->getPrefixedText() );
		} else {
			$this->debug( "-- Title: <invalid>" );
		}
		$this->debug( "-- User: " . $revision->user_text );
		$this->debug( "-- Timestamp: " . $revision->timestamp );
		$this->debug( "-- Comment: " . $revision->comment );
		$this->debug( "-- Text: " . $revision->text );
	}

	/**
	 * Notify the callback function when a new <page> is reached.
	 * @param $title Title
	 * @private
	 */
	function pageCallback( $title ) {
		if( is_callable( $this->mPageCallback ) ) {
			call_user_func( $this->mPageCallback, $title );
		}
	}

	/**
	 * Notify the callback function when a </page> is closed.
	 * @param $title Title
	 * @param $origTitle Title
	 * @param $revisionCount int
	 * @param $successCount Int: number of revisions for which callback returned true
	 * @private
	 */
	function pageOutCallback( $title, $origTitle, $revisionCount, $successCount ) {
		if( is_callable( $this->mPageOutCallback ) ) {
			call_user_func( $this->mPageOutCallback, $title, $origTitle,
				$revisionCount, $successCount );
		}
	}


	# XML parser callbacks from here out -- beware!
	function donothing( $parser, $x, $y="" ) {
		#$this->debug( "donothing" );
	}

	function in_start( $parser, $name, $attribs ) {
		$this->debug( "in_start $name" );
		if( $name != "mediawiki" ) {
			return $this->throwXMLerror( "Expected <mediawiki>, got <$name>" );
		}
		xml_set_element_handler( $parser, "in_mediawiki", "out_mediawiki" );
	}

	function in_mediawiki( $parser, $name, $attribs ) {
		$this->debug( "in_mediawiki $name" );
		if( $name == 'siteinfo' ) {
			xml_set_element_handler( $parser, "in_siteinfo", "out_siteinfo" );
		} elseif( $name == 'page' ) {
			$this->push( $name );
			$this->workRevisionCount = 0;
			$this->workSuccessCount = 0;
			$this->uploadCount = 0;
			$this->uploadSuccessCount = 0;
			xml_set_element_handler( $parser, "in_page", "out_page" );
		} else {
			return $this->throwXMLerror( "Expected <page>, got <$name>" );
		}
	}
	function out_mediawiki( $parser, $name ) {
		$this->debug( "out_mediawiki $name" );
		if( $name != "mediawiki" ) {
			return $this->throwXMLerror( "Expected </mediawiki>, got </$name>" );
		}
		xml_set_element_handler( $parser, "donothing", "donothing" );
	}


	function in_siteinfo( $parser, $name, $attribs ) {
		// no-ops for now
		$this->debug( "in_siteinfo $name" );
		switch( $name ) {
		case "sitename":
		case "base":
		case "generator":
		case "case":
		case "namespaces":
		case "namespace":
			break;
		default:
			return $this->throwXMLerror( "Element <$name> not allowed in <siteinfo>." );
		}
	}

	function out_siteinfo( $parser, $name ) {
		if( $name == "siteinfo" ) {
			xml_set_element_handler( $parser, "in_mediawiki", "out_mediawiki" );
		}
	}


	function in_page( $parser, $name, $attribs ) {
		$this->debug( "in_page $name" );
		switch( $name ) {
		case "id":
		case "title":
		case "restrictions":
			$this->appendfield = $name;
			$this->appenddata = "";
			xml_set_element_handler( $parser, "in_nothing", "out_append" );
			xml_set_character_data_handler( $parser, "char_append" );
			break;
		case "revision":
			$this->push( "revision" );
			if( is_object( $this->pageTitle ) ) {
				$this->workRevision = new WikiRevision;
				$this->workRevision->setTitle( $this->pageTitle );
				$this->workRevisionCount++;
			} else {
				// Skipping items due to invalid page title
				$this->workRevision = null;
			}
			xml_set_element_handler( $parser, "in_revision", "out_revision" );
			break;
		case "upload":
			$this->push( "upload" );
			if( is_object( $this->pageTitle ) ) {
				$this->workRevision = new WikiRevision;
				$this->workRevision->setTitle( $this->pageTitle );
				$this->uploadCount++;
			} else {
				// Skipping items due to invalid page title
				$this->workRevision = null;
			}
			xml_set_element_handler( $parser, "in_upload", "out_upload" );
			break;
		default:
			return $this->throwXMLerror( "Element <$name> not allowed in a <page>." );
		}
	}

	function out_page( $parser, $name ) {
		$this->debug( "out_page $name" );
		$this->pop();
		if( $name != "page" ) {
			return $this->throwXMLerror( "Expected </page>, got </$name>" );
		}
		xml_set_element_handler( $parser, "in_mediawiki", "out_mediawiki" );

		$this->pageOutCallback( $this->pageTitle, $this->origTitle,
			$this->workRevisionCount, $this->workSuccessCount );

		$this->workTitle = null;
		$this->workRevision = null;
		$this->workRevisionCount = 0;
		$this->workSuccessCount = 0;
		$this->pageTitle = null;
		$this->origTitle = null;
	}

	function in_nothing( $parser, $name, $attribs ) {
		$this->debug( "in_nothing $name" );
		return $this->throwXMLerror( "No child elements allowed here; got <$name>" );
	}
	function char_append( $parser, $data ) {
		$this->debug( "char_append '$data'" );
		$this->appenddata .= $data;
	}
	function out_append( $parser, $name ) {
		$this->debug( "out_append $name" );
		if( $name != $this->appendfield ) {
			return $this->throwXMLerror( "Expected </{$this->appendfield}>, got </$name>" );
		}

		switch( $this->appendfield ) {
		case "title":
			$this->workTitle = $this->appenddata;
			$this->origTitle = Title::newFromText( $this->workTitle );
			if( !is_null( $this->mTargetNamespace ) && !is_null( $this->origTitle ) ) {
				$this->pageTitle = Title::makeTitle( $this->mTargetNamespace,
					$this->origTitle->getDBkey() );
			} else {
				$this->pageTitle = Title::newFromText( $this->workTitle );
			}
			if( is_null( $this->pageTitle ) ) {
				// Invalid page title? Ignore the page
				$this->notice( "Skipping invalid page title '$this->workTitle'" );
			} else {
				$this->pageCallback( $this->workTitle );
			}
			break;
		case "id":
			if ( $this->parentTag() == 'revision' ) {
				if( $this->workRevision )
					$this->workRevision->setID( $this->appenddata );
			}
			break;
		case "text":
			if( $this->workRevision )
				$this->workRevision->setText( $this->appenddata );
			break;
		case "username":
			if( $this->workRevision )
				$this->workRevision->setUsername( $this->appenddata );
			break;
		case "ip":
			if( $this->workRevision )
				$this->workRevision->setUserIP( $this->appenddata );
			break;
		case "timestamp":
			if( $this->workRevision )
				$this->workRevision->setTimestamp( $this->appenddata );
			break;
		case "comment":
			if( $this->workRevision )
				$this->workRevision->setComment( $this->appenddata );
			break;
		case "minor":
			if( $this->workRevision )
				$this->workRevision->setMinor( true );
			break;
		case "filename":
			if( $this->workRevision )
				$this->workRevision->setFilename( $this->appenddata );
			break;
		case "src":
			if( $this->workRevision )
				$this->workRevision->setSrc( $this->appenddata );
			break;
		case "size":
			if( $this->workRevision )
				$this->workRevision->setSize( intval( $this->appenddata ) );
			break;
		default:
			$this->debug( "Bad append: {$this->appendfield}" );
		}
		$this->appendfield = "";
		$this->appenddata = "";

		$parent = $this->parentTag();
		xml_set_element_handler( $parser, "in_$parent", "out_$parent" );
		xml_set_character_data_handler( $parser, "donothing" );
	}

	function in_revision( $parser, $name, $attribs ) {
		$this->debug( "in_revision $name" );
		switch( $name ) {
		case "id":
		case "timestamp":
		case "comment":
		case "minor":
		case "text":
			$this->appendfield = $name;
			xml_set_element_handler( $parser, "in_nothing", "out_append" );
			xml_set_character_data_handler( $parser, "char_append" );
			break;
		case "contributor":
			$this->push( "contributor" );
			xml_set_element_handler( $parser, "in_contributor", "out_contributor" );
			break;
		default:
			return $this->throwXMLerror( "Element <$name> not allowed in a <revision>." );
		}
	}

	function out_revision( $parser, $name ) {
		$this->debug( "out_revision $name" );
		$this->pop();
		if( $name != "revision" ) {
			return $this->throwXMLerror( "Expected </revision>, got </$name>" );
		}
		xml_set_element_handler( $parser, "in_page", "out_page" );

		if( $this->workRevision ) {
			$ok = call_user_func_array( $this->mRevisionCallback,
				array( $this->workRevision, $this ) );
			if( $ok ) {
				$this->workSuccessCount++;
			}
		}
	}

	function in_upload( $parser, $name, $attribs ) {
		$this->debug( "in_upload $name" );
		switch( $name ) {
		case "timestamp":
		case "comment":
		case "text":
		case "filename":
		case "src":
		case "size":
			$this->appendfield = $name;
			xml_set_element_handler( $parser, "in_nothing", "out_append" );
			xml_set_character_data_handler( $parser, "char_append" );
			break;
		case "contributor":
			$this->push( "contributor" );
			xml_set_element_handler( $parser, "in_contributor", "out_contributor" );
			break;
		default:
			return $this->throwXMLerror( "Element <$name> not allowed in an <upload>." );
		}
	}

	function out_upload( $parser, $name ) {
		$this->debug( "out_revision $name" );
		$this->pop();
		if( $name != "upload" ) {
			return $this->throwXMLerror( "Expected </upload>, got </$name>" );
		}
		xml_set_element_handler( $parser, "in_page", "out_page" );

		if( $this->workRevision ) {
			$ok = call_user_func_array( $this->mUploadCallback,
				array( $this->workRevision, $this ) );
			if( $ok ) {
				$this->workUploadSuccessCount++;
			}
		}
	}

	function in_contributor( $parser, $name, $attribs ) {
		$this->debug( "in_contributor $name" );
		switch( $name ) {
		case "username":
		case "ip":
		case "id":
			$this->appendfield = $name;
			xml_set_element_handler( $parser, "in_nothing", "out_append" );
			xml_set_character_data_handler( $parser, "char_append" );
			break;
		default:
			$this->throwXMLerror( "Invalid tag <$name> in <contributor>" );
		}
	}

	function out_contributor( $parser, $name ) {
		$this->debug( "out_contributor $name" );
		$this->pop();
		if( $name != "contributor" ) {
			return $this->throwXMLerror( "Expected </contributor>, got </$name>" );
		}
		$parent = $this->parentTag();
		xml_set_element_handler( $parser, "in_$parent", "out_$parent" );
	}

	private function push( $name ) {
		array_push( $this->tagStack, $name );
		$this->debug( "PUSH $name" );
	}

	private function pop() {
		$name = array_pop( $this->tagStack );
		$this->debug( "POP $name" );
		return $name;
	}

	private function parentTag() {
		$name = $this->tagStack[count( $this->tagStack ) - 1];
		$this->debug( "PARENT $name" );
		return $name;
	}

}

/**
 * @todo document (e.g. one-sentence class description).
 * @ingroup SpecialPage
 */
class ImportStringSource {
	function __construct( $string ) {
		$this->mString = $string;
		$this->mRead = false;
	}

	function atEnd() {
		return $this->mRead;
	}

	function readChunk() {
		if( $this->atEnd() ) {
			return false;
		} else {
			$this->mRead = true;
			return $this->mString;
		}
	}
}

/**
 * @todo document (e.g. one-sentence class description).
 * @ingroup SpecialPage
 */
class ImportStreamSource {
	function __construct( $handle ) {
		$this->mHandle = $handle;
	}

	function atEnd() {
		return feof( $this->mHandle );
	}

	function readChunk() {
		return fread( $this->mHandle, 32768 );
	}

	static function newFromFile( $filename ) {
		$file = @fopen( $filename, 'rt' );
		if( !$file ) {
			return new WikiErrorMsg( "importcantopen" );
		}
		return new ImportStreamSource( $file );
	}

	static function newFromUpload( $fieldname = "xmlimport" ) {
		$upload =& $_FILES[$fieldname];

		if( !isset( $upload ) || !$upload['name'] ) {
			return new WikiErrorMsg( 'importnofile' );
		}
		if( !empty( $upload['error'] ) ) {
			switch($upload['error']){
				case 1: # The uploaded file exceeds the upload_max_filesize directive in php.ini.
					return new WikiErrorMsg( 'importuploaderrorsize' );
				case 2: # The uploaded file exceeds the MAX_FILE_SIZE directive that was specified in the HTML form.
					return new WikiErrorMsg( 'importuploaderrorsize' );
				case 3: # The uploaded file was only partially uploaded
					return new WikiErrorMsg( 'importuploaderrorpartial' );
			    case 6: #Missing a temporary folder. Introduced in PHP 4.3.10 and PHP 5.0.3.
			    	return new WikiErrorMsg( 'importuploaderrortemp' );
			    # case else: # Currently impossible
			}

		}
		$fname = $upload['tmp_name'];
		if( is_uploaded_file( $fname ) ) {
			return ImportStreamSource::newFromFile( $fname );
		} else {
			return new WikiErrorMsg( 'importnofile' );
		}
	}

	static function newFromURL( $url, $method = 'GET' ) {
		wfDebug( __METHOD__ . ": opening $url\n" );
		# Use the standard HTTP fetch function; it times out
		# quicker and sorts out user-agent problems which might
		# otherwise prevent importing from large sites, such
		# as the Wikimedia cluster, etc.
		$data = Http::request( $method, $url );
		if( $data !== false ) {
			$file = tmpfile();
			fwrite( $file, $data );
			fflush( $file );
			fseek( $file, 0 );
			return new ImportStreamSource( $file );
		} else {
			return new WikiErrorMsg( 'importcantopen' );
		}
	}

	public static function newFromInterwiki( $interwiki, $page, $history=false ) {
		if( $page == '' ) {
			return new WikiErrorMsg( 'import-noarticle' );
		}
		$link = Title::newFromText( "$interwiki:Special:Export/$page" );
		if( is_null( $link ) || $link->getInterwiki() == '' ) {
			return new WikiErrorMsg( 'importbadinterwiki' );
		} else {
			$params = $history ? 'history=1' : '';
			$url = $link->getFullUrl( $params );
			# For interwikis, use POST to avoid redirects.
			return ImportStreamSource::newFromURL( $url, "POST" );
		}
	}
}
