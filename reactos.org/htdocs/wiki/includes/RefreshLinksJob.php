<?php

/**
 * Background job to update links for a given title.
 *
 * @ingroup JobQueue
 */
class RefreshLinksJob extends Job {

	function __construct( $title, $params = '', $id = 0 ) {
		parent::__construct( 'refreshLinks', $title, $params, $id );
	}

	/**
	 * Run a refreshLinks job
	 * @return boolean success
	 */
	function run() {
		global $wgParser;
		wfProfileIn( __METHOD__ );

		$linkCache = LinkCache::singleton();
		$linkCache->clear();

		if ( is_null( $this->title ) ) {
			$this->error = "refreshLinks: Invalid title";
			wfProfileOut( __METHOD__ );
			return false;
		}

		$revision = Revision::newFromTitle( $this->title );
		if ( !$revision ) {
			$this->error = 'refreshLinks: Article not found "' . $this->title->getPrefixedDBkey() . '"';
			wfProfileOut( __METHOD__ );
			return false;
		}

		wfProfileIn( __METHOD__.'-parse' );
		$options = new ParserOptions;
		$parserOutput = $wgParser->parse( $revision->getText(), $this->title, $options, true, true, $revision->getId() );
		wfProfileOut( __METHOD__.'-parse' );
		wfProfileIn( __METHOD__.'-update' );
		$update = new LinksUpdate( $this->title, $parserOutput, false );
		$update->doUpdate();
		wfProfileOut( __METHOD__.'-update' );
		wfProfileOut( __METHOD__ );
		return true;
	}
}
