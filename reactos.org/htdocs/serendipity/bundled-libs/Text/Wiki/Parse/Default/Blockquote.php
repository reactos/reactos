<?php

/**
* 
* Parse for block-quoted text.
* 
* Find source text marked as a blockquote, identified by any number of
* greater-than signs '>' at the start of the line, followed by a space,
* and then the quote text; each '>' indicates an additional level of
* quoting.
* 
* $Id: Blockquote.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $
* 
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Parse_Blockquote extends Text_Wiki_Parse {
    
    
    /**
    * 
    * Regex for parsing the source text.
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
    * Generates a replacement for the matched text.
    * 
    * Token options are:
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
            while ($level > count($stack)) {
                
                // the current indent level is greater than the number
                // of stack elements, so we must be starting a new
                // level.  push the new level onto the stack with a 
                // dummy value (boolean true)...
                array_push($stack, true);
                
                $return .= "\n";
                
                // ...and add a start token to the return.
                $return .= $this->wiki->addToken(
                    $this->rule, 
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
                
                $return .= $this->wiki->addToken(
                    $this->rule, 
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
            $return .= $this->wiki->addToken(
                $this->rule, 
                array (
                    'type' => 'end',
                    'level' => count($stack)
                )
            );
        }
        
        // we're done!  send back the replacement text.
        return "\n$return\n\n";
    }
}
?>