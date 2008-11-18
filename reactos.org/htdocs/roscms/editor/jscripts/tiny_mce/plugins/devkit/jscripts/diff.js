// Diff_Match_Patch v1.3
// Computes the difference between two texts to create a patch.
// Applies the patch onto another text, allowing for errors.
// Copyright (C) 2006 Neil Fraser
// http://neil.fraser.name/software/diff_match_patch/

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License (www.gnu.org) for more details.


// Constants.
// Redefine these in your program to override the defaults.

// Number of seconds to map a diff before giving up.  (0 for infinity)
var DIFF_TIMEOUT = 1.0;
// Cost of an empty edit operation in terms of edit characters.
var DIFF_EDIT_COST = 4;
// Tweak the relative importance (0.0 = accuracy, 1.0 = proximity)
var MATCH_BALANCE = 0.5;
// At what point is no match declared (0.0 = perfection, 1.0 = very loose)
var MATCH_THRESHOLD = 0.5;
// The min and max cutoffs used when computing text lengths.
var MATCH_MINLENGTH = 100;
var MATCH_MAXLENGTH = 1000;
// Chunk size for context length.
var PATCH_MARGIN = 4;


  //////////////////////////////////////////////////////////////////////
 //  Diff                                                            //
//////////////////////////////////////////////////////////////////////

// The data structure representing a diff is an array of tuples:
// [[-1, "Hello"], [1, "Goodbye"], [0, " world."]]
// which means: delete "Hello", add "Goodbye" and keep " world."


function diff_main(text1, text2, checklines) {
  // Find the differences between two texts.  Return an array of changes.
  // If checklines is present and false, then don't run a line-level diff first to identify the changed areas.
  // Check for equality (speedup)
  if (text1 == text2)
    return [[0, text1]];

  if (typeof checklines == 'undefined')
    checklines = true;

  var a;
  // Trim off common prefix (speedup)
  a = diff_prefix(text1, text2);
  text1 = a[0];
  text2 = a[1];
  var commonprefix = a[2];

  // Trim off common suffix (speedup)
  a = diff_suffix(text1, text2);
  text1 = a[0];
  text2 = a[1];
  var commonsuffix = a[2];

  var diff, i;
  var longtext = text1.length > text2.length ? text1 : text2;
  var shorttext = text1.length > text2.length ? text2 : text1;

  if (!text1) {  // Just add some text (speedup)
    diff = [[1, text2]];
  } else if (!text2) { // Just delete some text (speedup)
    diff = [[-1, text1]];
  } else if ((i = longtext.indexOf(shorttext)) != -1) {
    // Shorter text is inside the longer text (speedup)
    diff = [[1, longtext.substring(0, i)], [0, shorttext], [1, longtext.substring(i+shorttext.length)]];
    // Swap insertions for deletions if diff is reversed.
    if (text1.length > text2.length)
      diff[0][0] = diff[2][0] = -1;
  } else {
    longtext = shorttext = null; // Garbage collect
    // Check to see if the problem can be split in two.
    var hm = diff_halfmatch(text1, text2);
    if (hm) {
      // A half-match was found, sort out the return data.
      var text1_a = hm[0];
      var text1_b = hm[1];
      var text2_a = hm[2];
      var text2_b = hm[3];
      var mid_common = hm[4];
      // Send both pairs off for separate processing.
      var diff_a = diff_main(text1_a, text2_a, checklines);
      var diff_b = diff_main(text1_b, text2_b, checklines);
      // Merge the results.
      diff = diff_a.concat([[0, mid_common]], diff_b);
    } else {
      // Perform a real diff.
      if (checklines && text1.length + text2.length < 250)
        checklines = false; // Too trivial for the overhead.
      if (checklines) {
        // Scan the text on a line-by-line basis first.
        a = diff_lines2chars(text1, text2);
        text1 = a[0];
        text2 = a[1];
        var linearray = a[2];
      }
      diff = diff_map(text1, text2);
      if (!diff) // No acceptable result.
        diff = [[-1, text1], [1, text2]];
      if (checklines) {
        diff_chars2lines(diff, linearray); // Convert the diff back to original text.
        diff_cleanup_semantic(diff); // Eliminate freak matches (e.g. blank lines)

        // Rediff any replacement blocks, this time on character-by-character basis.
        diff.push([0, '']);  // Add a dummy entry at the end.
        var pointer = 0;
        var count_delete = 0;
        var count_insert = 0;
        var text_delete = '';
        var text_insert = '';
        while(pointer < diff.length) {
          if (diff[pointer][0] == 1) {
            count_insert++;
            text_insert += diff[pointer][1];
          } else if (diff[pointer][0] == -1) {
            count_delete++;
            text_delete += diff[pointer][1];
          } else {  // Upon reaching an equality, check for prior redundancies.
            if (count_delete >= 1 && count_insert >= 1) {
              // Delete the offending records and add the merged ones.
              a = diff_main(text_delete, text_insert, false);
              diff.splice(pointer - count_delete - count_insert, count_delete + count_insert);
              pointer = pointer - count_delete - count_insert;
              for (i=a.length-1; i>=0; i--)
                diff.splice(pointer, 0, a[i]);
              pointer = pointer + a.length;
            }
            count_insert = 0;
            count_delete = 0;
            text_delete = '';
            text_insert = '';
          }
          pointer++;
        }
        diff.pop();  // Remove the dummy entry at the end.

      }
    }
  }

  if (commonprefix)
    diff.unshift([0, commonprefix]);
  if (commonsuffix)
    diff.push([0, commonsuffix]);
  diff_cleanup_merge(diff);
  return diff;
}


function diff_lines2chars(text1, text2) {
  // Split text into an array of strings.
  // Reduce the texts to a string of hashes where each character represents one line.
  var linearray = new Array();  // linearray[4] == "Hello\n"
  var linehash = new Object();  // linehash["Hello\n"] == 4

  // "\x00" is a valid JavaScript character, but the Venkman debugger doesn't like it (bug 335098)
  // So we'll insert a junk entry to avoid generating a null character.
  linearray.push('');

  function diff_lines2chars_munge(text) {
    // My first ever closure!
    var i, line;
    var chars = '';
    while (text) {
      i = text.indexOf('\n');
      if (i == -1)
        i = text.length;
      line = text.substring(0, i+1);
      text = text.substring(i+1);
      if (linehash.hasOwnProperty ? linehash.hasOwnProperty(line) : (linehash[line] !== undefined)) {
        chars += String.fromCharCode(linehash[line]);
      } else {
        linearray.push(line);
        linehash[line] = linearray.length - 1;
        chars += String.fromCharCode(linearray.length - 1);
      }
    }
    return chars;
  }

  var chars1 = diff_lines2chars_munge(text1);
  var chars2 = diff_lines2chars_munge(text2);
  return [chars1, chars2, linearray];
}


function diff_chars2lines(diff, linearray) {
  // Rehydrate the text in a diff from a string of line hashes to real lines of text.
  var chars, text;
  for (var x=0; x<diff.length; x++) {
    chars = diff[x][1];
    text = '';
    for (var y=0; y<chars.length; y++)
      text += linearray[chars.charCodeAt(y)];
    diff[x][1] = text;
  }
}


function diff_map(text1, text2) {
  // Explore the intersection points between the two texts.
  var now = new Date();
  var ms_end = now.getTime() + DIFF_TIMEOUT * 1000; // Don't run for too long.
  var max = (text1.length + text2.length) / 2;
  var v_map1 = new Array();
  var v_map2 = new Array();
  var v1 = new Object();
  var v2 = new Object();
  v1[1] = 0;
  v2[1] = 0;
  var x, y;
  var footstep; // Used to track overlapping paths.
  var footsteps = new Object();
  var done = false;
  var hasOwnProperty = !!(footsteps.hasOwnProperty);
  // If the total number of characters is odd, then the front path will collide with the reverse path.
  var front = (text1.length + text2.length) % 2;
  for (var d=0; d<max; d++) {
    now = new Date();
    if (DIFF_TIMEOUT > 0 && now.getTime() > ms_end) // Timeout reached
      return null;

    // Walk the front path one step.
    v_map1[d] = new Object();
    for (var k=-d; k<=d; k+=2) {
      if (k == -d || k != d && v1[k-1] < v1[k+1])
        x = v1[k+1];
      else
        x = v1[k-1]+1;
      y = x - k;
      footstep = x+","+y;
      if (front && (hasOwnProperty ? footsteps.hasOwnProperty(footstep) : (footsteps[footstep] !== undefined)))
        done = true;
      if (!front)
        footsteps[footstep] = d;
      while (!done && x < text1.length && y < text2.length && text1.charAt(x) == text2.charAt(y)) {
        x++; y++;
        footstep = x+","+y;
        if (front && (hasOwnProperty ? footsteps.hasOwnProperty(footstep) : (footsteps[footstep] !== undefined)))
          done = true;
        if (!front)
          footsteps[footstep] = d;
      }
      v1[k] = x;
      v_map1[d][x+","+y] = true;
      if (done) {
        // Front path ran over reverse path.
        v_map2 = v_map2.slice(0, footsteps[footstep]+1);
        var a = diff_path1(v_map1, text1.substring(0, x), text2.substring(0, y));
        return a.concat(diff_path2(v_map2, text1.substring(x), text2.substring(y)));
      }
    }

    // Walk the reverse path one step.
    v_map2[d] = new Object();
    for (var k=-d; k<=d; k+=2) {
      if (k == -d || k != d && v2[k-1] < v2[k+1])
        x = v2[k+1];
      else
        x = v2[k-1]+1;
      y = x - k;
      footstep = (text1.length-x)+","+(text2.length-y);
      if (!front && (hasOwnProperty ? footsteps.hasOwnProperty(footstep) : (footsteps[footstep] !== undefined)))
        done = true;
      if (front)
        footsteps[footstep] = d;
      while (!done && x < text1.length && y < text2.length && text1.charAt(text1.length-x-1) == text2.charAt(text2.length-y-1)) {
        x++; y++;
        footstep = (text1.length-x)+","+(text2.length-y);
        if (!front && (hasOwnProperty ? footsteps.hasOwnProperty(footstep) : (footsteps[footstep] !== undefined)))
          done = true;
        if (front)
          footsteps[footstep] = d;
      }
      v2[k] = x;
      v_map2[d][x+","+y] = true;
      if (done) {
        // Reverse path ran over front path.
        v_map1 = v_map1.slice(0, footsteps[footstep]+1);
        var a = diff_path1(v_map1, text1.substring(0, text1.length-x), text2.substring(0, text2.length-y));
        return a.concat(diff_path2(v_map2, text1.substring(text1.length-x), text2.substring(text2.length-y)));
      }
    }
  }
  // Number of diffs equals number of characters, no commonality at all.
  return null;
}


function diff_path1(v_map, text1, text2) {
  // Work from the middle back to the start to determine the path.
  var path = [];
  var x = text1.length;
  var y = text2.length;
  var last_op = null;
  for (var d=v_map.length-2; d>=0; d--) {
    while(1) {
      if (v_map[d].hasOwnProperty ? v_map[d].hasOwnProperty((x-1)+","+y) : (v_map[d][(x-1)+","+y] !== undefined)) {
        x--;
        if (last_op === -1)
          path[0][1] = text1.charAt(x) + path[0][1];
        else
          path.unshift([-1, text1.charAt(x)]);
        last_op = -1;
        break;
      } else if (v_map[d].hasOwnProperty ? v_map[d].hasOwnProperty(x+","+(y-1)) : (v_map[d][x+","+(y-1)] !== undefined)) {
        y--;
        if (last_op === 1)
          path[0][1] = text2.charAt(y) + path[0][1];
        else
          path.unshift([1, text2.charAt(y)]);
        last_op = 1;
        break;
      } else {
        x--;
        y--;
        //if (text1.charAt(x) != text2.charAt(y))
        //  return alert("No diagonal.  Can't happen. (diff_path1)");
        if (last_op === 0)
          path[0][1] = text1.charAt(x) + path[0][1];
        else
          path.unshift([0, text1.charAt(x)]);
        last_op = 0;
      }
    }
  }
  return path;
}


function diff_path2(v_map, text1, text2) {
  // Work from the middle back to the end to determine the path.
  var path = [];
  var x = text1.length;
  var y = text2.length;
  var last_op = null;
  for (var d=v_map.length-2; d>=0; d--) {
    while(1) {
      if (v_map[d].hasOwnProperty ? v_map[d].hasOwnProperty((x-1)+","+y) : (v_map[d][(x-1)+","+y] !== undefined)) {
        x--;
        if (last_op === -1)
          path[path.length-1][1] += text1.charAt(text1.length-x-1);
        else
          path.push([-1, text1.charAt(text1.length-x-1)]);
        last_op = -1;
        break;
      } else if (v_map[d].hasOwnProperty ? v_map[d].hasOwnProperty(x+","+(y-1)) : (v_map[d][x+","+(y-1)] !== undefined)) {
        y--;
        if (last_op === 1)
          path[path.length-1][1] += text2.charAt(text2.length-y-1);
        else
          path.push([1, text2.charAt(text2.length-y-1)]);
        last_op = 1;
        break;
      } else {
        x--;
        y--;
        //if (text1.charAt(text1.length-x-1) != text2.charAt(text2.length-y-1))
        //  return alert("No diagonal.  Can't happen. (diff_path2)");
        if (last_op === 0)
          path[path.length-1][1] += text1.charAt(text1.length-x-1);
        else
          path.push([0, text1.charAt(text1.length-x-1)]);
        last_op = 0;
      }
    }
  }
  return path;
}


function diff_prefix(text1, text2) {
  // Trim off common prefix
  var pointermin = 0;
  var pointermax = Math.min(text1.length, text2.length);
  var pointermid = pointermax;
  while(pointermin < pointermid) {
    if (text1.substring(0, pointermid) == text2.substring(0, pointermid))
      pointermin = pointermid;
    else
      pointermax = pointermid;
    pointermid = Math.floor((pointermax - pointermin) / 2 + pointermin);
  }
  var commonprefix = text1.substring(0, pointermid);
  text1 = text1.substring(pointermid);
  text2 = text2.substring(pointermid);
  return [text1, text2, commonprefix];
}


function diff_suffix(text1, text2) {
  // Trim off common suffix
  var pointermin = 0;
  var pointermax = Math.min(text1.length, text2.length);
  var pointermid = pointermax;
  while(pointermin < pointermid) {
    if (text1.substring(text1.length-pointermid) == text2.substring(text2.length-pointermid))
      pointermin = pointermid;
    else
      pointermax = pointermid;
    pointermid = Math.floor((pointermax - pointermin) / 2 + pointermin);
  }
  var commonsuffix = text1.substring(text1.length-pointermid);
  text1 = text1.substring(0, text1.length-pointermid);
  text2 = text2.substring(0, text2.length-pointermid);
  return [text1, text2, commonsuffix];
}


function diff_halfmatch(text1, text2) {
  // Do the two texts share a substring which is at least half the length of the longer text?
  var longtext = text1.length > text2.length ? text1 : text2;
  var shorttext = text1.length > text2.length ? text2 : text1;
  if (longtext.length < 10 || shorttext.length < 1)
    return null; // Pointless.

  function diff_halfmatch_i(longtext, shorttext, i) {
    // Start with a 1/4 length substring at position i as a seed.
    var seed = longtext.substring(i, i+Math.floor(longtext.length/4));
    var j = -1;
    var best_common = '';
    var best_longtext_a, best_longtext_b, best_shorttext_a, best_shorttext_b;
    while ((j = shorttext.indexOf(seed, j+1)) != -1) {
      var my_prefix = diff_prefix(longtext.substring(i), shorttext.substring(j));
      var my_suffix = diff_suffix(longtext.substring(0, i), shorttext.substring(0, j));
      if (best_common.length < (my_suffix[2] + my_prefix[2]).length) {
        best_common = my_suffix[2] + my_prefix[2];
        best_longtext_a = my_suffix[0];
        best_longtext_b = my_prefix[0];
        best_shorttext_a = my_suffix[1];
        best_shorttext_b = my_prefix[1];
      }
    }
    if (best_common.length >= longtext.length/2)
      return [best_longtext_a, best_longtext_b, best_shorttext_a, best_shorttext_b, best_common];
    else
      return null;
  }

  // First check if the second quarter is the seed for a half-match.
  var hm1 = diff_halfmatch_i(longtext, shorttext, Math.ceil(longtext.length/4));
  // Check again based on the third quarter.
  var hm2 = diff_halfmatch_i(longtext, shorttext, Math.ceil(longtext.length/2));
  var hm;
  if (!hm1 && !hm2)
    return null;
  else if (!hm2)
    hm = hm1;
  else if (!hm1)
    hm = hm2;
  else // Both matched.  Select the longest.
    hm = hm1[4].length > hm2[4].length ? hm1 : hm2;

  // A half-match was found, sort out the return data.
  if (text1.length > text2.length) {
    var text1_a = hm[0];
    var text1_b = hm[1];
    var text2_a = hm[2];
    var text2_b = hm[3];
  } else {
    var text2_a = hm[0];
    var text2_b = hm[1];
    var text1_a = hm[2];
    var text1_b = hm[3];
  }
  var mid_common = hm[4];
  return [text1_a, text1_b, text2_a, text2_b, mid_common];
}


function diff_cleanup_semantic(diff) {
  // Reduce the number of edits by eliminating semantically trivial equalities.
  var changes = false;
  var equalities = []; // Stack of indices where equalities are found.
  var lastequality = null; // Always equal to equalities[equalities.length-1][1]
  var pointer = 0; // Index of current position.
  var length_changes1 = 0; // Number of characters that changed prior to the equality.
  var length_changes2 = 0; // Number of characters that changed after the equality.
  while (pointer < diff.length) {
    if (diff[pointer][0] == 0) { // equality found
      equalities.push(pointer);
      length_changes1 = length_changes2;
      length_changes2 = 0;
      lastequality = diff[pointer][1];
    } else { // an insertion or deletion
      length_changes2 += diff[pointer][1].length;
      if (lastequality != null && (lastequality.length <= length_changes1) && (lastequality.length <= length_changes2)) {
        //alert("Splitting: '"+lastequality+"'");
        diff.splice(equalities[equalities.length-1], 0, [-1, lastequality]); // Duplicate record
        diff[equalities[equalities.length-1]+1][0] = 1; // Change second copy to insert.
        equalities.pop();  // Throw away the equality we just deleted;
        equalities.pop();  // Throw away the previous equality;
        pointer = equalities.length ? equalities[equalities.length-1] : -1;
        length_changes1 = 0; // Reset the counters.
        length_changes2 = 0;
        lastequality = null;
        changes = true;
      }
    }
    pointer++;
  }

  if (changes)
    diff_cleanup_merge(diff);
}


function diff_cleanup_efficiency(diff) {
  // Reduce the number of edits by eliminating operationally trivial equalities.
  var changes = false;
  var equalities = []; // Stack of indices where equalities are found.
  var lastequality = ''; // Always equal to equalities[equalities.length-1][1]
  var pointer = 0; // Index of current position.
  var pre_ins = false; // Is there an insertion operation before the last equality.
  var pre_del = false; // Is there an deletion operation before the last equality.
  var post_ins = false; // Is there an insertion operation after the last equality.
  var post_del = false; // Is there an deletion operation after the last equality.
  while (pointer < diff.length) {
    if (diff[pointer][0] == 0) { // equality found
      if (diff[pointer][1].length < DIFF_EDIT_COST && (post_ins || post_del)) {
        // Candidate found.
        equalities.push(pointer);
        pre_ins = post_ins;
        pre_del = post_del;
        lastequality = diff[pointer][1];
      } else {
        // Not a candidate, and can never become one.
        equalities = [];
        lastequality = '';
      }
      post_ins = post_del = false;
    } else { // an insertion or deletion
      if (diff[pointer][0] == -1)
        post_del = true;
      else
        post_ins = true;
      // Five types to be split:
      // <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
      // <ins>A</ins>X<ins>C</ins><del>D</del>
      // <ins>A</ins><del>B</del>X<ins>C</ins>
      // <ins>A</del>X<ins>C</ins><del>D</del>
      // <ins>A</ins><del>B</del>X<del>C</del>
      if (lastequality && ((pre_ins && pre_del && post_ins && post_del) || ((lastequality.length < DIFF_EDIT_COST/2) && (pre_ins + pre_del + post_ins + post_del) == 3))) {
        //alert("Splitting: '"+lastequality+"'");
        diff.splice(equalities[equalities.length-1], 0, [-1, lastequality]); // Duplicate record
        diff[equalities[equalities.length-1]+1][0] = 1; // Change second copy to insert.
        equalities.pop();  // Throw away the equality we just deleted;
        lastequality = '';
        if (pre_ins && pre_del) {
          // No changes made which could affect previous entry, keep going.
          post_ins = post_del = true;
          equalities = [];
        } else {
          equalities.pop();  // Throw away the previous equality;
          pointer = equalities.length ? equalities[equalities.length-1] : -1;
          post_ins = post_del = false;
        }
        changes = true;
      }
    }
    pointer++;
  }

  if (changes)
    diff_cleanup_merge(diff);
}


function diff_cleanup_merge(diff) {
  // Reorder and merge like edit sections.  Merge equalities.
  // Any edit section can move as long as it doesn't cross an equality.
  diff.push([0, '']);  // Add a dummy entry at the end.
  var pointer = 0;
  var count_delete = 0;
  var count_insert = 0;
  var text_delete = '';
  var text_insert = '';
  var record_insert, record_delete;
  var my_xfix;
  while(pointer < diff.length) {
    if (diff[pointer][0] == 1) {
      count_insert++;
      text_insert += diff[pointer][1];
      pointer++;
    } else if (diff[pointer][0] == -1) {
      count_delete++;
      text_delete += diff[pointer][1];
      pointer++;
    } else {  // Upon reaching an equality, check for prior redundancies.
      if (count_delete > 1 || count_insert > 1) {
        if (count_delete > 1 && count_insert > 1) {
          // Factor out any common prefixies.
          my_xfix = diff_prefix(text_insert, text_delete);
          if (my_xfix[2] != '') {
            if ((pointer - count_delete - count_insert) > 0 && diff[pointer - count_delete - count_insert - 1][0] == 0) {
              text_insert = my_xfix[0];
              text_delete = my_xfix[1];
              diff[pointer - count_delete - count_insert - 1][1] += my_xfix[2];
            }
          }
          // Factor out any common suffixies.
          my_xfix = diff_suffix(text_insert, text_delete);
          if (my_xfix[2] != '') {
            text_insert = my_xfix[0];
            text_delete = my_xfix[1];
            diff[pointer][1] = my_xfix[2] + diff[pointer][1];
          }
        }
        // Delete the offending records and add the merged ones.
        if (count_delete == 0)
          diff.splice(pointer - count_delete - count_insert, count_delete + count_insert, [1, text_insert]);
        else if (count_insert == 0)
          diff.splice(pointer - count_delete - count_insert, count_delete + count_insert, [-1, text_delete]);
        else
          diff.splice(pointer - count_delete - count_insert, count_delete + count_insert, [-1, text_delete], [1, text_insert]);
        pointer = pointer - count_delete - count_insert + (count_delete ? 1 : 0) + (count_insert ? 1 : 0) + 1;
      } else if (pointer != 0 && diff[pointer-1][0] == 0) {
        // Merge this equality with the previous one.
        diff[pointer-1][1] += diff[pointer][1];
        diff.splice(pointer, 1);
      } else {
        pointer++;
      }
      count_insert = 0;
      count_delete = 0;
      text_delete = '';
      text_insert = '';
    }
  }
  if (diff[diff.length-1][1] == '')
    diff.pop();  // Remove the dummy entry at the end.
}


function diff_addindex(diff) {
  // Add an index to each tuple, represents where the tuple is located in text2.
  // e.g. [[-1, 'h', 0], [1, 'c', 0], [0, 'at', 1]]
  var i = 0;
  for (var x=0; x<diff.length; x++) {
    diff[x].push(i);
    if (diff[x][0] != -1)
      i += diff[x][1].length;
  }
}


function diff_xindex(diff, loc) {
  // loc is a location in text1, compute and return the equivalent location in text2.
  // e.g. "The cat" vs "The big cat", 1->1, 5->8
  var chars1 = 0;
  var chars2 = 0;
  var last_chars1 = 0;
  var last_chars2 = 0;
  for (var x=0; x<diff.length; x++) {
    if (diff[x][0] != 1) // Equality or deletion.
      chars1 += diff[x][1].length;
    if (diff[x][0] != -1) // Equality or insertion.
      chars2 += diff[x][1].length;
    if (chars1 > loc) // Overshot the location.
      break;
    last_chars1 = chars1;
    last_chars2 = chars2;
  }
  if (diff.length != x && diff[x][0] == -1) // The location was deleted.
    return last_chars2;
  // Add the remaining character length.
  return last_chars2 + (loc - last_chars1);
}


function diff_prettyhtml(diff) {
  // Convert a diff array into a pretty HTML report.
  diff_addindex(diff);
  var html = '';
  for (var x=0; x<diff.length; x++) {
    var m = diff[x][0]; // Mode (-1=delete, 0=copy, 1=add)
    var t = diff[x][1]; // Text of change.
    var i = diff[x][2]; // Index of change.
    t = t.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    t = t.replace(/\n/g, "&para;<BR>");
    if (m == -1)
      html += "<DEL STYLE='background:#FFE6E6;' TITLE='i="+i+"'>"+t+"</DEL>";
    else if (m == 1)
      html += "<INS STYLE='background:#E6FFE6;' TITLE='i="+i+"'>"+t+"</INS>";
    else
      html += "<SPAN TITLE='i="+i+"'>"+t+"</SPAN>";
  }
  return html;
}


  //////////////////////////////////////////////////////////////////////
 //  Match                                                           //
//////////////////////////////////////////////////////////////////////


function match_getmaxbits() {
  // Compute the number of bits in an int.
  // The normal answer for JavaScript is 32.
  var maxbits = 0;
  var oldi = 1;
  var newi = 2;
  while (oldi != newi) {
    maxbits++;
    oldi = newi;
    newi = newi << 1;
  }
  return maxbits;
}
var MATCH_MAXBITS = match_getmaxbits();


function match_main(text, pattern, loc) {
  // Locate the best instance of 'pattern' in 'text' near 'loc'.
  loc = Math.max(0, Math.min(loc, text.length-pattern.length));
  if (text == pattern) {
    // Shortcut (potentially not guaranteed by the algorithm)
    return 0;
  } else if (text.length == 0) {
    // Nothing to match.
    return null;
  } else if (text.substring(loc, loc + pattern.length) == pattern) {
    // Perfect match at the perfect spot!  (Includes case of null pattern)
    return loc;
  } else {
    // Do a fuzzy compare.
    var match = match_bitap(text, pattern, loc);
    return match;
  }
}


function match_bitap(text, pattern, loc) {
  // Locate the best instance of 'pattern' in 'text' near 'loc' using the Bitap algorithm.
  if (pattern.length > MATCH_MAXBITS)
    return alert("Pattern too long for this browser.");

  // Initialise the alphabet.
  var s = match_alphabet(pattern);

  var score_text_length = text.length;
  // Coerce the text length between reasonable maximums and minimums.
  score_text_length = Math.max(score_text_length, MATCH_MINLENGTH);
  score_text_length = Math.min(score_text_length, MATCH_MAXLENGTH);

  function match_bitap_score (e, x) {
    // Compute and return the score for a match with e errors and x location.
    var d = Math.abs(loc-x);
    return (e / pattern.length / MATCH_BALANCE) + (d / score_text_length / (1.0 - MATCH_BALANCE));
  }

  // Highest score beyond which we give up.
  var score_threshold = MATCH_THRESHOLD;
  // Is there a nearby exact match? (speedup)
  var best_loc = text.indexOf(pattern, loc);
  if (best_loc != -1)
    score_threshold = Math.min(match_bitap_score(0, best_loc), score_threshold);
  // What about in the other direction? (speedup)
  best_loc = text.lastIndexOf(pattern, loc+pattern.length);
  if (best_loc != -1)
    score_threshold = Math.min(match_bitap_score(0, best_loc), score_threshold);

  // Initialise the bit arrays.
  var r = Array();
  var d = -1;
  var matchmask = Math.pow(2, pattern.length-1);
  best_loc = null;

  var bin_min, bin_mid;
  var bin_max = Math.max(loc+loc, text.length);
  var last_rd;
  for (var d=0; d<pattern.length; d++) {
    // Scan for the best match; each iteration allows for one more error.
    var rd = Array(text.length);

    // Run a binary search to determine how far from 'loc' we can stray at this error level.
    bin_min = loc;
    bin_mid = bin_max;
    while(bin_min < bin_mid) {
      if (match_bitap_score(d, bin_mid) < score_threshold)
        bin_min = bin_mid;
      else
        bin_max = bin_mid;
      bin_mid = Math.floor((bin_max - bin_min) / 2 + bin_min);
    }
    bin_max = bin_mid; // Use the result from this iteration as the maximum for the next.
    var start = Math.max(0, loc - (bin_mid - loc) - 1);
    var finish = Math.min(text.length-1, pattern.length + bin_mid);

    if (text.charAt(finish) == pattern.charAt(pattern.length-1))
      rd[finish] = Math.pow(2, d+1)-1;
    else
      rd[finish] = Math.pow(2, d)-1;
    for (var j=finish-1; j>=start; j--) {
      // The alphabet (s) is a sparse hash, so the following lines generate warnings.
      if (d == 0) // First pass: exact match.
        rd[j] = ((rd[j+1] << 1) | 1) & s[text.charAt(j)];
      else // Subsequent passes: fuzzy match.
        rd[j] = ((rd[j+1] << 1) | 1) & s[text.charAt(j)] | ((last_rd[j+1] << 1) | 1) | ((last_rd[j] << 1) | 1) | last_rd[j+1];
      if (rd[j] & matchmask) {
        var score = match_bitap_score(d, j);
        // This match will almost certainly be better than any existing match.  But check anyway.
        if (score <= score_threshold) {
          // Told you so.
          score_threshold = score;
          best_loc = j;
          if (j > loc) {
            // When passing loc, don't exceed our current distance from loc.
            start = Math.max(0, loc - (j - loc));
          } else {
            // Already passed loc, downhill from here on in.
            break;
          }
        }
      }
    }
    if (match_bitap_score(d+1, loc) > score_threshold) // No hope for a (better) match at greater error levels.
      break;
    last_rd = rd;
  }
  return best_loc;
}


function match_alphabet(pattern) {
  // Initialise the alphabet for the Bitap algorithm.
  var s = Object();
  for (var i=0; i<pattern.length; i++)
    s[pattern.charAt(i)] = 0;
  for (var i=0; i<pattern.length; i++)
    s[pattern.charAt(i)] |= Math.pow(2, pattern.length-i-1);
  return s;
}


  //////////////////////////////////////////////////////////////////////
 //  Patch                                                           //
//////////////////////////////////////////////////////////////////////


function patch_obj() {
  // Constructor for a patch object.
  this.diffs = [];
  this.start1 = null;
  this.start2 = null;
  this.length1 = 0;
  this.length2 = 0;

  this.toString = function() {
    // Emmulate GNU diff's format.
    // Header: @@ -382,8 +481,9 @@
    // Indicies are printed as 1-based, not 0-based.
    var coords1, coords2;
    if (this.length1 == 0)
      coords1 = this.start1+",0";
    else if (this.length1 == 1)
      coords1 = this.start1+1;
    else
      coords1 = (this.start1+1)+","+this.length1;
    if (this.length2 == 0)
      coords2 = this.start2+",0";
    else if (this.length2 == 1)
      coords2 = this.start2+1;
    else
      coords2 = (this.start2+1)+","+this.length2;
    var txt = "@@ -"+coords1+" +"+coords2+" @@\n";
    // Escape the body of the patch with %xx notation.
    for (var x=0; x<this.diffs.length; x++)
      txt += ("- +".charAt(this.diffs[x][0]+1)) + encodeURI(this.diffs[x][1]) + "\n";
    return txt.replace(/%20/g, ' ');
  }

  this.text1 = function() {
    // Compute and return the source text (all equalities and deletions).
    var txt = '';
    for (var x=0; x<this.diffs.length; x++)
      if (this.diffs[x][0] == 0 || this.diffs[x][0] == -1)
        txt += this.diffs[x][1];
    return txt;
  }

  this.text2 = function() {
    // Compute and return the destination text (all equalities and insertions).
    var txt = '';
    for (var x=0; x<this.diffs.length; x++)
      if (this.diffs[x][0] == 0 || this.diffs[x][0] == 1)
        txt += this.diffs[x][1];
    return txt;
  }
}


function patch_addcontext(patch, text) {
  var pattern = text.substring(patch.start2, patch.start2+patch.length1);
  var padding = 0;
  // Increase the context until we're unique (but don't let the pattern expand beyond MATCH_MAXBITS).
  while (text.indexOf(pattern) != text.lastIndexOf(pattern) && pattern.length < MATCH_MAXBITS-PATCH_MARGIN-PATCH_MARGIN) {
    padding += PATCH_MARGIN;
    pattern = text.substring(patch.start2 - padding, patch.start2+patch.length1 + padding);
  }
  // Add one chunk for good luck.
  padding += PATCH_MARGIN;
  // Add the prefix.
  var prefix = text.substring(patch.start2 - padding, patch.start2);
  if (prefix != '')
    patch.diffs.unshift([0, prefix]);
  // Add the suffix
  var suffix = text.substring(patch.start2+patch.length1, patch.start2+patch.length1 + padding);
  if (suffix != '')
    patch.diffs.push([0, suffix]);

  // Roll back the start points.
  patch.start1 -= prefix.length;
  patch.start2 -= prefix.length;
  // Extend the lengths.
  patch.length1 += prefix.length + suffix.length;
  patch.length2 += prefix.length + suffix.length;
}


function patch_make(text1, text2, diff) {
  // Compute a list of patches to turn text1 into text2.
  // Use diff if provided, otherwise compute it ourselves.
  if (typeof diff == 'undefined') {
    diff = diff_main(text1, text2, true);
    if (diff.length > 2) {
      diff_cleanup_semantic(diff);
      diff_cleanup_efficiency(diff);
    }
  }
  if (diff.length == 0)
    return []; // Get rid of the null case.
  var patches = [];
  var patch = new patch_obj();
  var char_count1 = 0; // Number of characters into the text1 string.
  var char_count2 = 0; // Number of characters into the text2 string.
  var last_type = null;
  var prepatch_text = text1; // Recreate the patches to determine context info.
  var postpatch_text = text1;
  for (var x=0; x<diff.length; x++) {
    var diff_type = diff[x][0];
    var diff_text = diff[x][1];

    if (patch.diffs.length == 0 && diff_type != 0) {
      // A new patch starts here.
      patch.start1 = char_count1;
      patch.start2 = char_count2;
    }

    if (diff_type == 1) {
      // Insertion
      patch.diffs.push(diff[x]);
      patch.length2 += diff_text.length;
      postpatch_text = postpatch_text.substring(0, char_count2) + diff_text + postpatch_text.substring(char_count2);
    } else if (diff_type == -1) {
      // Deletion.
      patch.length1 += diff_text.length;
      patch.diffs.push(diff[x]);
      postpatch_text = postpatch_text.substring(0, char_count2) + postpatch_text.substring(char_count2 + diff_text.length);
    } else if (diff_type == 0 && diff_text.length <= 2*PATCH_MARGIN && patch.diffs.length != 0 && diff.length != x+1) {
      // Small equality inside a patch.
      patch.diffs.push(diff[x]);
      patch.length1 += diff_text.length;
      patch.length2 += diff_text.length;
    }

    last_type = diff_type;
    if (diff_type == 0 && diff_text.length >= 2*PATCH_MARGIN) {
      // Time for a new patch.
      if (patch.diffs.length != 0) {
        patch_addcontext(patch, prepatch_text);
        patches.push(patch);
        var patch = new patch_obj();
        last_type = null;
        prepatch_text = postpatch_text;
      }
    }

    // Update the current character count.
    if (diff_type != 1)
      char_count1 += diff_text.length;
    if (diff_type != -1)
      char_count2 += diff_text.length;
  }
  // Pick up the leftover patch if not empty.
  if (patch.diffs.length != 0) {
    patch_addcontext(patch, prepatch_text);
    patches.push(patch);
  }

  return patches;
}


function patch_apply(patches, text) {
  // Merge a set of patches onto the text.
  // Return a patched text, as well as a list of true/false values indicating which patches were applied.
  patch_splitmax(patches);
  var results = [];
  var delta = 0;
  var expected_loc, start_loc;
  var text1, text2;
  var diff, mod, index1, index2;
  for (var x=0; x<patches.length; x++) {
    expected_loc = patches[x].start2 + delta;
    text1 = patches[x].text1();
    start_loc = match_main(text, text1, expected_loc);
    if (start_loc == null) {
      // No match found.  :(
      results.push(false);
    } else {
      // Found a match.  :)
      results.push(true);
      delta = start_loc - expected_loc;
      text2 = text.substring(start_loc, start_loc + text1.length);
      if (text1 == text2) {
        // Perfect match, just shove the replacement text in.
        text = text.substring(0, start_loc) + patches[x].text2() + text.substring(start_loc + text1.length);
      } else {
        // Imperfect match.  Run a diff to get a framework of equivalent indicies.
        diff = diff_main(text1, text2, false);
        index1 = 0;
        for (var y=0; y<patches[x].diffs.length; y++) {
          mod = patches[x].diffs[y];
          if (mod[0] != 0)
            index2 = diff_xindex(diff, index1);
          if (mod[0] == 1) // Insertion
            text = text.substring(0, start_loc + index2) + mod[1] + text.substring(start_loc + index2);
          else if (mod[0] == -1) // Deletion
            text = text.substring(0, start_loc + index2) + text.substring(start_loc + diff_xindex(diff, index1 + mod[1].length));
          if (mod[0] != -1)
            index1 += mod[1].length;
        }
      }
    }
  }
  return [text, results];
}


function patch_splitmax(patches) {
  // Look through the patches and break up any which are longer than the maximum limit of the match algorithm.
  var bigpatch, patch, patch_size, start1, start2, diff_type, diff_text, precontext, postcontext, empty;
  for (var x=0; x<patches.length; x++) {
    if (patches[x].length1 > MATCH_MAXBITS) {
      bigpatch = patches[x];
      // Remove the big old patch.
      patches.splice(x, 1);
      patch_size = MATCH_MAXBITS;
      start1 = bigpatch.start1;
      start2 = bigpatch.start2;
      precontext = '';
      while (bigpatch.diffs.length != 0) {
        // Create one of several smaller patches.
        patch = new patch_obj();
        empty = true;
        patch.start1 = start1 - precontext.length;
        patch.start2 = start2 - precontext.length;
        if (precontext  != '') {
          patch.length1 = patch.length2 = precontext.length;
          patch.diffs.push([0, precontext]);
        }
        while (bigpatch.diffs.length != 0 && patch.length1 < patch_size - PATCH_MARGIN) {
          diff_type = bigpatch.diffs[0][0];
          diff_text = bigpatch.diffs[0][1];
          if (diff_type == 1) {
            // Insertions are harmless.
            patch.length2 += diff_text.length;
            start2 += diff_text.length;
            patch.diffs.push(bigpatch.diffs.shift());
            empty = false;
          } else {
            // Deletion or equality.  Only take as much as we can stomach.
            diff_text = diff_text.substring(0, patch_size - patch.length1 - PATCH_MARGIN);
            patch.length1 += diff_text.length;
            start1 += diff_text.length;
            if (diff_type == 0) {
              patch.length2 += diff_text.length;
              start2 += diff_text.length;
            } else {
              empty = false;
            }
            patch.diffs.push([diff_type, diff_text]);
            if (diff_text == bigpatch.diffs[0][1])
              bigpatch.diffs.shift();
            else
              bigpatch.diffs[0][1] = bigpatch.diffs[0][1].substring(diff_text.length);
          }
        }
        // Compute the head context for the next patch.
        precontext = patch.text2();
        precontext = precontext.substring(precontext.length - PATCH_MARGIN);
        // Append the end context for this patch.
        postcontext = bigpatch.text1().substring(0, PATCH_MARGIN);
        if (postcontext  != '') {
          patch.length1 += postcontext.length;
          patch.length2 += postcontext.length;
          if (patch.diffs.length > 0 && patch.diffs[patch.diffs.length-1][0] == 0)
            patch.diffs[patch.diffs.length-1][1] += postcontext;
          else
            patch.diffs.push([0, postcontext]);
        }
        if (!empty)
          patches.splice(x++, 0, patch);
      }
    }
  }
}


function patch_totext(patches) {
  // Take a list of patches and return a textual representation.
  var text = '';
  for (var x=0; x<patches.length; x++)
    text += patches[x];
  return text;
}


function patch_fromtext(text) {
  // Take a textual representation of patches and return a list of patch objects.
  var patches = [];
  text = text.split('\n');
  var patch, m, chars1, chars2, sign, line;
  while (text.length != 0) {
    m = text[0].match(/^@@ -(\d+),?(\d*) \+(\d+),?(\d*) @@$/);
    if (!m)
      return alert("Invalid patch string:\n"+text[0]);
    patch = new patch_obj();
    patches.push(patch);
    patch.start1 = parseInt(m[1]);
    if (m[2] == '') {
      patch.start1--;
      patch.length1 = 1;
    } else if (m[2] == '0') {
      patch.length1 = 0;
    } else {
      patch.start1--;
      patch.length1 = parseInt(m[2]);
    }

    patch.start2 = parseInt(m[3]);
    if (m[4] == '') {
      patch.start2--;
      patch.length2 = 1;
    } else if (m[4] == '0') {
      patch.length2 = 0;
    } else {
      patch.start2--;
      patch.length2 = parseInt(m[4]);
    }
    text.shift();

    while (text.length != 0) {
      sign = text[0].charAt(0);
      line = decodeURIComponent(text[0].substring(1));
      if (sign == '-') {
        // Deletion.
        patch.diffs.push([-1, line]);
      } else if (sign == '+') {
        // Insertion.
        patch.diffs.push([1, line]);
      } else if (sign == ' ') {
        // Minor equality.
        patch.diffs.push([0, line]);
      } else if (sign == '@') {
        // Start of next patch.
        break;
      } else if (sign == '') {
        // Blank line?  Whatever.
      } else {
        // WTF?
        return alert("Invalid patch mode: '"+sign+"'\n"+line);
      }
      text.shift();
    }
  }
  return patches;
}

// EOF
