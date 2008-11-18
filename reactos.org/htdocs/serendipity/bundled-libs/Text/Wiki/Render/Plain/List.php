<?php


class Text_Wiki_Render_Plain_List extends Text_Wiki_Render {
    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * This rendering method is syntactically and semantically compliant
    * with XHTML 1.1 in that sub-lists are part of the previous list item.
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
        // make nice variables (type, level, count)
        extract($options);
        
        // set up indenting so that the results look nice; we do this
        // in two steps to avoid str_pad mathematics.  ;-)
        $pad = str_pad('', $level, "\t");
        $pad = str_replace("\t", '    ', $pad);
                
        switch ($type) {
        
        case 'bullet_list_start':
            break;
        
        case 'bullet_list_end':
            if ($level == 0) {
                return "\n\n";
            }
            break;
        
        case 'number_list_start':
            break;
        
        case 'number_list_end':
            if ($level == 0) {
                return "\n\n";
            }
            break;
        
        case 'bullet_item_start':
        case 'number_item_start':
            return "\n$pad";
            break;
        
        case 'bullet_item_end':
        case 'number_item_end':
        default:
            // ignore item endings and all other types.
            // item endings are taken care of by the other types
            // depending on their place in the list.
            return;
            break;
        }
    }
}
?>