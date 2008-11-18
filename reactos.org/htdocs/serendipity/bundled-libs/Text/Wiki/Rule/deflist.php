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
// $Id: deflist.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked as a
* definition list.  In short, if a line starts with ':' then it is a
* definition list item; another ':' on the same lines indicates the end
* of the definition term and the beginning of the definition narrative.
* The list items must be on sequential lines (no blank lines between
* them) -- a blank line indicates the beginning of a new list.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_deflist extends Text_Wiki_Rule {
    
    
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
    
    var $regex = '/\n((:).*\n)(?!(:))/Us';
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'type' =>
    *     'list_start'    : the start of a definition list
    *     'list_end'      : the end of a definition list
    *     'term_start'    : the start of a definition term
    *     'term_end'      : the end of a definition term
    *     'narr_start'    : the start of definition narrative
    *     'narr_end'      : the end of definition narrative
    *     'unknown'       : unknown type of definition portion
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
        
        // start the deflist
        $options = array('type' => 'list_start');
        $return .= $this->addToken($options);
        
        // $matches[1] is the text matched as a list set by parse();
        // create an array called $list that contains a new set of
        // matches for the various definition-list elements.
        preg_match_all(
            '/^(:)(.*)?(:)(.*)?$/Ums',
            $matches[1],
            $list,
            PREG_SET_ORDER
        );
        
        // add each term and narrative
        foreach ($list as $key => $val) {
            $return .= (
                $this->addToken(array('type' => 'term_start')) .
                trim($val[2]) .
                $this->addToken(array('type' => 'term_end')) .
                $this->addToken(array('type' => 'narr_start')) .
                trim($val[4]) . 
                $this->addToken(array('type' => 'narr_end'))
            );
        }
        
        
        // end the deflist
        $options = array('type' => 'list_end');
        $return .= $this->addToken($options);
        
        // done!
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
        $type = $options['type'];
        $pad = "    ";
        
        switch ($type) {
        
        case 'list_start':
            return "<dl>\n";
            break;
        
        case 'list_end':
            return "</dl>\n";
            break;
        
        case 'term_start':
            return $pad . "<dt>";
            break;
        
        case 'term_end':
            return "</dt>\n";
            break;
        
        case 'narr_start':
            return $pad . $pad . "<dd>";
            break;
        
        case 'narr_end':
            return "</dd>\n";
            break;
        
        default:
            return '';
        
        }
    }
}
?>
