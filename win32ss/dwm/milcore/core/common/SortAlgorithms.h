// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Generic sorting algorithms for use in MILCore
//
//------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------
//
//  Function:   ArrayInsertionSort
//
//  Synopsis:   Simple in-place sorting method that should be used to sort 
//              arrays with few elements or arrays that are mostly sorted.
//              This is a stable sort that preserves the original order
//              of equivalent elements.
//
//              If the array elements are already sorted, the performance 
//              of Insertion Sort is O(N).  In the worst case where the  
//              array is in reverse-order, InsertionSort can become O(N^2).
//
// Notes:       This is a template method which operates on arrays of T.  To 
//              sort a type with this method, it must support the assignment
//              operator (operator =) and the greater-than operator 
//              (operator >).  Naturally, these operators are provided for
//              built-in types, but may need to be explicitly implemented to 
//              sort arrays of struct's or class's.
//
// Example Usage:
//
//              int values[5] = { 0, 2, 3, 1, 5 };
//
//              ArrayInsertionSort(values, 5);
//
// Returns:    VOID
//
//------------------------------------------------------------------------

template <typename T>
VOID
ArrayInsertionSort(
    __inout_ecount_full(uElementCount) T *pElements,  // Array elements to sort
    UINT uElementCount                                // Number of elements in
                                                      // pElements
    )
{    
    UINT i, j;

    // This algorithm works by creating a subarray of sorted elements at
    // the beginning of pElements.  It inserts each element after this 
    // subarray into the correct position in the subarray.
    
    // Starting at the second element, insert each element in the sorted
    // subarray of elements that proceed it.
    for (i = 1; i < uElementCount; i++)
    {
        // Store temporary copy of value being inserted
        T insertValue = pElements[i];

        // Make room for insertValue by moving the elements larger than insertValue
        // up one index.
        //
        for (   // Start at the end of the sorted subarray
                j = i;         
                // While we haven't reached the beginning of the array and
                (j > 0) &&       
                // The current element is larger than the value we need to insert
                (pElements[j-1] > insertValue);
                // Decrement the current element index
                j--
            )
            {
                // Shift the current element up one index.
                pElements[j] = pElements[j-1];                
            }

        // Assign insertValue to its position in the sorted subarray.
        //
        // j will be the position of insertValue.
        //
        // To avoid checking, we always write insertValue to pElements[j], even
        // if insertValue was previously at j.
        pElements[j] = insertValue;                
    }
}


