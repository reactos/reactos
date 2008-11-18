<?php
/**
 * @file
 * @ingroup SpecialPage
 */

function wfSpecialCategories( $par=null ) {
	global $wgOut, $wgRequest;

	if( $par == '' ) {
		$from = $wgRequest->getText( 'from' );
	} else {
		$from = $par;
	}
	$cap = new CategoryPager( $from );
	$wgOut->addHTML(
		wfMsgExt( 'categoriespagetext', array( 'parse' ) ) .
		$cap->getStartForm( $from ) .
		$cap->getNavigationBar() .
		'<ul>' . $cap->getBody() . '</ul>' .
		$cap->getNavigationBar()
	);
}

/**
 * TODO: Allow sorting by count.  We need to have a unique index to do this
 * properly.
 *
 * @ingroup SpecialPage Pager
 */
class CategoryPager extends AlphabeticPager {
	function __construct( $from ) {
		parent::__construct();
		$from = str_replace( ' ', '_', $from );
		if( $from !== '' ) {
			global $wgCapitalLinks, $wgContLang;
			if( $wgCapitalLinks ) {
				$from = $wgContLang->ucfirst( $from );
			}
			$this->mOffset = $from;
		}
	}
	
	function getQueryInfo() {
		global $wgRequest;
		return array(
			'tables' => array( 'category' ),
			'fields' => array( 'cat_title','cat_pages' ),
			'conds' => array( 'cat_pages > 0' ), 
			'options' => array( 'USE INDEX' => 'cat_title' ),
		);
	}

	function getIndexField() {
#		return array( 'abc' => 'cat_title', 'count' => 'cat_pages' );
		return 'cat_title';
	}

	function getDefaultQuery() {
		parent::getDefaultQuery();
		unset( $this->mDefaultQuery['from'] );
	}
#	protected function getOrderTypeMessages() {
#		return array( 'abc' => 'special-categories-sort-abc',
#			'count' => 'special-categories-sort-count' );
#	}

	protected function getDefaultDirections() {
#		return array( 'abc' => false, 'count' => true );
		return false;
	}

	/* Override getBody to apply LinksBatch on resultset before actually outputting anything. */
	public function getBody() {
		if (!$this->mQueryDone) {
			$this->doQuery();
		}
		$batch = new LinkBatch;

		$this->mResult->rewind();

		while ( $row = $this->mResult->fetchObject() ) {
			$batch->addObj( Title::makeTitleSafe( NS_CATEGORY, $row->cat_title ) );
		}
		$batch->execute();
		$this->mResult->rewind();
		return parent::getBody();
	}

	function formatRow($result) {
		global $wgLang;
		$title = Title::makeTitle( NS_CATEGORY, $result->cat_title );
		$titleText = $this->getSkin()->makeLinkObj( $title, htmlspecialchars( $title->getText() ) );
		$count = wfMsgExt( 'nmembers', array( 'parsemag', 'escape' ),
				$wgLang->formatNum( $result->cat_pages ) );
		return Xml::tags('li', null, "$titleText ($count)" ) . "\n";
	}
	
	public function getStartForm( $from ) {
		global $wgScript;
		$t = SpecialPage::getTitleFor( 'Categories' );
	
		return
			Xml::tags( 'form', array( 'method' => 'get', 'action' => $wgScript ),
				Xml::hidden( 'title', $t->getPrefixedText() ) .
				Xml::fieldset( wfMsg( 'categories' ),
					Xml::inputLabel( wfMsg( 'categoriesfrom' ),
						'from', 'from', 20, $from ) .
					' ' .
					Xml::submitButton( wfMsg( 'allpagessubmit' ) ) ) );
	}
}
