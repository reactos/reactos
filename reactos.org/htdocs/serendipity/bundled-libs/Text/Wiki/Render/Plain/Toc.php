<?php

class Text_Wiki_Render_Plain_Toc extends Text_Wiki_Render {
    
    
    
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
        // type, count, level
        extract($options);
        
        if ($type == 'item_start') {
            
            // build some indenting spaces for the text
            $indent = ($level - 2) * 4;
            $pad = str_pad('', $indent);
            return $pad;
        }
        
        if ($type == 'item_end') {
            return "\n";
        }
    }

}
?>