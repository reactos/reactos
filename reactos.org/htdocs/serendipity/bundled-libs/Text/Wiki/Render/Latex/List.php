<?php


class Text_Wiki_Render_Latex_List extends Text_Wiki_Render {
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
        
        switch ($type)
            {
            case 'bullet_list_start':
                return "\\begin{itemize}\n";
                
            case 'bullet_list_end':
                return "\\end{itemize}\n";
                
            case 'number_list_start':
                return "\\begin{enumerate}\n";
                
            case 'number_list_end':
                return "\\end{enumerate}\n";
                
            case 'bullet_item_start':
            case 'number_item_start':
                return "\\item{";
                
            case 'bullet_item_end':
            case 'number_item_end':
                return "}\n";
                
            default:
                // ignore item endings and all other types.
                // item endings are taken care of by the other types
                // depending on their place in the list.
                return '';
                break;
            }
    }
}
?>