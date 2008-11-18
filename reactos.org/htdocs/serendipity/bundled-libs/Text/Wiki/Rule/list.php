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
// $Id: list.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked as
* a bulleted or numbered list.  In short, if a line starts with '* ' then
* it is a bullet list item; if a line starts with '# ' then it is a 
* number list item.  Spaces in front of the * or # indicate an indented
* sub-list.  The list items must be on sequential lines (no blank lines
* between them) -- a blank line indicates the beginning of a new list.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_list extends Text_Wiki_Rule {
    
    
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
    
    var $regex = '/\n((\*|#) .*\n)(?! {0,}(\*|#))/Us';
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'type' =>
    *     'bullet_start' : the start of a bullet list
    *     'bullet_end'   : the end of a bullet list
    *     'number_start' : the start of a number list
    *     'number_end'   : the end of a number list
    *     'item_start'   : the start of item text (bullet or number)
    *     'item_end'     : the end of item text (bullet or number)
    *     'unknown'      : unknown type of list or item
    *
    * 'level' => the indent level (0 for the first level, 1 for the
    * second, etc)
    *
    * 'count' => the list item number at this level. not needed for
    * xhtml, but very useful for PDF and RTF.
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
        // the replacement text we will return
        $return = '';
        
        // the list of post-processing matches
        $list = array();
        
        // a stack of list-start and list-end types; we keep this
        // so that we know what kind of list we're working with
        // (bullet or number) and what indent level we're at.
        $stack = array();
        
        // the item count is the number of list items for any
        // given list-type on the stack
        $itemcount = array();
        
        // populate $list with this set of matches. $matches[1] is the
        // text matched as a list set by parse().
        preg_match_all(
            '=^( {0,})(\*|#) (.*)$=Ums',
            $matches[1],
            $list,
            PREG_SET_ORDER
        );
        
        // loop through each list-item element.
        foreach ($list as $key => $val) {
            
            // $val[0] is the full matched list-item line
            // $val[1] is the number of initial spaces (indent level)
            // $val[2] is the list item type (* or #)
            // $val[3] is the list item text
            
            // how many levels are we indented? (1 means the "root"
            // list level, no indenting.)
            $level = strlen($val[1]) + 1;
            
            // get the list item type
            if ($val[2] == '*') {
                $type = 'bullet';
            } elseif ($val[2] == '#') {
                $type = 'number';
            } else {
                $type = 'unknown';
            }
            
            // get the text of the list item
            $text = $val[3];
            
            // add a level to the list?
            if ($level > count($stack)) {
                
                // the current indent level is greater than the
                // number of stack elements, so we must be starting
                // a new list.  push the new list type onto the
                // stack...
                array_push($stack, $type);
                
                // ...and add a list-start token to the return.
                $return .= $this->addToken(
                    array(
                        'type' => $type . '_start',
                        'level' => $level - 1
                    )
                );
            }
            
            // remove a level from the list?
            while (count($stack) > $level) {
                
                // so we don't keep counting the stack, we set up a temp
                // var for the count.  -1 becuase we're going to pop the
                // stack in the next command.  $tmp will then equal the
                // current level of indent.
                $tmp = count($stack) - 1;
                
                // as long as the stack count is greater than the
                // current indent level, we need to end list types. 
                // continue adding end-list tokens until the stack count
                // and the indent level are the same.
                $return .= $this->addToken(
                    array (
                        'type' => array_pop($stack) . '_end',
                        'level' => $tmp
                    )
                );
                
                // reset to the current (previous) list type so that
                // the new list item matches the proper list type.
                $type = $stack[$tmp - 1];
                
                // reset the item count for the popped indent level
                $itemcount[$tmp + 1] = 0;
            }
            
            // add to the item count for this list (taking into account
            // which level we are at).
            if (! isset($itemcount[$level])) {
                // first count
                $itemcount[$level] = 1;
            } else {
                // increment count
                $itemcount[$level]++;
            }
            
            // create a list-item starting token.
            $start = $this->addToken(
                array(
                    'type' => 'item_start',
                    'level' => $level,
                    'count' => $itemcount[$level]
                )
            );
            
            // create a list-item ending token.
            $end = $this->addToken(
                array(
                    'type' => 'item_end',
                    'level' => $level,
                    'count' => $itemcount[$level]
                )
            );
            
            // add the starting token, list-item text, and ending token
            // to the return.
            $return .= $start . $val[3] . $end;
        }
        
        // the last list-item may have been indented.  go through the
        // list-type stack and create end-list tokens until the stack
        // is empty.
        while (count($stack) > 0) {
            $return .= $this->addToken(
                array (
                    'type' => array_pop($stack) . '_end',
                    'level' => count($stack)
                )
            );
        }
        
        // we're done!  send back the replacement text.
        return "\n" . $return . "\n";
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
        // make nice variables (type, level, count)
        extract($options);
        
        // set up indenting so that the results look nice; we do this
        // in two steps to avoid str_pad mathematics.  ;-)
        $pad = str_pad('', $level, "\t");
        $pad = str_replace("\t", '    ', $pad);
        
        // attempt XHTML compliance so that sub-lists are part
        // of a list item, not between list items
        if ($level > 0) {
        	$pre = '<li style="list-style: none;">';
        	$post = '</li>';
        } else {
        	$pre = '';
        	$post = '';
        }
        
        switch ($type) {
        
        case 'bullet_start':
            return "$pad$pre<ul>\n";
            break;
        
        case 'bullet_end':
            return "$pad</ul>$post\n";
            break;
        
        case 'number_start':
            return "$pad$pre<ol>\n";
            break;
        
        case 'number_end':
            return "$pad</ol>$post\n";
            break;
        
        case 'item_start':
            return "$pad<li>";
            break;
        
        case 'item_end':
            return "</li>\n";
            break;
        
        default:
            return '';
        
        }
    }
}
?>
