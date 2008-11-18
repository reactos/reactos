<?php

/**
 * Differences from DOM schema:
 *   * attribute nodes are children
 *   * <h> nodes that aren't at the top are replaced with <possible-h>
 * @ingroup Parser
 */
class Preprocessor_Hash implements Preprocessor {
	var $parser;

	function __construct( $parser ) {
		$this->parser = $parser;
	}

	function newFrame() {
		return new PPFrame_Hash( $this );
	}

	function newCustomFrame( $args ) {
		return new PPCustomFrame_Hash( $this, $args );
	}

	/**
	 * Preprocess some wikitext and return the document tree.
	 * This is the ghost of Parser::replace_variables().
	 *
	 * @param string $text The text to parse
	 * @param integer flags Bitwise combination of:
	 *          Parser::PTD_FOR_INCLUSION    Handle <noinclude>/<includeonly> as if the text is being
	 *                                     included. Default is to assume a direct page view.
	 *
	 * The generated DOM tree must depend only on the input text and the flags.
	 * The DOM tree must be the same in OT_HTML and OT_WIKI mode, to avoid a regression of bug 4899.
	 *
	 * Any flag added to the $flags parameter here, or any other parameter liable to cause a
	 * change in the DOM tree for a given text, must be passed through the section identifier
	 * in the section edit link and thus back to extractSections().
	 *
	 * The output of this function is currently only cached in process memory, but a persistent
	 * cache may be implemented at a later date which takes further advantage of these strict
	 * dependency requirements.
	 *
	 * @private
	 */
	function preprocessToObj( $text, $flags = 0 ) {
		wfProfileIn( __METHOD__ );

		$rules = array(
			'{' => array(
				'end' => '}',
				'names' => array(
					2 => 'template',
					3 => 'tplarg',
				),
				'min' => 2,
				'max' => 3,
			),
			'[' => array(
				'end' => ']',
				'names' => array( 2 => null ),
				'min' => 2,
				'max' => 2,
			)
		);

		$forInclusion = $flags & Parser::PTD_FOR_INCLUSION;

		$xmlishElements = $this->parser->getStripList();
		$enableOnlyinclude = false;
		if ( $forInclusion ) {
			$ignoredTags = array( 'includeonly', '/includeonly' );
			$ignoredElements = array( 'noinclude' );
			$xmlishElements[] = 'noinclude';
			if ( strpos( $text, '<onlyinclude>' ) !== false && strpos( $text, '</onlyinclude>' ) !== false ) {
				$enableOnlyinclude = true;
			}
		} else {
			$ignoredTags = array( 'noinclude', '/noinclude', 'onlyinclude', '/onlyinclude' );
			$ignoredElements = array( 'includeonly' );
			$xmlishElements[] = 'includeonly';
		}
		$xmlishRegex = implode( '|', array_merge( $xmlishElements, $ignoredTags ) );

		// Use "A" modifier (anchored) instead of "^", because ^ doesn't work with an offset
		$elementsRegex = "~($xmlishRegex)(?:\s|\/>|>)|(!--)~iA";

		$stack = new PPDStack_Hash;

		$searchBase = "[{<\n";
		$revText = strrev( $text ); // For fast reverse searches

		$i = 0;                     # Input pointer, starts out pointing to a pseudo-newline before the start
		$accum =& $stack->getAccum();   # Current accumulator
		$findEquals = false;            # True to find equals signs in arguments
		$findPipe = false;              # True to take notice of pipe characters
		$headingIndex = 1;
		$inHeading = false;        # True if $i is inside a possible heading
		$noMoreGT = false;         # True if there are no more greater-than (>) signs right of $i
		$findOnlyinclude = $enableOnlyinclude; # True to ignore all input up to the next <onlyinclude>
		$fakeLineStart = true;     # Do a line-start run without outputting an LF character

		while ( true ) {
			//$this->memCheck();

			if ( $findOnlyinclude ) {
				// Ignore all input up to the next <onlyinclude>
				$startPos = strpos( $text, '<onlyinclude>', $i );
				if ( $startPos === false ) {
					// Ignored section runs to the end
					$accum->addNodeWithText( 'ignore', substr( $text, $i ) );
					break;
				}
				$tagEndPos = $startPos + strlen( '<onlyinclude>' ); // past-the-end
				$accum->addNodeWithText( 'ignore', substr( $text, $i, $tagEndPos - $i ) );
				$i = $tagEndPos;
				$findOnlyinclude = false;
			}

			if ( $fakeLineStart ) {
				$found = 'line-start';
				$curChar = '';
			} else {
				# Find next opening brace, closing brace or pipe
				$search = $searchBase;
				if ( $stack->top === false ) {
					$currentClosing = '';
				} else {
					$currentClosing = $stack->top->close;
					$search .= $currentClosing;
				}
				if ( $findPipe ) {
					$search .= '|';
				}
				if ( $findEquals ) {
					// First equals will be for the template
					$search .= '=';
				}
				$rule = null;
				# Output literal section, advance input counter
				$literalLength = strcspn( $text, $search, $i );
				if ( $literalLength > 0 ) {
					$accum->addLiteral( substr( $text, $i, $literalLength ) );
					$i += $literalLength;
				}
				if ( $i >= strlen( $text ) ) {
					if ( $currentClosing == "\n" ) {
						// Do a past-the-end run to finish off the heading
						$curChar = '';
						$found = 'line-end';
					} else {
						# All done
						break;
					}
				} else {
					$curChar = $text[$i];
					if ( $curChar == '|' ) {
						$found = 'pipe';
					} elseif ( $curChar == '=' ) {
						$found = 'equals';
					} elseif ( $curChar == '<' ) {
						$found = 'angle';
					} elseif ( $curChar == "\n" ) {
						if ( $inHeading ) {
							$found = 'line-end';
						} else {
							$found = 'line-start';
						}
					} elseif ( $curChar == $currentClosing ) {
						$found = 'close';
					} elseif ( isset( $rules[$curChar] ) ) {
						$found = 'open';
						$rule = $rules[$curChar];
					} else {
						# Some versions of PHP have a strcspn which stops on null characters
						# Ignore and continue
						++$i;
						continue;
					}
				}
			}

			if ( $found == 'angle' ) {
				$matches = false;
				// Handle </onlyinclude>
				if ( $enableOnlyinclude && substr( $text, $i, strlen( '</onlyinclude>' ) ) == '</onlyinclude>' ) {
					$findOnlyinclude = true;
					continue;
				}

				// Determine element name
				if ( !preg_match( $elementsRegex, $text, $matches, 0, $i + 1 ) ) {
					// Element name missing or not listed
					$accum->addLiteral( '<' );
					++$i;
					continue;
				}
				// Handle comments
				if ( isset( $matches[2] ) && $matches[2] == '!--' ) {
					// To avoid leaving blank lines, when a comment is both preceded
					// and followed by a newline (ignoring spaces), trim leading and
					// trailing spaces and one of the newlines.

					// Find the end
					$endPos = strpos( $text, '-->', $i + 4 );
					if ( $endPos === false ) {
						// Unclosed comment in input, runs to end
						$inner = substr( $text, $i );
						$accum->addNodeWithText( 'comment', $inner );
						$i = strlen( $text );
					} else {
						// Search backwards for leading whitespace
						$wsStart = $i ? ( $i - strspn( $revText, ' ', strlen( $text ) - $i ) ) : 0;
						// Search forwards for trailing whitespace
						// $wsEnd will be the position of the last space
						$wsEnd = $endPos + 2 + strspn( $text, ' ', $endPos + 3 );
						// Eat the line if possible
						// TODO: This could theoretically be done if $wsStart == 0, i.e. for comments at
						// the overall start. That's not how Sanitizer::removeHTMLcomments() did it, but
						// it's a possible beneficial b/c break.
						if ( $wsStart > 0 && substr( $text, $wsStart - 1, 1 ) == "\n"
							&& substr( $text, $wsEnd + 1, 1 ) == "\n" )
						{
							$startPos = $wsStart;
							$endPos = $wsEnd + 1;
							// Remove leading whitespace from the end of the accumulator
							// Sanity check first though
							$wsLength = $i - $wsStart;
							if ( $wsLength > 0
								&& $accum->lastNode instanceof PPNode_Hash_Text
								&& substr( $accum->lastNode->value, -$wsLength ) === str_repeat( ' ', $wsLength ) )
							{
								$accum->lastNode->value = substr( $accum->lastNode->value, 0, -$wsLength );
							}
							// Do a line-start run next time to look for headings after the comment
							$fakeLineStart = true;
						} else {
							// No line to eat, just take the comment itself
							$startPos = $i;
							$endPos += 2;
						}

						if ( $stack->top ) {
							$part = $stack->top->getCurrentPart();
							if ( isset( $part->commentEnd ) && $part->commentEnd == $wsStart - 1 ) {
								// Comments abutting, no change in visual end
								$part->commentEnd = $wsEnd;
							} else {
								$part->visualEnd = $wsStart;
								$part->commentEnd = $endPos;
							}
						}
						$i = $endPos + 1;
						$inner = substr( $text, $startPos, $endPos - $startPos + 1 );
						$accum->addNodeWithText( 'comment', $inner );
					}
					continue;
				}
				$name = $matches[1];
				$lowerName = strtolower( $name );
				$attrStart = $i + strlen( $name ) + 1;

				// Find end of tag
				$tagEndPos = $noMoreGT ? false : strpos( $text, '>', $attrStart );
				if ( $tagEndPos === false ) {
					// Infinite backtrack
					// Disable tag search to prevent worst-case O(N^2) performance
					$noMoreGT = true;
					$accum->addLiteral( '<' );
					++$i;
					continue;
				}

				// Handle ignored tags
				if ( in_array( $lowerName, $ignoredTags ) ) {
					$accum->addNodeWithText( 'ignore', substr( $text, $i, $tagEndPos - $i + 1 ) );
					$i = $tagEndPos + 1;
					continue;
				}

				$tagStartPos = $i;
				if ( $text[$tagEndPos-1] == '/' ) {
					// Short end tag
					$attrEnd = $tagEndPos - 1;
					$inner = null;
					$i = $tagEndPos + 1;
					$close = null;
				} else {
					$attrEnd = $tagEndPos;
					// Find closing tag
					if ( preg_match( "/<\/$name\s*>/i", $text, $matches, PREG_OFFSET_CAPTURE, $tagEndPos + 1 ) ) {
						$inner = substr( $text, $tagEndPos + 1, $matches[0][1] - $tagEndPos - 1 );
						$i = $matches[0][1] + strlen( $matches[0][0] );
						$close = $matches[0][0];
					} else {
						// No end tag -- let it run out to the end of the text.
						$inner = substr( $text, $tagEndPos + 1 );
						$i = strlen( $text );
						$close = null;
					}
				}
				// <includeonly> and <noinclude> just become <ignore> tags
				if ( in_array( $lowerName, $ignoredElements ) ) {
					$accum->addNodeWithText(  'ignore', substr( $text, $tagStartPos, $i - $tagStartPos ) );
					continue;
				}

				if ( $attrEnd <= $attrStart ) {
					$attr = '';
				} else {
					// Note that the attr element contains the whitespace between name and attribute,
					// this is necessary for precise reconstruction during pre-save transform.
					$attr = substr( $text, $attrStart, $attrEnd - $attrStart );
				}

				$extNode = new PPNode_Hash_Tree( 'ext' );
				$extNode->addChild( PPNode_Hash_Tree::newWithText( 'name', $name ) );
				$extNode->addChild( PPNode_Hash_Tree::newWithText( 'attr', $attr ) );
				if ( $inner !== null ) {
					$extNode->addChild( PPNode_Hash_Tree::newWithText( 'inner', $inner ) );
				}
				if ( $close !== null ) {
					$extNode->addChild( PPNode_Hash_Tree::newWithText( 'close', $close ) );
				}
				$accum->addNode( $extNode );
			}

			elseif ( $found == 'line-start' ) {
				// Is this the start of a heading?
				// Line break belongs before the heading element in any case
				if ( $fakeLineStart ) {
					$fakeLineStart = false;
				} else {
					$accum->addLiteral( $curChar );
					$i++;
				}

				$count = strspn( $text, '=', $i, 6 );
				if ( $count == 1 && $findEquals ) {
					// DWIM: This looks kind of like a name/value separator
					// Let's let the equals handler have it and break the potential heading
					// This is heuristic, but AFAICT the methods for completely correct disambiguation are very complex.
				} elseif ( $count > 0 ) {
					$piece = array(
						'open' => "\n",
						'close' => "\n",
						'parts' => array( new PPDPart_Hash( str_repeat( '=', $count ) ) ),
						'startPos' => $i,
						'count' => $count );
					$stack->push( $piece );
					$accum =& $stack->getAccum();
					extract( $stack->getFlags() );
					$i += $count;
				}
			}

			elseif ( $found == 'line-end' ) {
				$piece = $stack->top;
				// A heading must be open, otherwise \n wouldn't have been in the search list
				assert( $piece->open == "\n" );
				$part = $piece->getCurrentPart();
				// Search back through the input to see if it has a proper close
				// Do this using the reversed string since the other solutions (end anchor, etc.) are inefficient
				$wsLength = strspn( $revText, " \t", strlen( $text ) - $i );
				$searchStart = $i - $wsLength;
				if ( isset( $part->commentEnd ) && $searchStart - 1 == $part->commentEnd ) {
					// Comment found at line end
					// Search for equals signs before the comment
					$searchStart = $part->visualEnd;
					$searchStart -= strspn( $revText, " \t", strlen( $text ) - $searchStart );
				}
				$count = $piece->count;
				$equalsLength = strspn( $revText, '=', strlen( $text ) - $searchStart );
				if ( $equalsLength > 0 ) {
					if ( $i - $equalsLength == $piece->startPos ) {
						// This is just a single string of equals signs on its own line
						// Replicate the doHeadings behaviour /={count}(.+)={count}/
						// First find out how many equals signs there really are (don't stop at 6)
						$count = $equalsLength;
						if ( $count < 3 ) {
							$count = 0;
						} else {
							$count = min( 6, intval( ( $count - 1 ) / 2 ) );
						}
					} else {
						$count = min( $equalsLength, $count );
					}
					if ( $count > 0 ) {
						// Normal match, output <h>
						$element = new PPNode_Hash_Tree( 'possible-h' );
						$element->addChild( new PPNode_Hash_Attr( 'level', $count ) );
						$element->addChild( new PPNode_Hash_Attr( 'i', $headingIndex++ ) );
						$element->lastChild->nextSibling = $accum->firstNode;
						$element->lastChild = $accum->lastNode;
					} else {
						// Single equals sign on its own line, count=0
						$element = $accum;
					}
				} else {
					// No match, no <h>, just pass down the inner text
					$element = $accum;
				}
				// Unwind the stack
				$stack->pop();
				$accum =& $stack->getAccum();
				extract( $stack->getFlags() );

				// Append the result to the enclosing accumulator
				if ( $element instanceof PPNode ) {
					$accum->addNode( $element );
				} else {
					$accum->addAccum( $element );
				}
				// Note that we do NOT increment the input pointer.
				// This is because the closing linebreak could be the opening linebreak of
				// another heading. Infinite loops are avoided because the next iteration MUST
				// hit the heading open case above, which unconditionally increments the
				// input pointer.
			}

			elseif ( $found == 'open' ) {
				# count opening brace characters
				$count = strspn( $text, $curChar, $i );

				# we need to add to stack only if opening brace count is enough for one of the rules
				if ( $count >= $rule['min'] ) {
					# Add it to the stack
					$piece = array(
						'open' => $curChar,
						'close' => $rule['end'],
						'count' => $count,
						'lineStart' => ($i > 0 && $text[$i-1] == "\n"),
					);

					$stack->push( $piece );
					$accum =& $stack->getAccum();
					extract( $stack->getFlags() );
				} else {
					# Add literal brace(s)
					$accum->addLiteral( str_repeat( $curChar, $count ) );
				}
				$i += $count;
			}

			elseif ( $found == 'close' ) {
				$piece = $stack->top;
				# lets check if there are enough characters for closing brace
				$maxCount = $piece->count;
				$count = strspn( $text, $curChar, $i, $maxCount );

				# check for maximum matching characters (if there are 5 closing
				# characters, we will probably need only 3 - depending on the rules)
				$matchingCount = 0;
				$rule = $rules[$piece->open];
				if ( $count > $rule['max'] ) {
					# The specified maximum exists in the callback array, unless the caller
					# has made an error
					$matchingCount = $rule['max'];
				} else {
					# Count is less than the maximum
					# Skip any gaps in the callback array to find the true largest match
					# Need to use array_key_exists not isset because the callback can be null
					$matchingCount = $count;
					while ( $matchingCount > 0 && !array_key_exists( $matchingCount, $rule['names'] ) ) {
						--$matchingCount;
					}
				}

				if ($matchingCount <= 0) {
					# No matching element found in callback array
					# Output a literal closing brace and continue
					$accum->addLiteral( str_repeat( $curChar, $count ) );
					$i += $count;
					continue;
				}
				$name = $rule['names'][$matchingCount];
				if ( $name === null ) {
					// No element, just literal text
					$element = $piece->breakSyntax( $matchingCount );
					$element->addLiteral( str_repeat( $rule['end'], $matchingCount ) );
				} else {
					# Create XML element
					# Note: $parts is already XML, does not need to be encoded further
					$parts = $piece->parts;
					$titleAccum = $parts[0]->out;
					unset( $parts[0] );

					$element = new PPNode_Hash_Tree( $name );

					# The invocation is at the start of the line if lineStart is set in
					# the stack, and all opening brackets are used up.
					if ( $maxCount == $matchingCount && !empty( $piece->lineStart ) ) {
						$element->addChild( new PPNode_Hash_Attr( 'lineStart', 1 ) );
					}
					$titleNode = new PPNode_Hash_Tree( 'title' );
					$titleNode->firstChild = $titleAccum->firstNode;
					$titleNode->lastChild = $titleAccum->lastNode;
					$element->addChild( $titleNode );
					$argIndex = 1;
					foreach ( $parts as $partIndex => $part ) {
						if ( isset( $part->eqpos ) ) {
							// Find equals
							$lastNode = false;
							for ( $node = $part->out->firstNode; $node; $node = $node->nextSibling ) {
								if ( $node === $part->eqpos ) {
									break;
								}
								$lastNode = $node;
							}
							if ( !$node ) {
								throw new MWException( __METHOD__. ': eqpos not found' );
							}
							if ( $node->name !== 'equals' ) {
								throw new MWException( __METHOD__ .': eqpos is not equals' );
							}
							$equalsNode = $node;

							// Construct name node
							$nameNode = new PPNode_Hash_Tree( 'name' );
							if ( $lastNode !== false ) {
								$lastNode->nextSibling = false;
								$nameNode->firstChild = $part->out->firstNode;
								$nameNode->lastChild = $lastNode;
							}

							// Construct value node
							$valueNode = new PPNode_Hash_Tree( 'value' );
							if ( $equalsNode->nextSibling !== false ) {
								$valueNode->firstChild = $equalsNode->nextSibling;
								$valueNode->lastChild = $part->out->lastNode;
							}
							$partNode = new PPNode_Hash_Tree( 'part' );
							$partNode->addChild( $nameNode );
							$partNode->addChild( $equalsNode->firstChild );
							$partNode->addChild( $valueNode );
							$element->addChild( $partNode );
						} else {
							$partNode = new PPNode_Hash_Tree( 'part' );
							$nameNode = new PPNode_Hash_Tree( 'name' );
							$nameNode->addChild( new PPNode_Hash_Attr( 'index', $argIndex++ ) );
							$valueNode = new PPNode_Hash_Tree( 'value' );
							$valueNode->firstChild = $part->out->firstNode;
							$valueNode->lastChild = $part->out->lastNode;
							$partNode->addChild( $nameNode );
							$partNode->addChild( $valueNode );
							$element->addChild( $partNode );
						}
					}
				}

				# Advance input pointer
				$i += $matchingCount;

				# Unwind the stack
				$stack->pop();
				$accum =& $stack->getAccum();

				# Re-add the old stack element if it still has unmatched opening characters remaining
				if ($matchingCount < $piece->count) {
					$piece->parts = array( new PPDPart_Hash );
					$piece->count -= $matchingCount;
					# do we still qualify for any callback with remaining count?
					$names = $rules[$piece->open]['names'];
					$skippedBraces = 0;
					$enclosingAccum =& $accum;
					while ( $piece->count ) {
						if ( array_key_exists( $piece->count, $names ) ) {
							$stack->push( $piece );
							$accum =& $stack->getAccum();
							break;
						}
						--$piece->count;
						$skippedBraces ++;
					}
					$enclosingAccum->addLiteral( str_repeat( $piece->open, $skippedBraces ) );
				}

				extract( $stack->getFlags() );

				# Add XML element to the enclosing accumulator
				if ( $element instanceof PPNode ) {
					$accum->addNode( $element );
				} else {
					$accum->addAccum( $element );
				}
			}

			elseif ( $found == 'pipe' ) {
				$findEquals = true; // shortcut for getFlags()
				$stack->addPart();
				$accum =& $stack->getAccum();
				++$i;
			}

			elseif ( $found == 'equals' ) {
				$findEquals = false; // shortcut for getFlags()
				$accum->addNodeWithText( 'equals', '=' );
				$stack->getCurrentPart()->eqpos = $accum->lastNode;
				++$i;
			}
		}

		# Output any remaining unclosed brackets
		foreach ( $stack->stack as $piece ) {
			$stack->rootAccum->addAccum( $piece->breakSyntax() );
		}

		# Enable top-level headings
		for ( $node = $stack->rootAccum->firstNode; $node; $node = $node->nextSibling ) {
			if ( isset( $node->name ) && $node->name === 'possible-h' ) {
				$node->name = 'h';
			}
		}

		$rootNode = new PPNode_Hash_Tree( 'root' );
		$rootNode->firstChild = $stack->rootAccum->firstNode;
		$rootNode->lastChild = $stack->rootAccum->lastNode;
		wfProfileOut( __METHOD__ );
		return $rootNode;
	}
}

/**
 * Stack class to help Preprocessor::preprocessToObj()
 * @ingroup Parser
 */
class PPDStack_Hash extends PPDStack {
	function __construct() {
		$this->elementClass = 'PPDStackElement_Hash';
		parent::__construct();
		$this->rootAccum = new PPDAccum_Hash;
	}
}

/**
 * @ingroup Parser
 */
class PPDStackElement_Hash extends PPDStackElement {
	function __construct( $data = array() ) {
		$this->partClass = 'PPDPart_Hash';
		parent::__construct( $data );
	}

	/**
	 * Get the accumulator that would result if the close is not found.
	 */
	function breakSyntax( $openingCount = false ) {
		if ( $this->open == "\n" ) {
			$accum = $this->parts[0]->out;
		} else {
			if ( $openingCount === false ) {
				$openingCount = $this->count;
			}
			$accum = new PPDAccum_Hash;
			$accum->addLiteral( str_repeat( $this->open, $openingCount ) );
			$first = true;
			foreach ( $this->parts as $part ) {
				if ( $first ) {
					$first = false;
				} else {
					$accum->addLiteral( '|' );
				}
				$accum->addAccum( $part->out );
			}
		}
		return $accum;
	}
}

/**
 * @ingroup Parser
 */
class PPDPart_Hash extends PPDPart {
	function __construct( $out = '' ) {
		$accum = new PPDAccum_Hash;
		if ( $out !== '' ) {
			$accum->addLiteral( $out );
		}
		parent::__construct( $accum );
	}
}

/**
 * @ingroup Parser
 */
class PPDAccum_Hash {
	var $firstNode, $lastNode;

	function __construct() {
		$this->firstNode = $this->lastNode = false;
	}

	/**
	 * Append a string literal
	 */
	function addLiteral( $s ) {
		if ( $this->lastNode === false ) {
			$this->firstNode = $this->lastNode = new PPNode_Hash_Text( $s );
		} elseif ( $this->lastNode instanceof PPNode_Hash_Text ) {
			$this->lastNode->value .= $s;
		} else {
			$this->lastNode->nextSibling = new PPNode_Hash_Text( $s );
			$this->lastNode = $this->lastNode->nextSibling;
		}
	}

	/**
	 * Append a PPNode
	 */
	function addNode( PPNode $node ) {
		if ( $this->lastNode === false ) {
			$this->firstNode = $this->lastNode = $node;
		} else {
			$this->lastNode->nextSibling = $node;
			$this->lastNode = $node;
		}
	}

	/**
	 * Append a tree node with text contents
	 */
	function addNodeWithText( $name, $value ) {
		$node = PPNode_Hash_Tree::newWithText( $name, $value );
		$this->addNode( $node );
	}

	/**
	 * Append a PPAccum_Hash
	 * Takes over ownership of the nodes in the source argument. These nodes may
	 * subsequently be modified, especially nextSibling.
	 */
	function addAccum( $accum ) {
		if ( $accum->lastNode === false ) {
			// nothing to add
		} elseif ( $this->lastNode === false ) {
			$this->firstNode = $accum->firstNode;
			$this->lastNode = $accum->lastNode;
		} else {
			$this->lastNode->nextSibling = $accum->firstNode;
			$this->lastNode = $accum->lastNode;
		}
	}
}

/**
 * An expansion frame, used as a context to expand the result of preprocessToObj()
 * @ingroup Parser
 */
class PPFrame_Hash implements PPFrame {
	var $preprocessor, $parser, $title;
	var $titleCache;

	/**
	 * Hashtable listing templates which are disallowed for expansion in this frame,
	 * having been encountered previously in parent frames.
	 */
	var $loopCheckHash;

	/**
	 * Recursion depth of this frame, top = 0
	 */
	var $depth;


	/**
	 * Construct a new preprocessor frame.
	 * @param Preprocessor $preprocessor The parent preprocessor
	 */
	function __construct( $preprocessor ) {
		$this->preprocessor = $preprocessor;
		$this->parser = $preprocessor->parser;
		$this->title = $this->parser->mTitle;
		$this->titleCache = array( $this->title ? $this->title->getPrefixedDBkey() : false );
		$this->loopCheckHash = array();
		$this->depth = 0;
	}

	/**
	 * Create a new child frame
	 * $args is optionally a multi-root PPNode or array containing the template arguments
	 */
	function newChild( $args = false, $title = false ) {
		$namedArgs = array();
		$numberedArgs = array();
		if ( $title === false ) {
			$title = $this->title;
		}
		if ( $args !== false ) {
			$xpath = false;
			if ( $args instanceof PPNode_Hash_Array ) {
				$args = $args->value;
			} elseif ( !is_array( $args ) ) {
				throw new MWException( __METHOD__ . ': $args must be array or PPNode_Hash_Array' );
			}
			foreach ( $args as $arg ) {
				$bits = $arg->splitArg();
				if ( $bits['index'] !== '' ) {
					// Numbered parameter
					$numberedArgs[$bits['index']] = $bits['value'];
					unset( $namedArgs[$bits['index']] );
				} else {
					// Named parameter
					$name = trim( $this->expand( $bits['name'], PPFrame::STRIP_COMMENTS ) );
					$namedArgs[$name] = $bits['value'];
					unset( $numberedArgs[$name] );
				}
			}
		}
		return new PPTemplateFrame_Hash( $this->preprocessor, $this, $numberedArgs, $namedArgs, $title );
	}

	function expand( $root, $flags = 0 ) {
		if ( is_string( $root ) ) {
			return $root;
		}

		if ( ++$this->parser->mPPNodeCount > $this->parser->mOptions->mMaxPPNodeCount )
		{
			return '<span class="error">Node-count limit exceeded</span>';
		}
		if ( $this->depth > $this->parser->mOptions->mMaxPPExpandDepth ) {
			return '<span class="error">Expansion depth limit exceeded</span>';
		}
		++$this->depth;

		$outStack = array( '', '' );
		$iteratorStack = array( false, $root );
		$indexStack = array( 0, 0 );

		while ( count( $iteratorStack ) > 1 ) {
			$level = count( $outStack ) - 1;
			$iteratorNode =& $iteratorStack[ $level ];
			$out =& $outStack[$level];
			$index =& $indexStack[$level];

			if ( is_array( $iteratorNode ) ) {
				if ( $index >= count( $iteratorNode ) ) {
					// All done with this iterator
					$iteratorStack[$level] = false;
					$contextNode = false;
				} else {
					$contextNode = $iteratorNode[$index];
					$index++;
				}
			} elseif ( $iteratorNode instanceof PPNode_Hash_Array ) {
				if ( $index >= $iteratorNode->getLength() ) {
					// All done with this iterator
					$iteratorStack[$level] = false;
					$contextNode = false;
				} else {
					$contextNode = $iteratorNode->item( $index );
					$index++;
				}
			} else {
				// Copy to $contextNode and then delete from iterator stack,
				// because this is not an iterator but we do have to execute it once
				$contextNode = $iteratorStack[$level];
				$iteratorStack[$level] = false;
			}

			$newIterator = false;

			if ( $contextNode === false ) {
				// nothing to do
			} elseif ( is_string( $contextNode ) ) {
				$out .= $contextNode;
			} elseif ( is_array( $contextNode ) || $contextNode instanceof PPNode_Hash_Array ) {
				$newIterator = $contextNode;
			} elseif ( $contextNode instanceof PPNode_Hash_Attr ) {
				// No output
			} elseif ( $contextNode instanceof PPNode_Hash_Text ) {
				$out .= $contextNode->value;
			} elseif ( $contextNode instanceof PPNode_Hash_Tree ) {
				if ( $contextNode->name == 'template' ) {
					# Double-brace expansion
					$bits = $contextNode->splitTemplate();
					if ( $flags & self::NO_TEMPLATES ) {
						$newIterator = $this->virtualBracketedImplode( '{{', '|', '}}', $bits['title'], $bits['parts'] );
					} else {
						$ret = $this->parser->braceSubstitution( $bits, $this );
						if ( isset( $ret['object'] ) ) {
							$newIterator = $ret['object'];
						} else {
							$out .= $ret['text'];
						}
					}
				} elseif ( $contextNode->name == 'tplarg' ) {
					# Triple-brace expansion
					$bits = $contextNode->splitTemplate();
					if ( $flags & self::NO_ARGS ) {
						$newIterator = $this->virtualBracketedImplode( '{{{', '|', '}}}', $bits['title'], $bits['parts'] );
					} else {
						$ret = $this->parser->argSubstitution( $bits, $this );
						if ( isset( $ret['object'] ) ) {
							$newIterator = $ret['object'];
						} else {
							$out .= $ret['text'];
						}
					}
				} elseif ( $contextNode->name == 'comment' ) {
					# HTML-style comment
					# Remove it in HTML, pre+remove and STRIP_COMMENTS modes
					if ( $this->parser->ot['html']
						|| ( $this->parser->ot['pre'] && $this->parser->mOptions->getRemoveComments() )
						|| ( $flags & self::STRIP_COMMENTS ) )
					{
						$out .= '';
					}
					# Add a strip marker in PST mode so that pstPass2() can run some old-fashioned regexes on the result
					# Not in RECOVER_COMMENTS mode (extractSections) though
					elseif ( $this->parser->ot['wiki'] && ! ( $flags & self::RECOVER_COMMENTS ) ) {
						$out .= $this->parser->insertStripItem( $contextNode->firstChild->value );
					}
					# Recover the literal comment in RECOVER_COMMENTS and pre+no-remove
					else {
						$out .= $contextNode->firstChild->value;
					}
				} elseif ( $contextNode->name == 'ignore' ) {
					# Output suppression used by <includeonly> etc.
					# OT_WIKI will only respect <ignore> in substed templates.
					# The other output types respect it unless NO_IGNORE is set.
					# extractSections() sets NO_IGNORE and so never respects it.
					if ( ( !isset( $this->parent ) && $this->parser->ot['wiki'] ) || ( $flags & self::NO_IGNORE ) ) {
						$out .= $contextNode->firstChild->value;
					} else {
						//$out .= '';
					}
				} elseif ( $contextNode->name == 'ext' ) {
					# Extension tag
					$bits = $contextNode->splitExt() + array( 'attr' => null, 'inner' => null, 'close' => null );
					$out .= $this->parser->extensionSubstitution( $bits, $this );
				} elseif ( $contextNode->name == 'h' ) {
					# Heading
					if ( $this->parser->ot['html'] ) {
						# Expand immediately and insert heading index marker
						$s = '';
						for ( $node = $contextNode->firstChild; $node; $node = $node->nextSibling ) {
							$s .= $this->expand( $node, $flags );
						}

						$bits = $contextNode->splitHeading();
						$titleText = $this->title->getPrefixedDBkey();
						$this->parser->mHeadings[] = array( $titleText, $bits['i'] );
						$serial = count( $this->parser->mHeadings ) - 1;
						$marker = "{$this->parser->mUniqPrefix}-h-$serial-" . Parser::MARKER_SUFFIX;
						$s = substr( $s, 0, $bits['level'] ) . $marker . substr( $s, $bits['level'] );
						$this->parser->mStripState->general->setPair( $marker, '' );
						$out .= $s;
					} else {
						# Expand in virtual stack
						$newIterator = $contextNode->getChildren();
					}
				} else {
					# Generic recursive expansion
					$newIterator = $contextNode->getChildren();
				}
			} else {
				throw new MWException( __METHOD__.': Invalid parameter type' );
			}

			if ( $newIterator !== false ) {
				$outStack[] = '';
				$iteratorStack[] = $newIterator;
				$indexStack[] = 0;
			} elseif ( $iteratorStack[$level] === false ) {
				// Return accumulated value to parent
				// With tail recursion
				while ( $iteratorStack[$level] === false && $level > 0 ) {
					$outStack[$level - 1] .= $out;
					array_pop( $outStack );
					array_pop( $iteratorStack );
					array_pop( $indexStack );
					$level--;
				}
			}
		}
		--$this->depth;
		return $outStack[0];
	}

	function implodeWithFlags( $sep, $flags /*, ... */ ) {
		$args = array_slice( func_get_args(), 2 );

		$first = true;
		$s = '';
		foreach ( $args as $root ) {
			if ( $root instanceof PPNode_Hash_Array ) {
				$root = $root->value;
			}
			if ( !is_array( $root ) ) {
				$root = array( $root );
			}
			foreach ( $root as $node ) {
				if ( $first ) {
					$first = false;
				} else {
					$s .= $sep;
				}
				$s .= $this->expand( $node, $flags );
			}
		}
		return $s;
	}

	/**
	 * Implode with no flags specified
	 * This previously called implodeWithFlags but has now been inlined to reduce stack depth
	 */
	function implode( $sep /*, ... */ ) {
		$args = array_slice( func_get_args(), 1 );

		$first = true;
		$s = '';
		foreach ( $args as $root ) {
			if ( $root instanceof PPNode_Hash_Array ) {
				$root = $root->value;
			}
			if ( !is_array( $root ) ) {
				$root = array( $root );
			}
			foreach ( $root as $node ) {
				if ( $first ) {
					$first = false;
				} else {
					$s .= $sep;
				}
				$s .= $this->expand( $node );
			}
		}
		return $s;
	}

	/**
	 * Makes an object that, when expand()ed, will be the same as one obtained
	 * with implode()
	 */
	function virtualImplode( $sep /*, ... */ ) {
		$args = array_slice( func_get_args(), 1 );
		$out = array();
		$first = true;

		foreach ( $args as $root ) {
			if ( $root instanceof PPNode_Hash_Array ) {
				$root = $root->value;
			}
			if ( !is_array( $root ) ) {
				$root = array( $root );
			}
			foreach ( $root as $node ) {
				if ( $first ) {
					$first = false;
				} else {
					$out[] = $sep;
				}
				$out[] = $node;
			}
		}
		return new PPNode_Hash_Array( $out );
	}

	/**
	 * Virtual implode with brackets
	 */
	function virtualBracketedImplode( $start, $sep, $end /*, ... */ ) {
		$args = array_slice( func_get_args(), 3 );
		$out = array( $start );
		$first = true;

		foreach ( $args as $root ) {
			if ( $root instanceof PPNode_Hash_Array ) {
				$root = $root->value;
			}
			if ( !is_array( $root ) ) {
				$root = array( $root );
			}
			foreach ( $root as $node ) {
				if ( $first ) {
					$first = false;
				} else {
					$out[] = $sep;
				}
				$out[] = $node;
			}
		}
		$out[] = $end;
		return new PPNode_Hash_Array( $out );
	}

	function __toString() {
		return 'frame{}';
	}

	function getPDBK( $level = false ) {
		if ( $level === false ) {
			return $this->title->getPrefixedDBkey();
		} else {
			return isset( $this->titleCache[$level] ) ? $this->titleCache[$level] : false;
		}
	}

	/**
	 * Returns true if there are no arguments in this frame
	 */
	function isEmpty() {
		return true;
	}

	function getArgument( $name ) {
		return false;
	}

	/**
	 * Returns true if the infinite loop check is OK, false if a loop is detected
	 */
	function loopCheck( $title ) {
		return !isset( $this->loopCheckHash[$title->getPrefixedDBkey()] );
	}

	/**
	 * Return true if the frame is a template frame
	 */
	function isTemplate() {
		return false;
	}
}

/**
 * Expansion frame with template arguments
 * @ingroup Parser
 */
class PPTemplateFrame_Hash extends PPFrame_Hash {
	var $numberedArgs, $namedArgs, $parent;
	var $numberedExpansionCache, $namedExpansionCache;

	function __construct( $preprocessor, $parent = false, $numberedArgs = array(), $namedArgs = array(), $title = false ) {
		$this->preprocessor = $preprocessor;
		$this->parser = $preprocessor->parser;
		$this->parent = $parent;
		$this->numberedArgs = $numberedArgs;
		$this->namedArgs = $namedArgs;
		$this->title = $title;
		$pdbk = $title ? $title->getPrefixedDBkey() : false;
		$this->titleCache = $parent->titleCache;
		$this->titleCache[] = $pdbk;
		$this->loopCheckHash = /*clone*/ $parent->loopCheckHash;
		if ( $pdbk !== false ) {
			$this->loopCheckHash[$pdbk] = true;
		}
		$this->depth = $parent->depth + 1;
		$this->numberedExpansionCache = $this->namedExpansionCache = array();
	}

	function __toString() {
		$s = 'tplframe{';
		$first = true;
		$args = $this->numberedArgs + $this->namedArgs;
		foreach ( $args as $name => $value ) {
			if ( $first ) {
				$first = false;
			} else {
				$s .= ', ';
			}
			$s .= "\"$name\":\"" .
				str_replace( '"', '\\"', $value->__toString() ) . '"';
		}
		$s .= '}';
		return $s;
	}
	/**
	 * Returns true if there are no arguments in this frame
	 */
	function isEmpty() {
		return !count( $this->numberedArgs ) && !count( $this->namedArgs );
	}

	function getNumberedArgument( $index ) {
		if ( !isset( $this->numberedArgs[$index] ) ) {
			return false;
		}
		if ( !isset( $this->numberedExpansionCache[$index] ) ) {
			# No trimming for unnamed arguments
			$this->numberedExpansionCache[$index] = $this->parent->expand( $this->numberedArgs[$index], self::STRIP_COMMENTS );
		}
		return $this->numberedExpansionCache[$index];
	}

	function getNamedArgument( $name ) {
		if ( !isset( $this->namedArgs[$name] ) ) {
			return false;
		}
		if ( !isset( $this->namedExpansionCache[$name] ) ) {
			# Trim named arguments post-expand, for backwards compatibility
			$this->namedExpansionCache[$name] = trim(
				$this->parent->expand( $this->namedArgs[$name], self::STRIP_COMMENTS ) );
		}
		return $this->namedExpansionCache[$name];
	}

	function getArgument( $name ) {
		$text = $this->getNumberedArgument( $name );
		if ( $text === false ) {
			$text = $this->getNamedArgument( $name );
		}
		return $text;
	}

	/**
	 * Return true if the frame is a template frame
	 */
	function isTemplate() {
		return true;
	}
}

/**
 * Expansion frame with custom arguments
 * @ingroup Parser
 */
class PPCustomFrame_Hash extends PPFrame_Hash {
	var $args;

	function __construct( $preprocessor, $args ) {
		$this->preprocessor = $preprocessor;
		$this->parser = $preprocessor->parser;
		$this->args = $args;
	}

	function __toString() {
		$s = 'cstmframe{';
		$first = true;
		foreach ( $this->args as $name => $value ) {
			if ( $first ) {
				$first = false;
			} else {
				$s .= ', ';
			}
			$s .= "\"$name\":\"" .
				str_replace( '"', '\\"', $value->__toString() ) . '"';
		}
		$s .= '}';
		return $s;
	}

	function isEmpty() {
		return !count( $this->args );
	}

	function getArgument( $index ) {
		return $this->args[$index];
	}
}

/**
 * @ingroup Parser
 */
class PPNode_Hash_Tree implements PPNode {
	var $name, $firstChild, $lastChild, $nextSibling;

	function __construct( $name ) {
		$this->name = $name;
		$this->firstChild = $this->lastChild = $this->nextSibling = false;
	}

	function __toString() {
		$inner = '';
		$attribs = '';
		for ( $node = $this->firstChild; $node; $node = $node->nextSibling ) {
			if ( $node instanceof PPNode_Hash_Attr ) {
				$attribs .= ' ' . $node->name . '="' . htmlspecialchars( $node->value ) . '"';
			} else {
				$inner .= $node->__toString();
			}
		}
		if ( $inner === '' ) {
			return "<{$this->name}$attribs/>";
		} else {
			return "<{$this->name}$attribs>$inner</{$this->name}>";
		}
	}

	static function newWithText( $name, $text ) {
		$obj = new self( $name );
		$obj->addChild( new PPNode_Hash_Text( $text ) );
		return $obj;
	}

	function addChild( $node ) {
		if ( $this->lastChild === false ) {
			$this->firstChild = $this->lastChild = $node;
		} else {
			$this->lastChild->nextSibling = $node;
			$this->lastChild = $node;
		}
	}

	function getChildren() {
		$children = array();
		for ( $child = $this->firstChild; $child; $child = $child->nextSibling ) {
			$children[] = $child;
		}
		return new PPNode_Hash_Array( $children );
	}

	function getFirstChild() {
		return $this->firstChild;
	}

	function getNextSibling() {
		return $this->nextSibling;
	}

	function getChildrenOfType( $name ) {
		$children = array();
		for ( $child = $this->firstChild; $child; $child = $child->nextSibling ) {
			if ( isset( $child->name ) && $child->name === $name ) {
				$children[] = $name;
			}
		}
		return $children;
	}

	function getLength() { return false; }
	function item( $i ) { return false; }

	function getName() {
		return $this->name;
	}

	/**
	 * Split a <part> node into an associative array containing:
	 *    name          PPNode name
	 *    index         String index
	 *    value         PPNode value
	 */
	function splitArg() {
		$bits = array();
		for ( $child = $this->firstChild; $child; $child = $child->nextSibling ) {
			if ( !isset( $child->name ) ) {
				continue;
			}
			if ( $child->name === 'name' ) {
				$bits['name'] = $child;
				if ( $child->firstChild instanceof PPNode_Hash_Attr
					&& $child->firstChild->name === 'index' )
				{
					$bits['index'] = $child->firstChild->value;
				}
			} elseif ( $child->name === 'value' ) {
				$bits['value'] = $child;
			}
		}

		if ( !isset( $bits['name'] ) ) {
			throw new MWException( 'Invalid brace node passed to ' . __METHOD__ );
		}
		if ( !isset( $bits['index'] ) ) {
			$bits['index'] = '';
		}
		return $bits;
	}

	/**
	 * Split an <ext> node into an associative array containing name, attr, inner and close
	 * All values in the resulting array are PPNodes. Inner and close are optional.
	 */
	function splitExt() {
		$bits = array();
		for ( $child = $this->firstChild; $child; $child = $child->nextSibling ) {
			if ( !isset( $child->name ) ) {
				continue;
			}
			if ( $child->name == 'name' ) {
				$bits['name'] = $child;
			} elseif ( $child->name == 'attr' ) {
				$bits['attr'] = $child;
			} elseif ( $child->name == 'inner' ) {
				$bits['inner'] = $child;
			} elseif ( $child->name == 'close' ) {
				$bits['close'] = $child;
			}
		}
		if ( !isset( $bits['name'] ) ) {
			throw new MWException( 'Invalid ext node passed to ' . __METHOD__ );
		}
		return $bits;
	}

	/**
	 * Split an <h> node
	 */
	function splitHeading() {
		if ( $this->name !== 'h' ) {
			throw new MWException( 'Invalid h node passed to ' . __METHOD__ );
		}
		$bits = array();
		for ( $child = $this->firstChild; $child; $child = $child->nextSibling ) {
			if ( !isset( $child->name ) ) {
				continue;
			}
			if ( $child->name == 'i' ) {
				$bits['i'] = $child->value;
			} elseif ( $child->name == 'level' ) {
				$bits['level'] = $child->value;
			}
		}
		if ( !isset( $bits['i'] ) ) {
			throw new MWException( 'Invalid h node passed to ' . __METHOD__ );
		}
		return $bits;
	}

	/**
	 * Split a <template> or <tplarg> node
	 */
	function splitTemplate() {
		$parts = array();
		$bits = array( 'lineStart' => '' );
		for ( $child = $this->firstChild; $child; $child = $child->nextSibling ) {
			if ( !isset( $child->name ) ) {
				continue;
			}
			if ( $child->name == 'title' ) {
				$bits['title'] = $child;
			}
			if ( $child->name == 'part' ) {
				$parts[] = $child;
			}
			if ( $child->name == 'lineStart' ) {
				$bits['lineStart'] = '1';
			}
		}
		if ( !isset( $bits['title'] ) ) {
			throw new MWException( 'Invalid node passed to ' . __METHOD__ );
		}
		$bits['parts'] = new PPNode_Hash_Array( $parts );
		return $bits;
	}
}

/**
 * @ingroup Parser
 */
class PPNode_Hash_Text implements PPNode {
	var $value, $nextSibling;

	function __construct( $value ) {
		if ( is_object( $value ) ) {
			throw new MWException( __CLASS__ . ' given object instead of string' );
		}
		$this->value = $value;
	}

	function __toString() {
		return htmlspecialchars( $this->value );
	}

	function getNextSibling() {
		return $this->nextSibling;
	}

	function getChildren() { return false; }
	function getFirstChild() { return false; }
	function getChildrenOfType( $name ) { return false; }
	function getLength() { return false; }
	function item( $i ) { return false; }
	function getName() { return '#text'; }
	function splitArg() { throw new MWException( __METHOD__ . ': not supported' ); }
	function splitExt() { throw new MWException( __METHOD__ . ': not supported' ); }
	function splitHeading() { throw new MWException( __METHOD__ . ': not supported' ); }
}

/**
 * @ingroup Parser
 */
class PPNode_Hash_Array implements PPNode {
	var $value, $nextSibling;

	function __construct( $value ) {
		$this->value = $value;
	}

	function __toString() {
		return var_export( $this, true );
	}

	function getLength() {
		return count( $this->value );
	}

	function item( $i ) {
		return $this->value[$i];
	}

	function getName() { return '#nodelist'; }

	function getNextSibling() {
		return $this->nextSibling;
	}

	function getChildren() { return false; }
	function getFirstChild() { return false; }
	function getChildrenOfType( $name ) { return false; }
	function splitArg() { throw new MWException( __METHOD__ . ': not supported' ); }
	function splitExt() { throw new MWException( __METHOD__ . ': not supported' ); }
	function splitHeading() { throw new MWException( __METHOD__ . ': not supported' ); }
}

/**
 * @ingroup Parser
 */
class PPNode_Hash_Attr implements PPNode {
	var $name, $value, $nextSibling;

	function __construct( $name, $value ) {
		$this->name = $name;
		$this->value = $value;
	}

	function __toString() {
		return "<@{$this->name}>" . htmlspecialchars( $this->value ) . "</@{$this->name}>";
	}

	function getName() {
		return $this->name;
	}

	function getNextSibling() {
		return $this->nextSibling;
	}

	function getChildren() { return false; }
	function getFirstChild() { return false; }
	function getChildrenOfType( $name ) { return false; }
	function getLength() { return false; }
	function item( $i ) { return false; }
	function splitArg() { throw new MWException( __METHOD__ . ': not supported' ); }
	function splitExt() { throw new MWException( __METHOD__ . ': not supported' ); }
	function splitHeading() { throw new MWException( __METHOD__ . ': not supported' ); }
}
