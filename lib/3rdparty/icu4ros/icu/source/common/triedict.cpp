/**
 *******************************************************************************
 * Copyright (C) 2006-2007, International Business Machines Corporation        *
 * and others. All Rights Reserved.                                            *
 *******************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "triedict.h"
#include "unicode/chariter.h"
#include "unicode/uchriter.h"
#include "unicode/strenum.h"
#include "unicode/uenum.h"
#include "unicode/udata.h"
#include "cmemory.h"
#include "udataswp.h"
#include "uvector.h"
#include "uvectr32.h"
#include "uarrsort.h"

//#define DEBUG_TRIE_DICT 1

#ifdef DEBUG_TRIE_DICT
#include <sys/times.h>
#include <limits.h>
#include <stdio.h>
#endif

U_NAMESPACE_BEGIN

/*******************************************************************
 * TrieWordDictionary
 */

TrieWordDictionary::TrieWordDictionary() {
}

TrieWordDictionary::~TrieWordDictionary() {
}

/*******************************************************************
 * MutableTrieDictionary
 */

// Node structure for the ternary, uncompressed trie
struct TernaryNode : public UMemory {
    UChar       ch;         // UTF-16 code unit
    uint16_t    flags;      // Flag word
    TernaryNode *low;       // Less-than link
    TernaryNode *equal;     // Equal link
    TernaryNode *high;      // Greater-than link

    TernaryNode(UChar uc);
    ~TernaryNode();
};

enum MutableTrieNodeFlags {
    kEndsWord = 0x0001      // This node marks the end of a valid word
};

inline
TernaryNode::TernaryNode(UChar uc) {
    ch = uc;
    flags = 0;
    low = NULL;
    equal = NULL;
    high = NULL;
}

// Not inline since it's recursive
TernaryNode::~TernaryNode() {
    delete low;
    delete equal;
    delete high;
}

MutableTrieDictionary::MutableTrieDictionary( UChar median, UErrorCode &status ) {
    // Start the trie off with something. Having the root node already present
    // cuts a special case out of the search/insertion functions.
    // Making it a median character cuts the worse case for searches from
    // 4x a balanced trie to 2x a balanced trie. It's best to choose something
    // that starts a word that is midway in the list.
    fTrie = new TernaryNode(median);
    if (fTrie == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    fIter = utext_openUChars(NULL, NULL, 0, &status);
    if (U_SUCCESS(status) && fIter == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

MutableTrieDictionary::MutableTrieDictionary( UErrorCode &status ) {
    fTrie = NULL;
    fIter = utext_openUChars(NULL, NULL, 0, &status);
    if (U_SUCCESS(status) && fIter == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

MutableTrieDictionary::~MutableTrieDictionary() {
    delete fTrie;
    utext_close(fIter);
}

int32_t
MutableTrieDictionary::search( UText *text,
                                   int32_t maxLength,
                                   int32_t *lengths,
                                   int &count,
                                   int limit,
                                   TernaryNode *&parent,
                                   UBool &pMatched ) const {
    // TODO: current implementation works in UTF-16 space
    const TernaryNode *up = NULL;
    const TernaryNode *p = fTrie;
    int mycount = 0;
    pMatched = TRUE;
    int i;

    UChar uc = utext_current32(text);
    for (i = 0; i < maxLength && p != NULL; ++i) {
        while (p != NULL) {
            if (uc < p->ch) {
                up = p;
                p = p->low;
            }
            else if (uc == p->ch) {
                break;
            }
            else {
                up = p;
                p = p->high;
            }
        }
        if (p == NULL) {
            pMatched = FALSE;
            break;
        }
        // Must be equal to get here
        if (limit > 0 && (p->flags & kEndsWord)) {
            lengths[mycount++] = i+1;
            --limit;
        }
        up = p;
        p = p->equal;
        uc = utext_next32(text);
        uc = utext_current32(text);
    }
    
    // Note that there is no way to reach here with up == 0 unless
    // maxLength is 0 coming in.
    parent = (TernaryNode *)up;
    count = mycount;
    return i;
}

void
MutableTrieDictionary::addWord( const UChar *word,
                                int32_t length,
                                UErrorCode &status ) {
#if 0
    if (length <= 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
#endif
    TernaryNode *parent;
    UBool pMatched;
    int count;
    fIter = utext_openUChars(fIter, word, length, &status);
    
    int matched;
    matched = search(fIter, length, NULL, count, 0, parent, pMatched);
    
    while (matched++ < length) {
        UChar32 uc = utext_next32(fIter);  // TODO:  supplemetary support?
        U_ASSERT(uc != U_SENTINEL);
        TernaryNode *newNode = new TernaryNode(uc);
        if (newNode == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        if (pMatched) {
            parent->equal = newNode;
        }
        else {
            pMatched = TRUE;
            if (uc < parent->ch) {
                parent->low = newNode;
            }
            else {
                parent->high = newNode;
            }
        }
        parent = newNode;
    }

    parent->flags |= kEndsWord;
}

#if 0
void
MutableTrieDictionary::addWords( UEnumeration *words,
                                  UErrorCode &status ) {
    int32_t length;
    const UChar *word;
    while ((word = uenum_unext(words, &length, &status)) && U_SUCCESS(status)) {
        addWord(word, length, status);
    }
}
#endif

int32_t
MutableTrieDictionary::matches( UText *text,
                                int32_t maxLength,
                                int32_t *lengths,
                                int &count,
                                int limit ) const {
    TernaryNode *parent;
    UBool pMatched;
    return search(text, maxLength, lengths, count, limit, parent, pMatched);
}

// Implementation of iteration for MutableTrieDictionary
class MutableTrieEnumeration : public StringEnumeration {
private:
    UStack      fNodeStack;     // Stack of nodes to process
    UVector32   fBranchStack;   // Stack of which branch we are working on
    TernaryNode *fRoot;         // Root node
    enum StackBranch {
        kLessThan,
        kEqual,
        kGreaterThan,
        kDone
    };

public:
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
public:
    MutableTrieEnumeration(TernaryNode *root, UErrorCode &status) 
        : fNodeStack(status), fBranchStack(status) {
        fRoot = root;
        fNodeStack.push(root, status);
        fBranchStack.push(kLessThan, status);
        unistr.remove();
    }
    
    virtual ~MutableTrieEnumeration() {
    }
    
    virtual StringEnumeration *clone() const {
        UErrorCode status = U_ZERO_ERROR;
        return new MutableTrieEnumeration(fRoot, status);
    }
    
    virtual const UnicodeString *snext(UErrorCode &status) {
        if (fNodeStack.empty() || U_FAILURE(status)) {
            return NULL;
        }
        TernaryNode *node = (TernaryNode *) fNodeStack.peek();
        StackBranch where = (StackBranch) fBranchStack.peeki();
        while (!fNodeStack.empty() && U_SUCCESS(status)) {
            UBool emit;
            UBool equal;

            switch (where) {
            case kLessThan:
                if (node->low != NULL) {
                    fBranchStack.setElementAt(kEqual, fBranchStack.size()-1);
                    node = (TernaryNode *) fNodeStack.push(node->low, status);
                    where = (StackBranch) fBranchStack.push(kLessThan, status);
                    break;
                }
            case kEqual:
                emit = (node->flags & kEndsWord) != 0;
                equal = (node->equal != NULL);
                // If this node should be part of the next emitted string, append
                // the UChar to the string, and make sure we pop it when we come
                // back to this node. The character should only be in the string
                // for as long as we're traversing the equal subtree of this node
                if (equal || emit) {
                    unistr.append(node->ch);
                    fBranchStack.setElementAt(kGreaterThan, fBranchStack.size()-1);
                }
                if (equal) {
                    node = (TernaryNode *) fNodeStack.push(node->equal, status);
                    where = (StackBranch) fBranchStack.push(kLessThan, status);
                }
                if (emit) {
                    return &unistr;
                }
                if (equal) {
                    break;
                }
            case kGreaterThan:
                // If this node's character is in the string, remove it.
                if (node->equal != NULL || (node->flags & kEndsWord)) {
                    unistr.truncate(unistr.length()-1);
                }
                if (node->high != NULL) {
                    fBranchStack.setElementAt(kDone, fBranchStack.size()-1);
                    node = (TernaryNode *) fNodeStack.push(node->high, status);
                    where = (StackBranch) fBranchStack.push(kLessThan, status);
                    break;
                }
            case kDone:
                fNodeStack.pop();
                fBranchStack.popi();
                node = (TernaryNode *) fNodeStack.peek();
                where = (StackBranch) fBranchStack.peeki();
                break;
            default:
                return NULL;
            }
        }
        return NULL;
    }
    
    // Very expensive, but this should never be used.
    virtual int32_t count(UErrorCode &status) const {
        MutableTrieEnumeration counter(fRoot, status);
        int32_t result = 0;
        while (counter.snext(status) != NULL && U_SUCCESS(status)) {
            ++result;
        }
        return result;
    }
    
    virtual void reset(UErrorCode &status) {
        fNodeStack.removeAllElements();
        fBranchStack.removeAllElements();
        fNodeStack.push(fRoot, status);
        fBranchStack.push(kLessThan, status);
        unistr.remove();
    }
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(MutableTrieEnumeration)

StringEnumeration *
MutableTrieDictionary::openWords( UErrorCode &status ) const {
    if (U_FAILURE(status)) {
        return NULL;
    }
    return new MutableTrieEnumeration(fTrie, status);
}

/*******************************************************************
 * CompactTrieDictionary
 */

struct CompactTrieHeader {
    uint32_t        size;           // Size of the data in bytes
    uint32_t        magic;          // Magic number (including version)
    uint16_t        nodeCount;      // Number of entries in offsets[]
    uint16_t        root;           // Node number of the root node
    uint32_t        offsets[1];      // Offsets to nodes from start of data
};

// Note that to avoid platform-specific alignment issues, all members of the node
// structures should be the same size, or should contain explicit padding to
// natural alignment boundaries.

// We can't use a bitfield for the flags+count field, because the layout of those
// is not portable. 12 bits of count allows for up to 4096 entries in a node.
struct CompactTrieNode {
    uint16_t        flagscount;     // Count of sub-entries, plus flags
};

enum CompactTrieNodeFlags {
    kVerticalNode   = 0x1000,       // This is a vertical node
    kParentEndsWord = 0x2000,       // The node whose equal link points to this ends a word
    kReservedFlag1  = 0x4000,
    kReservedFlag2  = 0x8000,
    kCountMask      = 0x0FFF,       // The count portion of flagscount
    kFlagMask       = 0xF000        // The flags portion of flagscount
};

// The two node types are distinguished by the kVerticalNode flag.

struct CompactTrieHorizontalEntry {
    uint16_t        ch;             // UChar
    uint16_t        equal;          // Equal link node index
};

// We don't use inheritance here because C++ does not guarantee that the
// base class comes first in memory!!

struct CompactTrieHorizontalNode {
    uint16_t        flagscount;     // Count of sub-entries, plus flags
    CompactTrieHorizontalEntry      entries[1];
};

struct CompactTrieVerticalNode {
    uint16_t        flagscount;     // Count of sub-entries, plus flags
    uint16_t        equal;          // Equal link node index
    uint16_t        chars[1];       // Code units
};

// {'Dic', 1}, version 1
#define COMPACT_TRIE_MAGIC_1 0x44696301

CompactTrieDictionary::CompactTrieDictionary(UDataMemory *dataObj,
                                                UErrorCode &status )
: fUData(dataObj)
{
    fData = (const CompactTrieHeader *) udata_getMemory(dataObj);
    fOwnData = FALSE;
    if (fData->magic != COMPACT_TRIE_MAGIC_1) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        fData = NULL;
    }
}
CompactTrieDictionary::CompactTrieDictionary( const void *data,
                                                UErrorCode &status )
: fUData(NULL)
{
    fData = (const CompactTrieHeader *) data;
    fOwnData = FALSE;
    if (fData->magic != COMPACT_TRIE_MAGIC_1) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        fData = NULL;
    }
}

CompactTrieDictionary::CompactTrieDictionary( const MutableTrieDictionary &dict,
                                                UErrorCode &status )
: fUData(NULL)
{
    fData = compactMutableTrieDictionary(dict, status);
    fOwnData = !U_FAILURE(status);
}

CompactTrieDictionary::~CompactTrieDictionary() {
    if (fOwnData) {
        uprv_free((void *)fData);
    }
    if (fUData) {
        udata_close(fUData);
    }
}

uint32_t
CompactTrieDictionary::dataSize() const {
    return fData->size;
}

const void *
CompactTrieDictionary::data() const {
    return fData;
}

// This function finds the address of a node for us, given its node ID
static inline const CompactTrieNode *
getCompactNode(const CompactTrieHeader *header, uint16_t node) {
    return (const CompactTrieNode *)((const uint8_t *)header + header->offsets[node]);
}

int32_t
CompactTrieDictionary::matches( UText *text,
                                int32_t maxLength,
                                int32_t *lengths,
                                int &count,
                                int limit ) const {
    // TODO: current implementation works in UTF-16 space
    const CompactTrieNode *node = getCompactNode(fData, fData->root);
    int mycount = 0;

    UChar uc = utext_current32(text);
    int i = 0;

    while (node != NULL) {
        // Check if the node we just exited ends a word
        if (limit > 0 && (node->flagscount & kParentEndsWord)) {
            lengths[mycount++] = i;
            --limit;
        }
        // Check that we haven't exceeded the maximum number of input characters.
        // We have to do that here rather than in the while condition so that
        // we can check for ending a word, above.
        if (i >= maxLength) {
            break;
        }

        int nodeCount = (node->flagscount & kCountMask);
        if (nodeCount == 0) {
            // Special terminal node; return now
            break;
        }
        if (node->flagscount & kVerticalNode) {
            // Vertical node; check all the characters in it
            const CompactTrieVerticalNode *vnode = (const CompactTrieVerticalNode *)node;
            for (int j = 0; j < nodeCount && i < maxLength; ++j) {
                if (uc != vnode->chars[j]) {
                    // We hit a non-equal character; return
                    goto exit;
                }
                utext_next32(text);
                uc = utext_current32(text);
                ++i;
            }
            // To get here we must have come through the whole list successfully;
            // go on to the next node. Note that a word cannot end in the middle
            // of a vertical node.
            node = getCompactNode(fData, vnode->equal);
        }
        else {
            // Horizontal node; do binary search
            const CompactTrieHorizontalNode *hnode = (const CompactTrieHorizontalNode *)node;
            int low = 0;
            int high = nodeCount-1;
            int middle;
            node = NULL;    // If we don't find a match, we'll fall out of the loop
            while (high >= low) {
                middle = (high+low)/2;
                if (uc == hnode->entries[middle].ch) {
                    // We hit a match; get the next node and next character
                    node = getCompactNode(fData, hnode->entries[middle].equal);
                    utext_next32(text);
                    uc = utext_current32(text);
                    ++i;
                    break;
                }
                else if (uc < hnode->entries[middle].ch) {
                    high = middle-1;
                }
                else {
                    low = middle+1;
                }
            }
        }
    }
exit:
    count = mycount;
    return i;
}

// Implementation of iteration for CompactTrieDictionary
class CompactTrieEnumeration : public StringEnumeration {
private:
    UVector32               fNodeStack;     // Stack of nodes to process
    UVector32               fIndexStack;    // Stack of where in node we are
    const CompactTrieHeader *fHeader;       // Trie data

public:
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
public:
    CompactTrieEnumeration(const CompactTrieHeader *header, UErrorCode &status) 
        : fNodeStack(status), fIndexStack(status) {
        fHeader = header;
        fNodeStack.push(header->root, status);
        fIndexStack.push(0, status);
        unistr.remove();
    }
    
    virtual ~CompactTrieEnumeration() {
    }
    
    virtual StringEnumeration *clone() const {
        UErrorCode status = U_ZERO_ERROR;
        return new CompactTrieEnumeration(fHeader, status);
    }
    
    virtual const UnicodeString * snext(UErrorCode &status);

    // Very expensive, but this should never be used.
    virtual int32_t count(UErrorCode &status) const {
        CompactTrieEnumeration counter(fHeader, status);
        int32_t result = 0;
        while (counter.snext(status) != NULL && U_SUCCESS(status)) {
            ++result;
        }
        return result;
    }
    
    virtual void reset(UErrorCode &status) {
        fNodeStack.removeAllElements();
        fIndexStack.removeAllElements();
        fNodeStack.push(fHeader->root, status);
        fIndexStack.push(0, status);
        unistr.remove();
    }
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CompactTrieEnumeration)

const UnicodeString *
CompactTrieEnumeration::snext(UErrorCode &status) {
    if (fNodeStack.empty() || U_FAILURE(status)) {
        return NULL;
    }
    const CompactTrieNode *node = getCompactNode(fHeader, fNodeStack.peeki());
    int where = fIndexStack.peeki();
    while (!fNodeStack.empty() && U_SUCCESS(status)) {
        int nodeCount = (node->flagscount & kCountMask);
        UBool goingDown = FALSE;
        if (nodeCount == 0) {
            // Terminal node; go up immediately
            fNodeStack.popi();
            fIndexStack.popi();
            node = getCompactNode(fHeader, fNodeStack.peeki());
            where = fIndexStack.peeki();
        }
        else if (node->flagscount & kVerticalNode) {
            // Vertical node
            const CompactTrieVerticalNode *vnode = (const CompactTrieVerticalNode *)node;
            if (where == 0) {
                // Going down
                unistr.append((const UChar *)vnode->chars, (int32_t) nodeCount);
                fIndexStack.setElementAt(1, fIndexStack.size()-1);
                node = getCompactNode(fHeader, fNodeStack.push(vnode->equal, status));
                where = fIndexStack.push(0, status);
                goingDown = TRUE;
            }
            else {
                // Going up
                unistr.truncate(unistr.length()-nodeCount);
                fNodeStack.popi();
                fIndexStack.popi();
                node = getCompactNode(fHeader, fNodeStack.peeki());
                where = fIndexStack.peeki();
            }
        }
        else {
            // Horizontal node
            const CompactTrieHorizontalNode *hnode = (const CompactTrieHorizontalNode *)node;
            if (where > 0) {
                // Pop previous char
                unistr.truncate(unistr.length()-1);
            }
            if (where < nodeCount) {
                // Push on next node
                unistr.append((UChar)hnode->entries[where].ch);
                fIndexStack.setElementAt(where+1, fIndexStack.size()-1);
                node = getCompactNode(fHeader, fNodeStack.push(hnode->entries[where].equal, status));
                where = fIndexStack.push(0, status);
                goingDown = TRUE;
            }
            else {
                // Going up
                fNodeStack.popi();
                fIndexStack.popi();
                node = getCompactNode(fHeader, fNodeStack.peeki());
                where = fIndexStack.peeki();
            }
        }
        // Check if the parent of the node we've just gone down to ends a
        // word. If so, return it.
        if (goingDown && (node->flagscount & kParentEndsWord)) {
            return &unistr;
        }
    }
    return NULL;
}

StringEnumeration *
CompactTrieDictionary::openWords( UErrorCode &status ) const {
    if (U_FAILURE(status)) {
        return NULL;
    }
    return new CompactTrieEnumeration(fData, status);
}

//
// Below here is all code related to converting a ternary trie to a compact trie
// and back again
//

// Helper classes to construct the compact trie
class BuildCompactTrieNode: public UMemory {
 public:
    UBool           fParentEndsWord;
    UBool           fVertical;
    UBool           fHasDuplicate;
    int32_t         fNodeID;
    UnicodeString   fChars;

 public:
    BuildCompactTrieNode(UBool parentEndsWord, UBool vertical, UStack &nodes, UErrorCode &status) {
        fParentEndsWord = parentEndsWord;
        fHasDuplicate = FALSE;
        fVertical = vertical;
        fNodeID = nodes.size();
        nodes.push(this, status);
    }
    
    virtual ~BuildCompactTrieNode() {
    }
    
    virtual uint32_t size() {
        return sizeof(uint16_t);
    }
    
    virtual void write(uint8_t *bytes, uint32_t &offset, const UVector32 &/*translate*/) {
        // Write flag/count
        *((uint16_t *)(bytes+offset)) = (fChars.length() & kCountMask)
            | (fVertical ? kVerticalNode : 0) | (fParentEndsWord ? kParentEndsWord : 0 );
        offset += sizeof(uint16_t);
    }
};

class BuildCompactTrieHorizontalNode: public BuildCompactTrieNode {
 public:
    UStack          fLinks;

 public:
    BuildCompactTrieHorizontalNode(UBool parentEndsWord, UStack &nodes, UErrorCode &status)
        : BuildCompactTrieNode(parentEndsWord, FALSE, nodes, status), fLinks(status) {
    }
    
    virtual ~BuildCompactTrieHorizontalNode() {
    }
    
    virtual uint32_t size() {
        return offsetof(CompactTrieHorizontalNode,entries) +
                (fChars.length()*sizeof(CompactTrieHorizontalEntry));
    }
    
    virtual void write(uint8_t *bytes, uint32_t &offset, const UVector32 &translate) {
        BuildCompactTrieNode::write(bytes, offset, translate);
        int32_t count = fChars.length();
        for (int32_t i = 0; i < count; ++i) {
            CompactTrieHorizontalEntry *entry = (CompactTrieHorizontalEntry *)(bytes+offset);
            entry->ch = fChars[i];
            entry->equal = translate.elementAti(((BuildCompactTrieNode *)fLinks[i])->fNodeID);
#ifdef DEBUG_TRIE_DICT
            if (entry->equal == 0) {
                fprintf(stderr, "ERROR: horizontal link %d, logical node %d maps to physical node zero\n",
                        i, ((BuildCompactTrieNode *)fLinks[i])->fNodeID);
            }
#endif
            offset += sizeof(CompactTrieHorizontalEntry);
        }
    }
    
    void addNode(UChar ch, BuildCompactTrieNode *link, UErrorCode &status) {
        fChars.append(ch);
        fLinks.push(link, status);
    }
};

class BuildCompactTrieVerticalNode: public BuildCompactTrieNode {
 public:
    BuildCompactTrieNode    *fEqual;

 public:
    BuildCompactTrieVerticalNode(UBool parentEndsWord, UStack &nodes, UErrorCode &status)
        : BuildCompactTrieNode(parentEndsWord, TRUE, nodes, status) {
        fEqual = NULL;
    }
    
    virtual ~BuildCompactTrieVerticalNode() {
    }
    
    virtual uint32_t size() {
        return offsetof(CompactTrieVerticalNode,chars) + (fChars.length()*sizeof(uint16_t));
    }
    
    virtual void write(uint8_t *bytes, uint32_t &offset, const UVector32 &translate) {
        CompactTrieVerticalNode *node = (CompactTrieVerticalNode *)(bytes+offset);
        BuildCompactTrieNode::write(bytes, offset, translate);
        node->equal = translate.elementAti(fEqual->fNodeID);
        offset += sizeof(node->equal);
#ifdef DEBUG_TRIE_DICT
        if (node->equal == 0) {
            fprintf(stderr, "ERROR: vertical link, logical node %d maps to physical node zero\n",
                    fEqual->fNodeID);
        }
#endif
        fChars.extract(0, fChars.length(), (UChar *)node->chars);
        offset += sizeof(uint16_t)*fChars.length();
    }
    
    void addChar(UChar ch) {
        fChars.append(ch);
    }
    
    void setLink(BuildCompactTrieNode *node) {
        fEqual = node;
    }
};

// Forward declaration
static void walkHorizontal(const TernaryNode *node,
                            BuildCompactTrieHorizontalNode *building,
                            UStack &nodes,
                            UErrorCode &status);

// Convert one node. Uses recursion.

static BuildCompactTrieNode *
compactOneNode(const TernaryNode *node, UBool parentEndsWord, UStack &nodes, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    BuildCompactTrieNode *result = NULL;
    UBool horizontal = (node->low != NULL || node->high != NULL);
    if (horizontal) {
        BuildCompactTrieHorizontalNode *hResult =
                new BuildCompactTrieHorizontalNode(parentEndsWord, nodes, status);
        if (hResult == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        if (U_SUCCESS(status)) {
            walkHorizontal(node, hResult, nodes, status);
            result = hResult;
        }
    }
    else {
        BuildCompactTrieVerticalNode *vResult =
                new BuildCompactTrieVerticalNode(parentEndsWord, nodes, status);
        if (vResult == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        else if (U_SUCCESS(status)) {
            UBool   endsWord = FALSE;
            // Take up nodes until we end a word, or hit a node with < or > links
            do {
                vResult->addChar(node->ch);
                endsWord = (node->flags & kEndsWord) != 0;
                node = node->equal;
            }
            while(node != NULL && !endsWord && node->low == NULL && node->high == NULL);
            if (node == NULL) {
                if (!endsWord) {
                    status = U_ILLEGAL_ARGUMENT_ERROR;  // Corrupt input trie
                }
                else {
                    vResult->setLink((BuildCompactTrieNode *)nodes[1]);
                }
            }
            else {
                vResult->setLink(compactOneNode(node, endsWord, nodes, status));
            }
            result = vResult;
        }
    }
    return result;
}

// Walk the set of peers at the same level, to build a horizontal node.
// Uses recursion.

static void walkHorizontal(const TernaryNode *node,
                            BuildCompactTrieHorizontalNode *building,
                            UStack &nodes,
                            UErrorCode &status) {
    while (U_SUCCESS(status) && node != NULL) {
        if (node->low != NULL) {
            walkHorizontal(node->low, building, nodes, status);
        }
        BuildCompactTrieNode *link = NULL;
        if (node->equal != NULL) {
            link = compactOneNode(node->equal, (node->flags & kEndsWord) != 0, nodes, status);
        }
        else if (node->flags & kEndsWord) {
            link = (BuildCompactTrieNode *)nodes[1];
        }
        if (U_SUCCESS(status) && link != NULL) {
            building->addNode(node->ch, link, status);
        }
        // Tail recurse manually instead of leaving it to the compiler.
        //if (node->high != NULL) {
        //    walkHorizontal(node->high, building, nodes, status);
        //}
        node = node->high;
    }
}

U_NAMESPACE_END
U_NAMESPACE_USE
U_CDECL_BEGIN
static int32_t U_CALLCONV
_sortBuildNodes(const void * /*context*/, const void *voidl, const void *voidr) {
    BuildCompactTrieNode *left = *(BuildCompactTrieNode **)voidl;
    BuildCompactTrieNode *right = *(BuildCompactTrieNode **)voidr;
    // Check for comparing a node to itself, to avoid spurious duplicates
    if (left == right) {
        return 0;
    }
    // Most significant is type of node. Can never coalesce.
    if (left->fVertical != right->fVertical) {
        return left->fVertical - right->fVertical;
    }
    // Next, the "parent ends word" flag. If that differs, we cannot coalesce.
    if (left->fParentEndsWord != right->fParentEndsWord) {
        return left->fParentEndsWord - right->fParentEndsWord;
    }
    // Next, the string. If that differs, we can never coalesce.
    int32_t result = left->fChars.compare(right->fChars);
    if (result != 0) {
        return result;
    }
    // We know they're both the same node type, so branch for the two cases.
    if (left->fVertical) {
        result = ((BuildCompactTrieVerticalNode *)left)->fEqual->fNodeID
                            - ((BuildCompactTrieVerticalNode *)right)->fEqual->fNodeID;
    }
    else {
        // We need to compare the links vectors. They should be the
        // same size because the strings were equal.
        // We compare the node IDs instead of the pointers, to handle
        // coalesced nodes.
        BuildCompactTrieHorizontalNode *hleft, *hright;
        hleft = (BuildCompactTrieHorizontalNode *)left;
        hright = (BuildCompactTrieHorizontalNode *)right;
        int32_t count = hleft->fLinks.size();
        for (int32_t i = 0; i < count && result == 0; ++i) {
            result = ((BuildCompactTrieNode *)(hleft->fLinks[i]))->fNodeID -
                     ((BuildCompactTrieNode *)(hright->fLinks[i]))->fNodeID;
        }
    }
    // If they are equal to each other, mark them (speeds coalescing)
    if (result == 0) {
        left->fHasDuplicate = TRUE;
        right->fHasDuplicate = TRUE;
    }
    return result;
}
U_CDECL_END
U_NAMESPACE_BEGIN

static void coalesceDuplicates(UStack &nodes, UErrorCode &status) {
    // We sort the array of nodes to place duplicates next to each other
    if (U_FAILURE(status)) {
        return;
    }
    int32_t size = nodes.size();
    void **array = (void **)uprv_malloc(sizeof(void *)*size);
    if (array == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    (void) nodes.toArray(array);
    
    // Now repeatedly identify duplicates until there are no more
    int32_t dupes = 0;
    long    passCount = 0;
#ifdef DEBUG_TRIE_DICT
    long    totalDupes = 0;
#endif
    do {
        BuildCompactTrieNode *node;
        BuildCompactTrieNode *first = NULL;
        BuildCompactTrieNode **p;
        BuildCompactTrieNode **pFirst = NULL;
        int32_t counter = size - 2;
        // Sort the array, skipping nodes 0 and 1. Use quicksort for the first
        // pass for speed. For the second and subsequent passes, we use stable
        // (insertion) sort for two reasons:
        // 1. The array is already mostly ordered, so we get better performance.
        // 2. The way we find one and only one instance of a set of duplicates is to
        //    check that the node ID equals the array index. If we used an unstable
        //    sort for the second or later passes, it's possible that none of the
        //    duplicates would wind up with a node ID equal to its array index.
        //    The sort stability guarantees that, because as we coalesce more and
        //    more groups, the first element of the resultant group will be one of
        //    the first elements of the groups being coalesced.
        // To use quicksort for the second and subsequent passes, we would have to
        // find the minimum of the node numbers in a group, and set all the nodes
        // in the group to that node number.
        uprv_sortArray(array+2, counter, sizeof(void *), _sortBuildNodes, NULL, (passCount > 0), &status);
        dupes = 0;
        for (p = (BuildCompactTrieNode **)array + 2; counter > 0; --counter, ++p) {
            node = *p;
            if (node->fHasDuplicate) {
                if (first == NULL) {
                    first = node;
                    pFirst = p;
                }
                else if (_sortBuildNodes(NULL, pFirst, p) != 0) {
                    // Starting a new run of dupes
                    first = node;
                    pFirst = p;
                }
                else if (node->fNodeID != first->fNodeID) {
                    // Slave one to the other, note duplicate
                    node->fNodeID = first->fNodeID;
                    dupes += 1;
                }
            }
            else {
                // This node has no dupes
                first = NULL;
                pFirst = NULL;
            }
        }
        passCount += 1;
#ifdef DEBUG_TRIE_DICT
        totalDupes += dupes;
        fprintf(stderr, "Trie node dupe removal, pass %d: %d nodes tagged\n", passCount, dupes);
#endif
    }
    while (dupes > 0);
#ifdef DEBUG_TRIE_DICT
    fprintf(stderr, "Trie node dupe removal complete: %d tagged in %d passes\n", totalDupes, passCount);
#endif

    // We no longer need the temporary array, as the nodes have all been marked appropriately.
    uprv_free(array);
}

U_NAMESPACE_END
U_CDECL_BEGIN
static void U_CALLCONV _deleteBuildNode(void *obj) {
    delete (BuildCompactTrieNode *) obj;
}
U_CDECL_END
U_NAMESPACE_BEGIN

CompactTrieHeader *
CompactTrieDictionary::compactMutableTrieDictionary( const MutableTrieDictionary &dict,
                                UErrorCode &status ) {
    if (U_FAILURE(status)) {
        return NULL;
    }
#ifdef DEBUG_TRIE_DICT
    struct tms timing;
    struct tms previous;
    (void) ::times(&previous);
#endif
    UStack nodes(_deleteBuildNode, NULL, status);      // Index of nodes

    // Add node 0, used as the NULL pointer/sentinel.
    nodes.addElement((int32_t)0, status);

    // Start by creating the special empty node we use to indicate that the parent
    // terminates a word. This must be node 1, because the builder assumes
    // that.
    if (U_FAILURE(status)) {
        return NULL;
    }
    BuildCompactTrieNode *terminal = new BuildCompactTrieNode(TRUE, FALSE, nodes, status);
    if (terminal == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }

    // This call does all the work of building the new trie structure. The root
    // will be node 2.
    BuildCompactTrieNode *root = compactOneNode(dict.fTrie, FALSE, nodes, status);
#ifdef DEBUG_TRIE_DICT
    (void) ::times(&timing);
    fprintf(stderr, "Compact trie built, %d nodes, time user %f system %f\n",
        nodes.size(), (double)(timing.tms_utime-previous.tms_utime)/CLK_TCK,
        (double)(timing.tms_stime-previous.tms_stime)/CLK_TCK);
    previous = timing;
#endif

    // Now coalesce all duplicate nodes.
    coalesceDuplicates(nodes, status);
#ifdef DEBUG_TRIE_DICT
    (void) ::times(&timing);
    fprintf(stderr, "Duplicates coalesced, time user %f system %f\n",
        (double)(timing.tms_utime-previous.tms_utime)/CLK_TCK,
        (double)(timing.tms_stime-previous.tms_stime)/CLK_TCK);
    previous = timing;
#endif

    // Next, build the output trie.
    // First we compute all the sizes and build the node ID translation table.
    uint32_t totalSize = offsetof(CompactTrieHeader,offsets);
    int32_t count = nodes.size();
    int32_t nodeCount = 1;              // The sentinel node we already have
    BuildCompactTrieNode *node;
    int32_t i;
    UVector32 translate(count, status); // Should be no growth needed after this
    translate.push(0, status);          // The sentinel node
    
    if (U_FAILURE(status)) {
        return NULL;
    }

    for (i = 1; i < count; ++i) {
        node = (BuildCompactTrieNode *)nodes[i];
        if (node->fNodeID == i) {
            // Only one node out of each duplicate set is used
            if (i >= translate.size()) {
                // Logically extend the mapping table
                translate.setSize(i+1);
            }
            translate.setElementAt(nodeCount++, i);
            totalSize += node->size();
        }
    }
    
    // Check for overflowing 16 bits worth of nodes.
    if (nodeCount > 0x10000) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    
    // Add enough room for the offsets.
    totalSize += nodeCount*sizeof(uint32_t);
#ifdef DEBUG_TRIE_DICT
    (void) ::times(&timing);
    fprintf(stderr, "Sizes/mapping done, time user %f system %f\n",
        (double)(timing.tms_utime-previous.tms_utime)/CLK_TCK,
        (double)(timing.tms_stime-previous.tms_stime)/CLK_TCK);
    previous = timing;
    fprintf(stderr, "%d nodes, %d unique, %d bytes\n", nodes.size(), nodeCount, totalSize);
#endif
    uint8_t *bytes = (uint8_t *)uprv_malloc(totalSize);
    if (bytes == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    CompactTrieHeader *header = (CompactTrieHeader *)bytes;
    header->size = totalSize;
    header->nodeCount = nodeCount;
    header->offsets[0] = 0;                     // Sentinel
    header->root = translate.elementAti(root->fNodeID);
#ifdef DEBUG_TRIE_DICT
    if (header->root == 0) {
        fprintf(stderr, "ERROR: root node %d translate to physical zero\n", root->fNodeID);
    }
#endif
    uint32_t offset = offsetof(CompactTrieHeader,offsets)+(nodeCount*sizeof(uint32_t));
    nodeCount = 1;
    // Now write the data
    for (i = 1; i < count; ++i) {
        node = (BuildCompactTrieNode *)nodes[i];
        if (node->fNodeID == i) {
            header->offsets[nodeCount++] = offset;
            node->write(bytes, offset, translate);
        }
    }
#ifdef DEBUG_TRIE_DICT
    (void) ::times(&timing);
    fprintf(stderr, "Trie built, time user %f system %f\n",
        (double)(timing.tms_utime-previous.tms_utime)/CLK_TCK,
        (double)(timing.tms_stime-previous.tms_stime)/CLK_TCK);
    previous = timing;
    fprintf(stderr, "Final offset is %d\n", offset);
    
    // Collect statistics on node types and sizes
    int hCount = 0;
    int vCount = 0;
    size_t hSize = 0;
    size_t vSize = 0;
    size_t hItemCount = 0;
    size_t vItemCount = 0;
    uint32_t previousOff = offset;
    for (uint16_t nodeIdx = nodeCount-1; nodeIdx >= 2; --nodeIdx) {
        const CompactTrieNode *node = getCompactNode(header, nodeIdx);
        if (node->flagscount & kVerticalNode) {
            vCount += 1;
            vItemCount += (node->flagscount & kCountMask);
            vSize += previousOff-header->offsets[nodeIdx];
        }
        else {
            hCount += 1;
            hItemCount += (node->flagscount & kCountMask);
            hSize += previousOff-header->offsets[nodeIdx];
        }
        previousOff = header->offsets[nodeIdx];
    }
    fprintf(stderr, "Horizontal nodes: %d total, average %f bytes with %f items\n", hCount,
                (double)hSize/hCount, (double)hItemCount/hCount);
    fprintf(stderr, "Vertical nodes: %d total, average %f bytes with %f items\n", vCount,
                (double)vSize/vCount, (double)vItemCount/vCount);
#endif

    if (U_FAILURE(status)) {
        uprv_free(bytes);
        header = NULL;
    }
    else {
        header->magic = COMPACT_TRIE_MAGIC_1;
    }
    return header;
}

// Forward declaration
static TernaryNode *
unpackOneNode( const CompactTrieHeader *header, const CompactTrieNode *node, UErrorCode &status );


// Convert a horizontal node (or subarray thereof) into a ternary subtrie
static TernaryNode *
unpackHorizontalArray( const CompactTrieHeader *header, const CompactTrieHorizontalEntry *array,
                            int low, int high, UErrorCode &status ) {
    if (U_FAILURE(status) || low > high) {
        return NULL;
    }
    int middle = (low+high)/2;
    TernaryNode *result = new TernaryNode(array[middle].ch);
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    const CompactTrieNode *equal = getCompactNode(header, array[middle].equal);
    if (equal->flagscount & kParentEndsWord) {
        result->flags |= kEndsWord;
    }
    result->low = unpackHorizontalArray(header, array, low, middle-1, status);
    result->high = unpackHorizontalArray(header, array, middle+1, high, status);
    result->equal = unpackOneNode(header, equal, status);
    return result;
}                            

// Convert one compact trie node into a ternary subtrie
static TernaryNode *
unpackOneNode( const CompactTrieHeader *header, const CompactTrieNode *node, UErrorCode &status ) {
    int nodeCount = (node->flagscount & kCountMask);
    if (nodeCount == 0 || U_FAILURE(status)) {
        // Failure, or terminal node
        return NULL;
    }
    if (node->flagscount & kVerticalNode) {
        const CompactTrieVerticalNode *vnode = (const CompactTrieVerticalNode *)node;
        TernaryNode *head = NULL;
        TernaryNode *previous = NULL;
        TernaryNode *latest = NULL;
        for (int i = 0; i < nodeCount; ++i) {
            latest = new TernaryNode(vnode->chars[i]);
            if (latest == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            if (head == NULL) {
                head = latest;
            }
            if (previous != NULL) {
                previous->equal = latest;
            }
            previous = latest;
        }
        if (latest != NULL) {
            const CompactTrieNode *equal = getCompactNode(header, vnode->equal);
            if (equal->flagscount & kParentEndsWord) {
                latest->flags |= kEndsWord;
            }
            latest->equal = unpackOneNode(header, equal, status);
        }
        return head;
    }
    else {
        // Horizontal node
        const CompactTrieHorizontalNode *hnode = (const CompactTrieHorizontalNode *)node;
        return unpackHorizontalArray(header, &hnode->entries[0], 0, nodeCount-1, status);
    }
}

MutableTrieDictionary *
CompactTrieDictionary::cloneMutable( UErrorCode &status ) const {
    MutableTrieDictionary *result = new MutableTrieDictionary( status );
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    TernaryNode *root = unpackOneNode(fData, getCompactNode(fData, fData->root), status);
    if (U_FAILURE(status)) {
        delete root;    // Clean up
        delete result;
        return NULL;
    }
    result->fTrie = root;
    return result;
}

U_NAMESPACE_END

U_CAPI int32_t U_EXPORT2
triedict_swap(const UDataSwapper *ds, const void *inData, int32_t length, void *outData,
           UErrorCode *status) {

    if (status == NULL || U_FAILURE(*status)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || length<-1 || (length>0 && outData==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    //
    //  Check that the data header is for for dictionary data.
    //    (Header contents are defined in genxxx.cpp)
    //
    const UDataInfo *pInfo = (const UDataInfo *)((const uint8_t *)inData+4);
    if(!(  pInfo->dataFormat[0]==0x54 &&   /* dataFormat="TrDc" */
           pInfo->dataFormat[1]==0x72 &&
           pInfo->dataFormat[2]==0x44 &&
           pInfo->dataFormat[3]==0x63 &&
           pInfo->formatVersion[0]==1  )) {
        udata_printError(ds, "triedict_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *status=U_UNSUPPORTED_ERROR;
        return 0;
    }

    //
    // Swap the data header.  (This is the generic ICU Data Header, not the 
    //                         CompactTrieHeader).  This swap also conveniently gets us
    //                         the size of the ICU d.h., which lets us locate the start
    //                         of the RBBI specific data.
    //
    int32_t headerSize=udata_swapDataHeader(ds, inData, length, outData, status);

    //
    // Get the CompactTrieHeader, and check that it appears to be OK.
    //
    const uint8_t  *inBytes =(const uint8_t *)inData+headerSize;
    const CompactTrieHeader *header = (const CompactTrieHeader *)inBytes;
    if (ds->readUInt32(header->magic) != COMPACT_TRIE_MAGIC_1
            || ds->readUInt32(header->size) < sizeof(CompactTrieHeader))
    {
        udata_printError(ds, "triedict_swap(): CompactTrieHeader is invalid.\n");
        *status=U_UNSUPPORTED_ERROR;
        return 0;
    }

    //
    // Prefight operation?  Just return the size
    //
    uint32_t totalSize = ds->readUInt32(header->size);
    int32_t sizeWithUData = (int32_t)totalSize + headerSize;
    if (length < 0) {
        return sizeWithUData;
    }

    //
    // Check that length passed in is consistent with length from RBBI data header.
    //
    if (length < sizeWithUData) {
        udata_printError(ds, "triedict_swap(): too few bytes (%d after ICU Data header) for trie data.\n",
                            totalSize);
        *status=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
        }

    //
    // Swap the Data.  Do the data itself first, then the CompactTrieHeader, because
    //                 we need to reference the header to locate the data, and an
    //                 inplace swap of the header leaves it unusable.
    //
    uint8_t             *outBytes = (uint8_t *)outData + headerSize;
    CompactTrieHeader   *outputHeader = (CompactTrieHeader *)outBytes;

#if 0
    //
    // If not swapping in place, zero out the output buffer before starting.
    //
    if (inBytes != outBytes) {
        uprv_memset(outBytes, 0, totalSize);
    }

    // We need to loop through all the nodes in the offset table, and swap each one.
    uint16_t nodeCount = ds->readUInt16(header->nodeCount);
    // Skip node 0, which should always be 0.
    for (int i = 1; i < nodeCount; ++i) {
        uint32_t nodeOff = ds->readUInt32(header->offsets[i]);
        const CompactTrieNode *inNode = (const CompactTrieNode *)(inBytes + nodeOff);
        CompactTrieNode *outNode = (CompactTrieNode *)(outBytes + nodeOff);
        uint16_t flagscount = ds->readUInt16(inNode->flagscount);
        uint16_t itemCount = flagscount & kCountMask;
        ds->writeUInt16(&outNode->flagscount, flagscount);
        if (itemCount > 0) {
            if (flagscount & kVerticalNode) {
                ds->swapArray16(ds, inBytes+nodeOff+offsetof(CompactTrieVerticalNode,chars),
                                    itemCount*sizeof(uint16_t),
                                    outBytes+nodeOff+offsetof(CompactTrieVerticalNode,chars), status);
                uint16_t equal = ds->readUInt16(inBytes+nodeOff+offsetof(CompactTrieVerticalNode,equal);
                ds->writeUInt16(outBytes+nodeOff+offsetof(CompactTrieVerticalNode,equal));
            }
            else {
                const CompactTrieHorizontalNode *inHNode = (const CompactTrieHorizontalNode *)inNode;
                CompactTrieHorizontalNode *outHNode = (CompactTrieHorizontalNode *)outNode;
                for (int j = 0; j < itemCount; ++j) {
                    uint16_t word = ds->readUInt16(inHNode->entries[j].ch);
                    ds->writeUInt16(&outHNode->entries[j].ch, word);
                    word = ds->readUInt16(inHNode->entries[j].equal);
                    ds->writeUInt16(&outHNode->entries[j].equal, word);
                }
            }
        }
    }
#endif

    // All the data in all the nodes consist of 16 bit items. Swap them all at once.
    uint16_t nodeCount = ds->readUInt16(header->nodeCount);
    uint32_t nodesOff = offsetof(CompactTrieHeader,offsets)+((uint32_t)nodeCount*sizeof(uint32_t));
    ds->swapArray16(ds, inBytes+nodesOff, totalSize-nodesOff, outBytes+nodesOff, status);

    // Swap the header
    ds->writeUInt32(&outputHeader->size, totalSize);
    uint32_t magic = ds->readUInt32(header->magic);
    ds->writeUInt32(&outputHeader->magic, magic);
    ds->writeUInt16(&outputHeader->nodeCount, nodeCount);
    uint16_t root = ds->readUInt16(header->root);
    ds->writeUInt16(&outputHeader->root, root);
    ds->swapArray32(ds, inBytes+offsetof(CompactTrieHeader,offsets),
            sizeof(uint32_t)*(int32_t)nodeCount,
            outBytes+offsetof(CompactTrieHeader,offsets), status);

    return sizeWithUData;
}

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
