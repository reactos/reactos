<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * @todo document
 * @ingroup SpecialPage
 */
class ProtectedTitlesForm {

	protected $IdLevel = 'level';
	protected $IdType  = 'type';

	function showList( $msg = '' ) {
		global $wgOut, $wgRequest;

		$wgOut->setPagetitle( wfMsg( "protectedtitles" ) );
		if ( "" != $msg ) {
			$wgOut->setSubtitle( $msg );
		}

		// Purge expired entries on one in every 10 queries
		if ( !mt_rand( 0, 10 ) ) {
			Title::purgeExpiredRestrictions();
		}

		$type = $wgRequest->getVal( $this->IdType );
		$level = $wgRequest->getVal( $this->IdLevel );
		$sizetype = $wgRequest->getVal( 'sizetype' );
		$size = $wgRequest->getIntOrNull( 'size' );
		$NS = $wgRequest->getIntOrNull( 'namespace' );

		$pager = new ProtectedTitlesPager( $this, array(), $type, $level, $NS, $sizetype, $size );

		$wgOut->addHTML( $this->showOptions( $NS, $type, $level, $sizetype, $size ) );

		if ( $pager->getNumRows() ) {
			$s = $pager->getNavigationBar();
			$s .= "<ul>" .
				$pager->getBody() .
				"</ul>";
			$s .= $pager->getNavigationBar();
		} else {
			$s = '<p>' . wfMsgHtml( 'protectedtitlesempty' ) . '</p>';
		}
		$wgOut->addHTML( $s );
	}

	/**
	 * Callback function to output a restriction
	 */
	function formatRow( $row ) {
		global $wgUser, $wgLang, $wgContLang;

		wfProfileIn( __METHOD__ );

		static $skin=null;

		if( is_null( $skin ) )
			$skin = $wgUser->getSkin();

		$title = Title::makeTitleSafe( $row->pt_namespace, $row->pt_title );
		$link = $skin->makeLinkObj( $title );

		$description_items = array ();

		$protType = wfMsgHtml( 'restriction-level-' . $row->pt_create_perm );

		$description_items[] = $protType;

		$expiry_description = ''; $stxt = '';

		if ( $row->pt_expiry != 'infinity' && strlen($row->pt_expiry) ) {
			$expiry = Block::decodeExpiry( $row->pt_expiry );

			$expiry_description = wfMsgForContent( 'protect-expiring', $wgLang->timeanddate( $expiry ) );

			$description_items[] = $expiry_description;
		}

		wfProfileOut( __METHOD__ );

		return '<li>' . wfSpecialList( $link . $stxt, implode( $description_items, ', ' ) ) . "</li>\n";
	}

	/**
	 * @param $namespace int
	 * @param $type string
	 * @param $level string
	 * @param $minsize int
	 * @private
	 */
	function showOptions( $namespace, $type='edit', $level, $sizetype, $size ) {
		global $wgScript;
		$action = htmlspecialchars( $wgScript );
		$title = SpecialPage::getTitleFor( 'ProtectedTitles' );
		$special = htmlspecialchars( $title->getPrefixedDBkey() );
		return "<form action=\"$action\" method=\"get\">\n" .
			'<fieldset>' .
			Xml::element( 'legend', array(), wfMsg( 'protectedtitles' ) ) .
			Xml::hidden( 'title', $special ) . "&nbsp;\n" .
			$this->getNamespaceMenu( $namespace ) . "&nbsp;\n" .
			// $this->getLevelMenu( $level ) . "<br/>\n" .
			"&nbsp;" . Xml::submitButton( wfMsg( 'allpagessubmit' ) ) . "\n" .
			"</fieldset></form>";
	}

	/**
	 * Prepare the namespace filter drop-down; standard namespace
	 * selector, sans the MediaWiki namespace
	 *
	 * @param mixed $namespace Pre-select namespace
	 * @return string
	 */
	function getNamespaceMenu( $namespace = null ) {
		return Xml::label( wfMsg( 'namespace' ), 'namespace' )
			. '&nbsp;'
			. Xml::namespaceSelector( $namespace, '' );
	}

	/**
	 * @return string Formatted HTML
	 * @private
	 */
	function getLevelMenu( $pr_level ) {
		global $wgRestrictionLevels;

		$m = array( wfMsg('restriction-level-all') => 0 ); // Temporary array
		$options = array();

		// First pass to load the log names
		foreach( $wgRestrictionLevels as $type ) {
			if ( $type !='' && $type !='*') {
				$text = wfMsg("restriction-level-$type");
				$m[$text] = $type;
			}
		}

		// Third pass generates sorted XHTML content
		foreach( $m as $text => $type ) {
			$selected = ($type == $pr_level );
			$options[] = Xml::option( $text, $type, $selected );
		}

		return
			Xml::label( wfMsg('restriction-level') , $this->IdLevel ) . '&nbsp;' .
			Xml::tags( 'select',
				array( 'id' => $this->IdLevel, 'name' => $this->IdLevel ),
				implode( "\n", $options ) );
	}
}

/**
 * @todo document
 * @ingroup Pager
 */
class ProtectedtitlesPager extends AlphabeticPager {
	public $mForm, $mConds;

	function __construct( $form, $conds = array(), $type, $level, $namespace, $sizetype='', $size=0 ) {
		$this->mForm = $form;
		$this->mConds = $conds;
		$this->level = $level;
		$this->namespace = $namespace;
		$this->size = intval($size);
		parent::__construct();
	}

	function getStartBody() {
		wfProfileIn( __METHOD__ );
		# Do a link batch query
		$this->mResult->seek( 0 );
		$lb = new LinkBatch;

		while ( $row = $this->mResult->fetchObject() ) {
			$lb->add( $row->pt_namespace, $row->pt_title );
		}

		$lb->execute();
		wfProfileOut( __METHOD__ );
		return '';
	}

	function formatRow( $row ) {
		return $this->mForm->formatRow( $row );
	}

	function getQueryInfo() {
		$conds = $this->mConds;
		$conds[] = 'pt_expiry>' . $this->mDb->addQuotes( $this->mDb->timestamp() );

		if( !is_null($this->namespace) )
			$conds[] = 'pt_namespace=' . $this->mDb->addQuotes( $this->namespace );
		return array(
			'tables' => 'protected_titles',
			'fields' => 'pt_namespace,pt_title,pt_create_perm,pt_expiry,pt_timestamp',
			'conds' => $conds
		);
	}

	function getIndexField() {
		return 'pt_timestamp';
	}
}

/**
 * Constructor
 */
function wfSpecialProtectedtitles() {

	$ppForm = new ProtectedTitlesForm();

	$ppForm->showList();
}
