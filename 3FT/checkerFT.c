/*--------------------------------------------------------------------*/
/* checkerFT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerFT.h"
#include "dynarray.h"
#include "path.h"
#include "nodeFT.h"

/* helper function checking the parent(oNParent)-child(oNChild) relation invariants, index-child's
position in the parent's child arr, totChildren, number of children the parent reports. 
Returns:
     TRUE  – if the parent–child relationship satisfies all invariants
     FALSE – if any structural, ordering, or pointer invariant is violated */
static boolean checkerFT_Child_isValid(Node_T oNParent, Node_T oNChild,
                                       size_t index, size_t totChildren) {
    Path_T oPPPath;
    Path_T oPNPath;
    Node_T oNOtherChild;
    Node_T oNPrevChild;
    size_t j;
    
    assert(oNParent != NULL);

    oPNPath = Node_getPath(oNChild);
    oPPPath = Node_getPath(oNParent);

    /* child shouldn't be NULL */
    if(oNChild == NULL) {
        fprintf(stderr, "Child shouldn't be NULL\n");
        return FALSE;
    }

    /* the child's pointer should be the same as the parent */
    if(Node_getParent(oNChild) != oNParent) {
      fprintf(stderr, "Child's parent pointer doesn't match the parnet\n");
      return FALSE;
    }
    
    if(oPPPath == NULL || oPNPath == NULL) {
        fprintf(stderr, "parent or child path is NULL\n");
        return FALSE;
    }

    /* parent must be the longest proper prefix of child's path */
    if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
       Path_getDepth(oPNPath) - 1) {
        fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
        return FALSE;
    }

    /* adding check for duplicate children under the same parent */
    for(j = index + 1; j < totChildren; j++) {
        oNOtherChild = NULL;
        if (Node_getChild(oNParent, j, &oNOtherChild) == SUCCESS &&
            oNOtherChild != NULL) {
            if(Path_comparePath(Node_getPath(oNChild),Node_getPath(oNOtherChild))
                == 0) {
                fprintf(stderr, "duplicate child paths under parent: (%s) (%s)\n",
                        Path_getPathname(oPPPath), Path_getPathname(oPNPath));
                return FALSE;
            }
        }
      
    }
  
  /* adding check for order */
    if(index > 0) {
      oNPrevChild = NULL;
      if(Node_getChild(oNParent, index-1, &oNPrevChild) == SUCCESS &&
        oNPrevChild != NULL) {
        
        if(Path_comparePath(Node_getPath(oNPrevChild),Node_getPath(oNChild)) > 0) {
          fprintf(stderr, "children names out of order\n");
          return FALSE;
        }
      }
    }

    return TRUE;
}

/* Checks whether oNNode satisfies all File Tree structural invariants.
   Returns TRUE if the node is well-formed:
     – non-NULL node and path
     – parent/child path prefix relationships correct
     – parent pointer consistent
     – no duplicate sibling names
     – child array indices valid and match Node_getNumChildren()
   Returns FALSE if any invariant is violated.
*/
boolean CheckerFT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;
   Node_T oNChild;
   Path_T oPNPath;
   Path_T oPPPath;
   size_t ulNumChildren;
   size_t ulIndex;

   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }
    
    /* adding check that the node path is not null */
    oPNPath = Node_getPath(oNNode);
    if(oPNPath == NULL) {
        fprintf(stderr, "Node has a NULL path\n");
        return FALSE;
    }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent == NULL) {
     if(Node_isFile(oNNode)) {
       fprintf(stderr, "root node is a file\n");
       return FALSE;
     }
   }
   else {
       oPNPath = Node_getPath(oNNode);
       oPPPath = Node_getPath(oNParent);

       if(oPPPath == NULL || oPNPath == NULL) {
           fprintf(stderr, "parent or child path is NULL\n");
           return FALSE;
       }

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }


   /* adding check file node must not have children */ 
   if(Node_isFile(oNNode)) {
     if(Node_getNumChildren(oNNode) != 0) {
       fprintf(stderr, "File Node has children: (%s)\n",
               Path_getPathname(oPNPath));
       return FALSE;
     }
   }

  
   /* checking children conditions */
   ulNumChildren = Node_getNumChildren(oNNode);
   for(ulIndex = 0; ulIndex < ulNumChildren; ulIndex++) {
       oNChild = NULL;

       if(Node_getChild(oNNode, ulIndex, &oNChild) != SUCCESS) {
           fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
           return FALSE;
       }

       if(!checkerFT_Child_isValid(oNNode, oNChild, ulIndex, ulNumChildren)) {
           return FALSE;
       }     
   }

   return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.
*/
static boolean CheckerFT_treeCheck(Node_T oNNode) {
   size_t ulIndex;

   if(oNNode!= NULL) {

      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerFT_Node_isValid(oNNode))
         return FALSE;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         if(!CheckerFT_treeCheck(oNChild))
            return FALSE;
      }
   }
   return TRUE;
}

/* Recursively counts all nodes in the subtree rooted at oNNode.
   Arguments:
     oNNode – current node being visited
     pCount – pointer to the running total of nodes

   On completion:
     *pCount is incremented for oNNode and for every descendant.
*/
static void CheckerFT_CountNodes(Node_T oNNode, size_t *pCount) {
  size_t i;
  Node_T oNChild = NULL;

  assert(pCount != NULL); 

  if(oNNode == NULL) {
    return;
  }
  (*pCount)++;

  for(i = 0; i < Node_getNumChildren(oNNode); i++) {
    if (Node_getChild(oNNode, i, &oNChild) == SUCCESS && oNChild != NULL) {
      CheckerFT_CountNodes(oNChild, pCount);
    }
  }
}
/* see checkerDT.h for specification */
boolean CheckerFT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
   size_t actualCount = 0;
   
    /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
      if(oNRoot != NULL) {
          fprintf(stderr, "Not initialized, but root is not NULL\n");
          return FALSE;
      }
      return TRUE; 
   }
   /* adding check; if initialized, when root is NULL and count isn't 0 */
    if(oNRoot == NULL) {
        if(ulCount != 0) {
            fprintf(stderr, "Root is NULL but count is not 0\n");
            return FALSE;
        }
        return TRUE;
    }

    /* adding check if root has a parent */
    if(Node_getParent(oNRoot) != NULL) {
        fprintf(stderr, "Root node has parent\n");
        return FALSE;
    }

    /* root must be a directory */
    if(Node_isFile(oNRoot)) {
      fprintf(stderr,"Root node is a file\n");
      return FALSE;
    }

    /* verifying the no of nodes */
    CheckerFT_CountNodes(oNRoot, &actualCount);
    if(actualCount != ulCount) {
        fprintf(stderr, "ulCount not equal to actual number of nodes\n");
        return FALSE;
    }

    
    

   /* Now checks invariants recursively at each node from the root. */
   return CheckerFT_treeCheck(oNRoot);
}
