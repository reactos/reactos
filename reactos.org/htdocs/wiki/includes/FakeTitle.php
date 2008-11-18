<?php

/**
 * Fake title class that triggers an error if any members are called
 */
class FakeTitle {
	function error() { throw new MWException( "Attempt to call member function of FakeTitle\n" ); }

	// PHP 5.1 method overload
	function __call( $name, $args ) { $this->error(); }

	// PHP <5.1 compatibility
	function getInterwikiLink()  { $this->error(); }
	function getInterwikiCached() { $this->error(); }
	function isLocal() { $this->error(); }
	function isTrans() { $this->error(); }
	function getText() { $this->error(); }
	function getPartialURL() { $this->error(); }
	function getDBkey() { $this->error(); }
	function getNamespace() { $this->error(); }
	function getNsText() { $this->error(); }
	function getSubjectNsText() { $this->error(); }
	function getInterwiki() { $this->error(); }
	function getFragment() { $this->error(); }
	function getDefaultNamespace() { $this->error(); }
	function getIndexTitle() { $this->error(); }
	function getPrefixedDBkey() { $this->error(); }
	function getPrefixedText() { $this->error(); }
	function getFullText() { $this->error(); }
	function getPrefixedURL() { $this->error(); }
	function getFullURL() {$this->error(); }
	function getLocalURL() { $this->error(); }
	function escapeLocalURL() { $this->error(); }
	function escapeFullURL() { $this->error(); }
	function getInternalURL() { $this->error(); }
	function getEditURL() { $this->error(); }
	function getEscapedText() { $this->error(); }
	function isExternal() { $this->error(); }
	function isSemiProtected() { $this->error(); }
	function isProtected() { $this->error(); }
	function userIsWatching() { $this->error(); }
	function userCan() { $this->error(); }
	function userCanCreate() { $this->error(); }
	function userCanEdit() { $this->error(); }
	function userCanMove() { $this->error(); }
	function isMovable() { $this->error(); }
	function userCanRead() { $this->error(); }
	function isTalkPage() { $this->error(); }
	function isCssJsSubpage() { $this->error(); }
	function isValidCssJsSubpage() { $this->error(); }
	function getSkinFromCssJsSubpage() { $this->error(); }
	function isCssSubpage() { $this->error(); }
	function isJsSubpage() { $this->error(); }
	function userCanEditCssJsSubpage() { $this->error(); }
	function loadRestrictions( $res ) { $this->error(); }
	function getRestrictions($action) { $this->error(); }
	function isDeleted() { $this->error(); }
	function getArticleID( $flags = 0 ) { $this->error(); }
	function getLatestRevID() { $this->error(); }
	function resetArticleID( $newid ) { $this->error(); }
	function invalidateCache() { $this->error(); }
	function getTalkPage() { $this->error(); }
	function getSubjectPage() { $this->error(); }
	function getLinksTo() { $this->error(); }
	function getTemplateLinksTo() { $this->error(); }
	function getBrokenLinksFrom() { $this->error(); }
	function getSquidURLs() { $this->error(); }
	function moveNoAuth() { $this->error(); }
	function isValidMoveOperation() { $this->error(); }
	function moveTo() { $this->error(); }
	function moveOverExistingRedirect() { $this->error(); }
	function moveToNewTitle() { $this->error(); }
	function isValidMoveTarget() { $this->error(); }
	function getParentCategories() { $this->error(); }
	function getParentCategoryTree() { $this->error(); }
	function pageCond() { $this->error(); }
	function getPreviousRevisionID() { $this->error(); }
	function getNextRevisionID() { $this->error(); }
	function equals() { $this->error(); }
	function exists() { $this->error(); }
	function isAlwaysKnown() { $this->error(); }
	function touchLinks() { $this->error(); }
	function trackbackURL() { $this->error(); }
	function trackbackRDF() { $this->error(); }
}
