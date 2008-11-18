<?php
/**
 * See deferred.txt
 * @ingroup Search
 */
class SearchUpdate {

	/* private */ var $mId = 0, $mNamespace, $mTitle, $mText;
	/* private */ var $mTitleWords;

	function SearchUpdate( $id, $title, $text = false ) {
		$nt = Title::newFromText( $title );
		if( $nt ) {
			$this->mId = $id;
			$this->mText = $text;

			$this->mNamespace = $nt->getNamespace();
			$this->mTitle = $nt->getText(); # Discard namespace

			$this->mTitleWords = $this->mTextWords = array();
		} else {
			wfDebug( "SearchUpdate object created with invalid title '$title'\n" );
		}
	}

	function doUpdate() {
		global $wgContLang, $wgDisableSearchUpdate;

		if( $wgDisableSearchUpdate || !$this->mId ) {
			return false;
		}
		$fname = 'SearchUpdate::doUpdate';
		wfProfileIn( $fname );

		$search = SearchEngine::create();
		$lc = SearchEngine::legalSearchChars() . '&#;';

		if( $this->mText === false ) {
			$search->updateTitle($this->mId,
				Title::indexTitle( $this->mNamespace, $this->mTitle ));
			wfProfileOut( $fname );
			return;
		}

		# Language-specific strip/conversion
		$text = $wgContLang->stripForSearch( $this->mText );

		wfProfileIn( $fname.'-regexps' );
		$text = preg_replace( "/<\\/?\\s*[A-Za-z][A-Za-z0-9]*\\s*([^>]*?)>/",
		  ' ', strtolower( " " . $text /*$this->mText*/ . " " ) ); # Strip HTML markup
		$text = preg_replace( "/(^|\\n)==\\s*([^\\n]+)\\s*==(\\s)/sD",
		  "\\1\\2 \\2 \\2\\3", $text ); # Emphasize headings

		# Strip external URLs
		$uc = "A-Za-z0-9_\\/:.,~%\\-+&;#?!=()@\\xA0-\\xFF";
		$protos = "http|https|ftp|mailto|news|gopher";
		$pat = "/(^|[^\\[])({$protos}):[{$uc}]+([^{$uc}]|$)/";
		$text = preg_replace( $pat, "\\1 \\3", $text );

		$p1 = "/([^\\[])\\[({$protos}):[{$uc}]+]/";
		$p2 = "/([^\\[])\\[({$protos}):[{$uc}]+\\s+([^\\]]+)]/";
		$text = preg_replace( $p1, "\\1 ", $text );
		$text = preg_replace( $p2, "\\1 \\3 ", $text );

		# Internal image links
		$pat2 = "/\\[\\[image:([{$uc}]+)\\.(gif|png|jpg|jpeg)([^{$uc}])/i";
		$text = preg_replace( $pat2, " \\1 \\3", $text );

		$text = preg_replace( "/([^{$lc}])([{$lc}]+)]]([a-z]+)/",
		  "\\1\\2 \\2\\3", $text ); # Handle [[game]]s

		# Strip all remaining non-search characters
		$text = preg_replace( "/[^{$lc}]+/", " ", $text );

		# Handle 's, s'
		#
		#   $text = preg_replace( "/([{$lc}]+)'s /", "\\1 \\1's ", $text );
		#   $text = preg_replace( "/([{$lc}]+)s' /", "\\1s ", $text );
		#
		# These tail-anchored regexps are insanely slow. The worst case comes
		# when Japanese or Chinese text (ie, no word spacing) is written on
		# a wiki configured for Western UTF-8 mode. The Unicode characters are
		# expanded to hex codes and the "words" are very long paragraph-length
		# monstrosities. On a large page the above regexps may take over 20
		# seconds *each* on a 1GHz-level processor.
		#
		# Following are reversed versions which are consistently fast
		# (about 3 milliseconds on 1GHz-level processor).
		#
		$text = strrev( preg_replace( "/ s'([{$lc}]+)/", " s'\\1 \\1", strrev( $text ) ) );
		$text = strrev( preg_replace( "/ 's([{$lc}]+)/", " s\\1", strrev( $text ) ) );

		# Strip wiki '' and '''
		$text = preg_replace( "/''[']*/", " ", $text );
		wfProfileOut( "$fname-regexps" );

		wfRunHooks( 'SearchUpdate', array( $this->mId, $this->mNamespace, $this->mTitle, &$text ) );

		# Perform the actual update
		$search->update($this->mId, Title::indexTitle( $this->mNamespace, $this->mTitle ),
				$text);

		wfProfileOut( $fname );
	}
}

/**
 * Placeholder class
 * @ingroup Search
 */
class SearchUpdateMyISAM extends SearchUpdate {
	# Inherits everything
}
