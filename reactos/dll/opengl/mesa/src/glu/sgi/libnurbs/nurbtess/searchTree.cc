/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/
/*
*/

#include <stdlib.h>
#include <stdio.h>
#include "zlassert.h"

#include "searchTree.h"

#define max(a,b) ((a>b)? a:b)

treeNode* TreeNodeMake(void *key)
{
  treeNode *ret = (treeNode*) malloc(sizeof(treeNode));
  assert(ret);
  ret->key = key;
  ret->parent = NULL;
  ret->left = NULL;
  ret->right = NULL;
  return ret;
}

void TreeNodeDeleteSingleNode(treeNode* node)
{
  free(node);
}

void TreeNodeDeleteWholeTree(treeNode* node)
{
  if(node == NULL) return;
  TreeNodeDeleteWholeTree(node->left);
  TreeNodeDeleteWholeTree(node->right);
  TreeNodeDeleteSingleNode(node);
}

void TreeNodePrint(treeNode* node, 
		   void (*keyPrint) (void*))
{
  if(node ==NULL) return;
  TreeNodePrint(node->left, keyPrint);
  keyPrint(node->key);
  TreeNodePrint(node->right, keyPrint);  
}

int TreeNodeDepth(treeNode* root)
{
  if(root == NULL) return 0;
  else{
    int leftdepth = TreeNodeDepth(root->left);
    int rightdepth = TreeNodeDepth(root->right);  
    return 1 + max(leftdepth, rightdepth);
  }
}

/*return the node with the key.
 *NULL is returned if not found
 */
treeNode* TreeNodeFind(treeNode* tree, void* key,
		       int (*compkey) (void*, void*)) 	       
{
  if(tree == NULL) 
    return NULL;
  if(key == tree->key)
    return tree;
  else if(compkey(key, tree->key) < 0)
    return TreeNodeFind(tree->left, key, compkey);
  else 
    return TreeNodeFind(tree->right, key, compkey);    
}


treeNode* TreeNodeInsert(treeNode* root, treeNode* newnode,
		    int (*compkey) (void *, void *))
{
  treeNode *y = NULL;
  treeNode *x = root;
  /*going down the tree from the root.
   *x traces the path, y is the parent of x.
   */
  while (x != NULL){
    y = x;
    if(compkey(newnode->key,x->key) < 0) /*if newnode < x*/
      x = x->left;
    else 
      x = x->right;
  }

  /*now y has the property that 
   * if newnode < y, then y->left is NULL
   * if newnode > y, then y->right is NULL.
   *So we want to isnert newnode to be the child of y
   */
  newnode->parent = y;
  if(y == NULL) 
    return newnode;
  else if( compkey(newnode->key, y->key) <0)
    {
      y->left = newnode;
    }
  else
    {
      y->right = newnode;
    }

  return root;
}
    
treeNode* TreeNodeDeleteSingleNode(treeNode* tree, treeNode* node)
{
  treeNode* y;
  treeNode* x;
  treeNode* ret;
  if(node==NULL) return tree;

  if(node->left == NULL || node->right == NULL) {

    y = node;
    if(y->left != NULL) 
      x = y->left;
    else
      x = y->right;

    if( x != NULL)
      x->parent = y->parent;

    if(y->parent == NULL) /*y is the root which has at most one child x*/
      ret = x;
    else /*y is not the root*/
      {
	if(y == y->parent->left) 
	  y->parent->left = x;
	else
	  y->parent->right = x;
	ret =  tree;
      }
  }
  else { /*node has two children*/

     y = TreeNodeSuccessor(node);
     assert(y->left == NULL);

     if(y == node->right) /*y is the right child if node*/
       {
	 y->parent = node->parent;
	 y->left = node->left;
	 node->left->parent = y;
	 
       }
     else  /*y != node->right*/
       {
	 x = y->right;
	 if(x!= NULL)
	   x->parent = y->parent;

	 assert(y->parent != NULL);
	 if(y == y->parent->left) 
	   y->parent->left = x;
	 else
	   y->parent->right = x;
	 /*move y to the position of node*/
	 y->parent = node->parent;
	 y->left = node->left;
	 y->right = node->right;
	 node->left->parent = y;
	 node->right->parent = y;
       }
    if(node->parent != NULL) {
      if(node->parent->left == node)
	node->parent->left = y;
      else
	node->parent->right = y;
      ret = tree; /*the root if the tree doesn't change*/
    }
    else /*node->parent is NULL: node is the root*/
      ret = y;    
  }

  /*finally free the node, and return the new root*/
  TreeNodeDeleteSingleNode(node);
  return ret;
}
     

/*the minimum node in the tree rooted by node
 */
treeNode* TreeNodeMinimum(treeNode* node)
{
  treeNode* temp = node;
  if(temp == NULL) return NULL;
  while(temp->left != NULL) {
    temp = temp->left;
  }
  return temp;
}

/*the maximum node in the tree rooted by node
 */
treeNode* TreeNodeMaximum(treeNode* node)
{
  treeNode* temp = node;
  if(temp == NULL) return NULL;
  while(temp->right != NULL) {
    temp = temp->right;
  }
  return temp;
}

/*return the first node (in sorted order) which is to the right of this node
 */
treeNode* TreeNodeSuccessor(treeNode* node)
{
  if(node == NULL) return NULL;
  if(node->right != NULL)
    return TreeNodeMinimum(node->right);
  else{ /*node->right is NULL*/

    /*find the first right-ancestor*/
    treeNode *y = node->parent;
    treeNode* x = node;
    while(y != NULL && x == y->right) /*if y is a left parent of x*/
      {

	x = y;
	y = y->parent;
      }
    return y;
  }
}

/*return the first node (in sorted order) which is to the left of this node
 */
treeNode* TreeNodePredecessor(treeNode* node)
{
  if(node == NULL) return NULL;
  if(node->left != NULL)
    return TreeNodeMaximum(node->left);
  else{ /*node->left is NULL*/
    /*find the first left-ancestor*/
    treeNode *y = node->parent;
    treeNode *x = node;
    while(y != NULL && x == y->left) /*if y is a right parent of x*/
      {
	x = y;
	y = y->parent;
      }
    return y;
  }
}
