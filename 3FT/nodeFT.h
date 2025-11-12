/*--------------------------------------------------------------------*/
/* nodeFT.h                                                           */
/* Author: Christopher Moretti                                        */
/*--------------------------------------------------------------------*/

#ifndef NODEFT_INCLUDED
#define NODEFT_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"


/* A Node_T is a node in a File Tree(directory or file) */
typedef struct node *Node_T;

/* to distinguish between directory and file nodes */
enum NodeType {FT_DIR, FT_FILE};

/*
  Creates a new node in the File Tree, with path oPPath,parent oNParent. 
  and the node type. 
  Returns an int SUCCESS status and sets *poNResult
  to be the new node if successful. Otherwise, sets *poNResult to NULL
  and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
*/
int Node_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult);

/*
  Destroys and frees all memory allocated for the subtree rooted at
  oNNode, i.e., deletes this node and all its descendents. Returns the
  number of nodes deleted.
  for file nodes, frees the contents of the nodes and then the node itself.
*/
size_t Node_free(Node_T oNNode);

/* Returns the path object representing oNNode's absolute path. */
Path_T Node_getPath(Node_T oNNode);

/*
  (just for directory nodes)
  Returns TRUE if oNParent has a child with path oPPath. Returns
  FALSE if it does not.

  If oNParent has such a child, stores in *pulChildID the child's
  identifier (as used in Node_getChild). If oNParent does not have
  such a child, stores in *pulChildID the identifier that such a
  child _would_ have if inserted.
*/
boolean Node_hasChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID);

/* 
(just for directory nodes)
Returns the number of children that oNParent has. 
*/
size_t Node_getNumChildren(Node_T oNParent);

/*
  (just for directory nodes)
  Returns an int SUCCESS status and sets *poNResult to be the child
  node of oNParent with identifier ulChildID, if one exists.
  Otherwise, sets *poNResult to NULL and returns status:
  * NO_SUCH_PATH if ulChildID is not a valid child for oNParent
*/
int Node_getChild(Node_T oNParent, size_t ulChildID,
                  Node_T *poNResult);

/*
  Returns a the parent node of oNNode.
  Returns NULL if oNNode is the root and thus has no parent.
*/
Node_T Node_getParent(Node_T oNNode);

/*
  Compares oNFirst and oNSecond lexicographically based on their paths.
  Returns <0, 0, or >0 if onFirst is "less than", "equal to", or
  "greater than" oNSecond, respectively.
*/
int Node_compare(Node_T oNFirst, Node_T oNSecond);

/*
  Returns a string representation for oNNode, or NULL if
  there is an allocation error.

  Allocates memory for the returned string, which is then owned by
  the caller!
*/
char *Node_toString(Node_T oNNode);

/* Returns TRUE if node oNNode represents a file and FALSE if directory*/
enum NodeType Node_isFile(Node_T oNNode);

/* Returns pointer to the stored contents of the file oNNode 
   Returns NULL if directory 
*/
void *Node_getFileContents(Node_T oNNode);

/* Returns the byte length of the file oNNode's content
   Return 0 for directories 
*/
size_t Node_getFileLength(Node_T oNNode);

/*
  Replaces current contents of the file node oNNode with pvNewContents
  of size ulNewLength. Returns a pointer to the old contents if successful.
  otherwise returns NULL if direcotry or if there's an allocation error.
*/
void *Node_replaceFileContents(Node_T oNNode, const char *pvNewContents,
                             size_t ulNewLength);

#endif /* NODEFT_INCLUDED */
