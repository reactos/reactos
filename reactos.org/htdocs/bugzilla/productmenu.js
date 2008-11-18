// Adds to the target select object all elements in array that
// correspond to the elements selected in source.
//     - array should be a array of arrays, indexed by product name. the
//       array should contain the elements that correspont to that
//       product. Example:
//         var array = Array();
//         array['ProductOne'] = [ 'ComponentA', 'ComponentB' ];
//         updateSelect(array, source, target);
//     - sel is a list of selected items, either whole or a diff
//       depending on sel_is_diff.
//     - sel_is_diff determines if we are sending in just a diff or the
//       whole selection. a diff is used to optimize adding selections.
//     - target should be the target select object.
//     - single specifies if we selected a single item. if we did, no
//       need to merge.

function updateSelect( array, sel, target, sel_is_diff, single, blank ) {

    var i, j, comp;

    // if single, even if it's a diff (happens when you have nothing
    // selected and select one item alone), skip this.
    if ( ! single ) {

        // array merging/sorting in the case of multiple selections
        if ( sel_is_diff ) {

            // merge in the current options with the first selection
            comp = merge_arrays( array[sel[0]], target.options, 1 );

            // merge the rest of the selection with the results
            for ( i = 1 ; i < sel.length ; i++ ) {
                comp = merge_arrays( array[sel[i]], comp, 0 );
            }
        } else {
            // here we micro-optimize for two arrays to avoid merging with a
            // null array
            comp = merge_arrays( array[sel[0]],array[sel[1]], 0 );

            // merge the arrays. not very good for multiple selections.
            for ( i = 2; i < sel.length; i++ ) {
                comp = merge_arrays( comp, array[sel[i]], 0 );
            }
        }
    } else {
        // single item in selection, just get me the list
        comp = array[sel[0]];
    }

    // save the selection in the target select so we can restore it later
    var selections = new Array();
    for ( i = 0; i < target.options.length; i++ )
      if (target.options[i].selected) selections.push(target.options[i].value);

    // clear select
    target.options.length = 0;

    // add empty "Any" value back to the list
    if (blank) target.options[0] = new Option( blank, "" );

    // load elements of list into select
    for ( i = 0; i < comp.length; i++ ) {
        target.options[target.options.length] = new Option( comp[i], comp[i] );
    }

    // restore the selection
    for ( i=0 ; i<selections.length ; i++ )
      for ( j=0 ; j<target.options.length ; j++ )
        if (target.options[j].value == selections[i]) target.options[j].selected = true;

}

// Returns elements in a that are not in b.
// NOT A REAL DIFF: does not check the reverse.
//     - a,b: arrays of values to be compare.

function fake_diff_array( a, b ) {
    var newsel = new Array();

    // do a boring array diff to see who's new
        for ( var ia in a ) {
            var found = 0;
            for ( var ib in b ) {
                if ( a[ia] == b[ib] ) {
                    found = 1;
                }
            }
            if ( ! found ) {
                newsel[newsel.length] = a[ia];
            }
            found = 0;
        }
        return newsel;
    }

// takes two arrays and sorts them by string, returning a new, sorted
// array. the merge removes dupes, too.
//     - a, b: arrays to be merge.
//     - b_is_select: if true, then b is actually an optionitem and as
//       such we need to use item.value on it.

    function merge_arrays( a, b, b_is_select ) {
        var pos_a = 0;
        var pos_b = 0;
        var ret = new Array();
        var bitem, aitem;

    // iterate through both arrays and add the larger item to the return
    // list. remove dupes, too. Use toLowerCase to provide
    // case-insensitivity.

        while ( ( pos_a < a.length ) && ( pos_b < b.length ) ) {

            if ( b_is_select ) {
                bitem = b[pos_b].value;
            } else {
                bitem = b[pos_b];
            }
            aitem = a[pos_a];

        // smaller item in list a
            if ( aitem.toLowerCase() < bitem.toLowerCase() ) {
                ret[ret.length] = aitem;
                pos_a++;
            } else {
            // smaller item in list b
                if ( aitem.toLowerCase() > bitem.toLowerCase() ) {
                    ret[ret.length] = bitem;
                    pos_b++;
                } else {
                // list contents are equal, inc both counters.
                    ret[ret.length] = aitem;
                pos_a++;
                pos_b++;
            }
        }
        }

    // catch leftovers here. these sections are ugly code-copying.
        if ( pos_a < a.length ) {
            for ( ; pos_a < a.length ; pos_a++ ) {
                ret[ret.length] = a[pos_a];
            }
        }

        if ( pos_b < b.length ) {
            for ( ; pos_b < b.length; pos_b++ ) {
                if ( b_is_select ) {
                    bitem = b[pos_b].value;
                } else {
                    bitem = b[pos_b];
                }
                ret[ret.length] = bitem;
            }
        }
        return ret;
    }

// selectProduct reads the selection from f[productfield] and updates
// f.version, component and target_milestone accordingly.
//     - f: a form containing product, component, varsion and
//       target_milestone select boxes.
// globals (3vil!):
//     - cpts, vers, tms: array of arrays, indexed by product name. the
//       subarrays contain a list of names to be fed to the respective
//       selectboxes. For bugzilla, these are generated with perl code
//       at page start.
//     - usetms: this is a global boolean that is defined if the
//       bugzilla installation has it turned on. generated in perl too.
//     - first_load: boolean, specifying if it's the first time we load
//       the query page.
//     - last_sel: saves our last selection list so we know what has
//       changed, and optimize for additions.

function selectProduct( f , productfield, componentfield, blank ) {

    // this is to avoid handling events that occur before the form
    // itself is ready, which happens in buggy browsers.

    if ( ( !f ) || ( ! f[productfield] ) ) {
        return;
    }

    // Do nothing if no products are defined (this avoids the
    // "a has no properties" error from merge_arrays function)
    if (f[productfield].length == blank ? 1 : 0) {
        return;
    }

    // if this is the first load and nothing is selected, no need to
    // merge and sort all components; perl gives it to us sorted.

    if ( ( first_load ) && ( f[productfield].selectedIndex == -1 ) ) {
            first_load = 0;
            return;
    }

    // turn first_load off. this is tricky, since it seems to be
    // redundant with the above clause. It's not: if when we first load
    // the page there is _one_ element selected, it won't fall into that
    // clause, and first_load will remain 1. Then, if we unselect that
    // item, selectProduct will be called but the clause will be valid
    // (since selectedIndex == -1), and we will return - incorrectly -
    // without merge/sorting.

    first_load = 0;

    // - sel keeps the array of products we are selected.
    // - is_diff says if it's a full list or just a list of products that
    //   were added to the current selection.
    // - single indicates if a single item was selected
    // - selectedIndex is the index of the first selected item
    // - selectedValue is the value of the first selected item
    var sel = Array();
    var is_diff = 0;
    var single;
    var selectedIndex = f[productfield].selectedIndex;
    var selectedValue = f[productfield].options[selectedIndex].value;

    // If nothing is selected, or the selected item is the "blank" value
    // at the top of the list which represents all products on drop-down menus,
    // then pick all products so we show all components.
    if ( selectedIndex == -1 || !cpts[selectedValue])
    {
        for ( var i = blank ? 1 : 0 ; i < f[productfield].length ; i++ ) {
            sel[sel.length] = f[productfield].options[i].value;
        }
        // If there is only one product, then only one product can be selected
        single = ( sel.length == 1 );
    } else {

        for ( i = blank ? 1 : 0 ; i < f[productfield].length ; i++ ) {
            if ( f[productfield].options[i].selected ) {
                sel[sel.length] = f[productfield].options[i].value;
            }
        }

        single = ( sel.length == 1 );

        // save last_sel before we kill it
            var tmp = last_sel;
        last_sel = sel;

        // this is an optimization: if we've added components, no need
        // to remerge them; just merge the new ones with the existing
        // options.

        if ( ( tmp ) && ( tmp.length < sel.length ) ) {
            sel = fake_diff_array(sel, tmp);
            is_diff = 1;
        }
    }

    // do the actual fill/update
    updateSelect( cpts, sel, f[componentfield], is_diff, single, blank );
}
