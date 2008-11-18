<?php
/* vim: set expandtab tabstop=4 shiftwidth=4: */
// +----------------------------------------------------------------------+
// | PHP version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2003 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at                              |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Paul M. Jones <pmjones@ciaweb.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: blockquote.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked as a
* blockquote, identified by any number of greater-than signs '>' at the
* start of the line, followed by a space, and then the quote text; each
* '>' indicates an additional level of quoting.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_blockquote extends Text_Wiki_Rule {
    
    
    /**
    * 
    * The regular expression used to parse the source text and find
    * matches conforming to this rule.  Used by the parse() method.
    * 
    * @access public
    * 
    * @var string
    * 
    * @see parse()
    * 
    */
    
    var $regex = '/\n((\>).*\n)(?!(\>))/Us';
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'type' =>
    *     'start' : the start of a blockquote
    *     'end'   : the end of a blockquote
    *
    * 'level' => the indent level (0 for the first level, 1 for the
    * second, etc)
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A series of text and delimited tokens marking the different
    * list text and list elements.
    *
    */
    
    function process(&$matches)
    {
        // the replacement text we will return to parse()
        $return = '';
        
        // the list of post-processing matches
        $list = array();
        
        // $matches[1] is the text matched as a list set by parse();
        // create an array called $list that contains a new set of
        // matches for the various list-item elements.
        preg_match_all(
            '=^(\>+) (.*\n)=Ums',
            $matches[1],
            $list,
            PREG_SET_ORDER
        );
        
        // a stack of starts and ends; we keep this so that we know what
        // indent level we're at.
        $stack = array();
        
        // loop through each list-item element.
        foreach ($list as $key => $val) {
            
            // $val[0] is the full matched list-item line
            // $val[1] is the number of initial '>' chars (indent level)
            // $val[2] is the quote text
            
            // we number levels starting at 1, not zero
            $level = strlen($val[1]);
            
            // get the text of the line
            $text = $val[2];
            
            // add a level to the list?
            if ($level > count($stack)) {
                
                // the current indent level is greater than the number
                // of stack elements, so we must be starting a new
                // level.  push the new level onto the stack with a 
                // dummy value (boolean true)...
                array_push($stack, true);
                
                $return .= "\n";
                
                // ...and add a start token to the return.
                $return .= $this->addToken(
                    array(
                        'type' => 'start',
                        'level' => $level - 1
                    )
                );
                
                $return .= "\n\n";
            }
            
            // remove a level?
            while (count($stack) > $level) {
                
                // as long as the stack count is greater than the
                // current indent level, we need to end list types.
                // continue adding end-list tokens until the stack count
                // and the indent level are the same.
                array_pop($stack);
                
                $return .= "\n\n";
                
                $return .= $this->addToken(
                    array (
                        'type' => 'end',
                        'level' => count($stack)
                    )
                );
                
                $return .= "\n";
            }
            
            // add the line text.
            $return .= $text;
        }
        
        // the last line may have been indented.  go through the stack
        // and create end-tokens until the stack is empty.
        $return .= "\n";
        
        while (count($stack) > 0) {
            array_pop($stack);
            $return .= $this->addToken(
                array (
                    'type' => 'end',
                    'level' => count($stack)
                )
            );
        }
        
        // we're done!  send back the replacement text.
        return "$return\n";
    }
    
    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function renderXhtml($options)
    {
        $type = $options['type'];
        $level = $options['level'];
	
		// set up indenting so that the results look nice; we do this
		// in two steps to avoid str_pad mathematics.  ;-)
		$pad = str_pad('', $level, "\t");
		$pad = str_replace("\t", '    ', $pad);
		
		// starting
		if ($type == 'start') {
			return "$pad<blockquote>";
		}
		
		// ending
		if ($type == 'end') {
			return $pad . "</blockquote>\n";
		}
    }
}
?>
