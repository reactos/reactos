<?php

class Text_Wiki_Render_Plain_Deflist extends Text_Wiki_Render {
    
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
        $pad = "    ";
        
        switch ($type) {
        
        case 'list_start':
            return "\n";
            break;
        
        case 'list_end':
            return "\n\n";
            break;
        
        case 'term_start':
        
            // done!
            return $pad;
            break;
        
        case 'term_end':
            return "\n";
            break;
        
        case 'narr_start':
        
            // done!
            return $pad . $pad;
            break;
        
        case 'narr_end':
            return "\n";
            break;
        
        default:
            return '';
        
        }
    }
}
?>