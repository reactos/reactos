<?php

/**
 * @ingroup Parser
 */
interface Preprocessor {
	/** Create a new preprocessor object based on an initialised Parser object */
	function __construct( $parser );

	/** Create a new top-level frame for expansion of a page */
	function newFrame();

	/** Create a new custom frame for programmatic use of parameter replacement as used in some extensions */
	function newCustomFrame( $args );

	/** Preprocess text to a PPNode */
	function preprocessToObj( $text, $flags = 0 );
}

/**
 * @ingroup Parser
 */
interface PPFrame {
	const NO_ARGS = 1;
	const NO_TEMPLATES = 2;
	const STRIP_COMMENTS = 4;
	const NO_IGNORE = 8;
	const RECOVER_COMMENTS = 16;

	const RECOVER_ORIG = 27; // = 1|2|8|16 no constant expression support in PHP yet

	/**
	 * Create a child frame
	 */
	function newChild( $args = false, $title = false );

	/**
	 * Expand a document tree node
	 */
	function expand( $root, $flags = 0 );

	/**
	 * Implode with flags for expand()
	 */
	function implodeWithFlags( $sep, $flags /*, ... */ );

	/**
	 * Implode with no flags specified
	 */
	function implode( $sep /*, ... */ );

	/**
	 * Makes an object that, when expand()ed, will be the same as one obtained
	 * with implode()
	 */
	function virtualImplode( $sep /*, ... */ );

	/**
	 * Virtual implode with brackets
	 */
	function virtualBracketedImplode( $start, $sep, $end /*, ... */ );

	/**
	 * Returns true if there are no arguments in this frame
	 */
	function isEmpty();

	/**
	 * Get an argument to this frame by name
	 */
	function getArgument( $name );

	/**
	 * Returns true if the infinite loop check is OK, false if a loop is detected
	 */
	function loopCheck( $title );

	/**
	 * Return true if the frame is a template frame
	 */
	function isTemplate();
}

/**
 * There are three types of nodes:
 *     * Tree nodes, which have a name and contain other nodes as children
 *     * Array nodes, which also contain other nodes but aren't considered part of a tree
 *     * Leaf nodes, which contain the actual data
 *
 * This interface provides access to the tree structure and to the contents of array nodes,
 * but it does not provide access to the internal structure of leaf nodes. Access to leaf
 * data is provided via two means:
 *     * PPFrame::expand(), which provides expanded text
 *     * The PPNode::split*() functions, which provide metadata about certain types of tree node
 * @ingroup Parser
 */
interface PPNode {
	/**
	 * Get an array-type node containing the children of this node.
	 * Returns false if this is not a tree node.
	 */
	function getChildren();

	/**
	 * Get the first child of a tree node. False if there isn't one.
	 */
	function getFirstChild();

	/**
	 * Get the next sibling of any node. False if there isn't one
	 */
	function getNextSibling();

	/**
	 * Get all children of this tree node which have a given name.
	 * Returns an array-type node, or false if this is not a tree node.
	 */
	function getChildrenOfType( $type );


	/**
	 * Returns the length of the array, or false if this is not an array-type node
	 */
	function getLength();

	/**
	 * Returns an item of an array-type node
	 */
	function item( $i );

	/**
	 * Get the name of this node. The following names are defined here:
	 *
	 *    h             A heading node.
	 *    template      A double-brace node.
	 *    tplarg        A triple-brace node.
	 *    title         The first argument to a template or tplarg node.
	 *    part          Subsequent arguments to a template or tplarg node.
	 *    #nodelist     An array-type node
	 *
	 * The subclass may define various other names for tree and leaf nodes.
	 */
	function getName();

	/**
	 * Split a <part> node into an associative array containing:
	 *    name          PPNode name
	 *    index         String index
	 *    value         PPNode value
	 */
	function splitArg();

	/**
	 * Split an <ext> node into an associative array containing name, attr, inner and close
	 * All values in the resulting array are PPNodes. Inner and close are optional.
	 */
	function splitExt();

	/**
	 * Split an <h> node
	 */
	function splitHeading();
}
