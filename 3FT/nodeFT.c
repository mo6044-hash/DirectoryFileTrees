/*--------------------------------------------------------------------*/
/* nodeFT.c                                                           */
/* Author: Helenia                                                    */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"
#include "nodeFT.h"
#include "checkerFT.h" 

/* A node in a DT */
struct node {
   /* the object corresponding to the node's absolute path */
   Path_T oPPath;
   /* this node's parent */
   Node_T oNParent;
   /* the object containing links to this node's children */
   DynArray_T oDChildren;
   /* boolean for distinguishing file from directory */
   boolean bIsFile;
   /* file node contents and their length (only for files)*/
   void *pvContents;
   /* size of file cotents */
   size_t ulLength;

};


/*
  Links new child oNChild into oNParent's children array at index
  ulIndex. Returns SUCCESS if the new child was added successfully,
  or  MEMORY_ERROR if allocation fails adding oNChild to the array.
*/
static int Node_addChild(Node_T oNParent, Node_T oNChild,
                         size_t ulIndex) {
   assert(oNParent != NULL);
   assert(oNChild != NULL);
   assert(!oNParent->bIsFile); /* parent must be a directory */ 

   if(DynArray_addAt(oNParent->oDChildren, ulIndex, oNChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}

/*
  Compares the string representation of oNfirst with a string
  pcSecond representing a node's path.
  Returns <0, 0, or >0 if oNFirst is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int Node_compareString(const Node_T oNFirst,
                                 const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   /* return Path_compareString(oNFirst->oPPath, pcSecond); */
   return strcmp(Path_getPathname(oNFirst->oPPath), pcSecond); 
}


/*
  Creates a new node with path oPPath and parent oNParent.  Returns an
  int SUCCESS status and sets *poNResult to be the new node if
  successful. Otherwise, sets *poNResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
*/
int Node_new(Path_T oPPath, Node_T oNParent, boolean bIsFile, void *pvContents, 
             size_t ulLength, Node_T *poNResult) {
   struct node *psNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex;
   int iStatus;

   assert(oPPath != NULL);
   assert(poNResult != NULL);
   assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent)); 

   /* allocate space for a new node */
   psNew = malloc(sizeof(struct node));
   if(psNew == NULL) {
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   /* set the new node's path */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(psNew);
      *poNResult = NULL;
      return iStatus;
   }
   psNew->oPPath = oPNewPath;

   /*setting the file info; contents, length*/
   psNew->bIsFile = bIsFile;
   
   if(bIsFile) {
      if(ulLength > 0 && pvContents != NULL) {
         psNew->pvContents = malloc(ulLength);
         if(psNew->pvContents== NULL) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return MEMORY_ERROR;
         }
         memcpy(psNew->pvContents, pvContents, ulLength);
      }

      else {
         psNew->pvContents = NULL;
      }

      psNew->ulLength = ulLength;
      psNew->oDChildren = NULL; /* since files have no childre */
   }

   else {
      /* initializing the directory */
      psNew->pvContents = NULL;
      psNew->ulLength = 0;
      psNew->oDChildren = DynArray_new(0);
      if(psNew->oDChildren == NULL) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return MEMORY_ERROR;
      }
   }

   /* validate and set the new node's parent */
   if(oNParent != NULL) {
      size_t ulSharedDepth;

      oPParentPath = oNParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);
      /* parent must be an ancestor of child */
      if(ulSharedDepth < ulParentDepth) {
         if(psNew->oDChildren != NULL) {
            DynArray_free(psNew->oDChildren);
         }
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent must be exactly one level up from child */
      if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
         if(psNew->oDChildren != NULL) {
            DynArray_free(psNew->oDChildren);
         }
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }

      /* parent should not be file */
      if (oNParent->bIsFile) {
         if(psNew->oDChildren != NULL) {
            DynArray_free(psNew->oDChildren);
         }
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NOT_A_DIRECTORY;
      }

      /* parent must not already have child with this path */
      if(Node_hasChild(oNParent, oPPath, &ulIndex)) {
         if(psNew->oDChildren != NULL) {
            DynArray_free(psNew->oDChildren);
         }
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return ALREADY_IN_TREE;
      } 
      

      /* Link into parent's children list */
      psNew->oNParent = oNParent;   
      iStatus = Node_addChild(oNParent, psNew, ulIndex);
      if(iStatus != SUCCESS) {
         if(psNew->oDChildren != NULL) {
            DynArray_free(psNew->oDChildren);
         }
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return iStatus;
      }
   }
   
   else {
      /* new node must be root */
      /* can only create one "level" at a time */
      if(Path_getDepth(psNew->oPPath) != 1) {
         if(psNew->oDChildren != NULL) {
            DynArray_free(psNew->oDChildren);
         }
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
      psNew->oNParent = NULL;
   }
   
   *poNResult = psNew;

   assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));
   assert(CheckerFT_Node_isValid(*poNResult)); 

   return SUCCESS;
}


size_t Node_free(Node_T oNNode) {
   size_t ulIndex;
   size_t ulCount = 0;

   assert(oNNode != NULL);
   assert(CheckerFT_Node_isValid(oNNode)); 

   /* remove from parent's list */
   if(oNNode->oNParent != NULL && !oNNode->oNParent->bIsFile) {
      if(DynArray_bsearch(
            oNNode->oNParent->oDChildren,
            oNNode, &ulIndex,
            (int (*)(const void *, const void *)) Node_compare)
        )
         (void) DynArray_removeAt(oNNode->oNParent->oDChildren,
                                  ulIndex);
   }

   /* recursively remove children (Directory only)*/
   if(!oNNode->bIsFile && oNNode->oDChildren != NULL) {
      while(DynArray_getLength(oNNode->oDChildren) != 0) {
         ulCount += Node_free(DynArray_get(oNNode->oDChildren, 0));
      }
      DynArray_free(oNNode->oDChildren);
   }

   /*free file content */
   if(oNNode->bIsFile && oNNode->pvContents != NULL) {
      free(oNNode->pvContents);
   }

   /* remove path */
   Path_free(oNNode->oPPath);

   /* finally, free the struct node */
   free(oNNode);
   ulCount++;
   return ulCount;
}


Path_T Node_getPath(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oPPath;
}

/* for directory */
boolean Node_hasChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID) {
   assert(oNParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   if(oNParent->bIsFile) {
      *pulChildID = 0;
      return FALSE;
   }

   /* *pulChildID is the index into oNParent->oDChildren */
   return DynArray_bsearch(oNParent->oDChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) Node_compareString);
}

size_t Node_getNumChildren(Node_T oNParent) {
   assert(oNParent != NULL);
   if (oNParent->bIsFile) {
      return 0;
   }
   return DynArray_getLength(oNParent->oDChildren);
}

int  Node_getChild(Node_T oNParent, size_t ulChildID,
                   Node_T *poNResult) {

   assert(oNParent != NULL);
   assert(poNResult != NULL);

   if(oNParent->bIsFile) {
      *poNResult = NULL;
      return NOT_A_DIRECTORY;
   }

   /* ulChildID is the index into oNParent->oDChildren */
   if(ulChildID >= Node_getNumChildren(oNParent)) {
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }
   else {
      *poNResult = DynArray_get(oNParent->oDChildren, ulChildID);
      return SUCCESS;
   }
}

Node_T Node_getParent(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oNParent;
}

int Node_compare(Node_T oNFirst, Node_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   /* return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath); */
   return strcmp(Path_getPathname(Node_getPath(oNFirst)), 
                 Path_getPathname(Node_getPath(oNSecond))); 
   
}

char *Node_toString(Node_T oNNode) {
   char *copyPath;

   assert(oNNode != NULL);

   copyPath = malloc(Path_getStrLength(Node_getPath(oNNode))+1);
   if(copyPath == NULL)
      return NULL;
   else
      return strcpy(copyPath, Path_getPathname(Node_getPath(oNNode)));
}

/* for file */
boolean Node_isFile(Node_T oNNode) {
   assert(oNNode != NULL);
   return oNNode->bIsFile;
}

void *Node_getFileContents(Node_T oNNode) {
   assert(oNNode != NULL);
   if(oNNode->bIsFile) {
      return oNNode->pvContents;
   }
   else {
      return NULL;
   }
}

size_t Node_getFileLength(Node_T oNNode) {
   assert(oNNode != NULL);
   if(oNNode->bIsFile) {
      return oNNode->ulLength;
   }
   else {
      return 0;
   }
}

void *Node_replaceFileContents(Node_T oNNode, const char *pvNewContents, 
                              size_t ulNewLength) {
   void *pvOldContents;
   void *pvNew;
   assert(oNNode != NULL);
   assert(CheckerFT_Node_isValid(oNNode));

   if(!oNNode->bIsFile) {
      return NULL;
   }

   /* store old contents to return later */
   pvOldContents = oNNode->pvContents;

   if(ulNewLength == 0) {
      oNNode->pvContents = NULL;
      free(pvOldContents);
   }

   /* replace with new contents */
   else {
      pvNew = malloc(ulNewLength);
      if(!pvNew) {
         /* restore old contents on failure 
         oNNode->pvContents = pvOldContents; */
         return NULL;
      }
      memcpy(pvNew, pvNewContents, ulNewLength);
      oNNode->pvContents = pvNew;
   }
   oNNode->ulLength = ulNewLength;
   assert(CheckerFT_Node_isValid(oNNode));

   return pvOldContents;
}
