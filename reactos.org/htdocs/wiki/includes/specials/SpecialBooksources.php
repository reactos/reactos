<?php

/**
 * Special page outputs information on sourcing a book with a particular ISBN
 * The parser creates links to this page when dealing with ISBNs in wikitext
 *
 * @author Rob Church <robchur@gmail.com>
 * @todo Validate ISBNs using the standard check-digit method
 * @ingroup SpecialPages
 */
class SpecialBookSources extends SpecialPage {

	/**
	 * ISBN passed to the page, if any
	 */
	private $isbn = '';

	/**
	 * Constructor
	 */
	public function __construct() {
		parent::__construct( 'Booksources' );
	}

	/**
	 * Show the special page
	 *
	 * @param $isbn ISBN passed as a subpage parameter
	 */
	public function execute( $isbn ) {
		global $wgOut, $wgRequest;
		$this->setHeaders();
		$this->isbn = $this->cleanIsbn( $isbn ? $isbn : $wgRequest->getText( 'isbn' ) );
		$wgOut->addWikiMsg( 'booksources-summary' );
		$wgOut->addHtml( $this->makeForm() );
		if( strlen( $this->isbn ) > 0 )
			$this->showList();
	}

	/**
	 * Trim ISBN and remove characters which aren't required
	 *
	 * @param $isbn Unclean ISBN
	 * @return string
	 */
	private function cleanIsbn( $isbn ) {
		return trim( preg_replace( '![^0-9X]!', '', $isbn ) );
	}

	/**
	 * Generate a form to allow users to enter an ISBN
	 *
	 * @return string
	 */
	private function makeForm() {
		global $wgScript;
		$title = self::getTitleFor( 'Booksources' );
		$form  = '<fieldset><legend>' . wfMsgHtml( 'booksources-search-legend' ) . '</legend>';
		$form .= Xml::openElement( 'form', array( 'method' => 'get', 'action' => $wgScript ) );
		$form .= Xml::hidden( 'title', $title->getPrefixedText() );
		$form .= '<p>' . Xml::inputLabel( wfMsg( 'booksources-isbn' ), 'isbn', 'isbn', 20, $this->isbn );
		$form .= '&nbsp;' . Xml::submitButton( wfMsg( 'booksources-go' ) ) . '</p>';
		$form .= Xml::closeElement( 'form' );
		$form .= '</fieldset>';
		return $form;
	}

	/**
	 * Determine where to get the list of book sources from,
	 * format and output them
	 *
	 * @return string
	 */
	private function showList() {
		global $wgOut, $wgContLang;

		# Hook to allow extensions to insert additional HTML,
		# e.g. for API-interacting plugins and so on
		wfRunHooks( 'BookInformation', array( $this->isbn, &$wgOut ) );

		# Check for a local page such as Project:Book_sources and use that if available
		$title = Title::makeTitleSafe( NS_PROJECT, wfMsgForContent( 'booksources' ) ); # Show list in content language
		if( is_object( $title ) && $title->exists() ) {
			$rev = Revision::newFromTitle( $title );
			$wgOut->addWikiText( str_replace( 'MAGICNUMBER', $this->isbn, $rev->getText() ) );
			return true;
		}

		# Fall back to the defaults given in the language file
		$wgOut->addWikiMsg( 'booksources-text' );
		$wgOut->addHtml( '<ul>' );
		$items = $wgContLang->getBookstoreList();
		foreach( $items as $label => $url )
			$wgOut->addHtml( $this->makeListItem( $label, $url ) );
		$wgOut->addHtml( '</ul>' );
		return true;
	}

	/**
	 * Format a book source list item
	 *
	 * @param $label Book source label
	 * @param $url Book source URL
	 * @return string
	 */
	private function makeListItem( $label, $url ) {
		$url = str_replace( '$1', $this->isbn, $url );
		return '<li><a href="' . htmlspecialchars( $url ) . '">' . htmlspecialchars( $label ) . '</a></li>';
	}
}
