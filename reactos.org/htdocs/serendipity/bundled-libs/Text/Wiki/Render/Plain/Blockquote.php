<?php

class Text_Wiki_Render_Plain_Blockquote extends Text_Wiki_Render {
    
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
    
    function token($options)
    {
        $type = $options['type'];
        $level = $options['level'];
    
        // set up indenting so that the results look nice; we do this
        // in two steps to avoid str_pad mathematics.  ;-)
        $pad = str_pad('', $level + 1, "\t");
        $pad = str_replace("\t", '    ', $pad);
        
        // starting
        if ($type == 'start') {
            return "\n$pad";
        }
        
        // ending
        if ($type == 'end') {
            return "\n$pad";
        }
    }
}
?>