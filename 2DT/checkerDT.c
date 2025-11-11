/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"
#include "nodeDT.h"


static long visited_index(DynArray_T arr, Node_T node);

/*Returns a newly allocated duplicate of the string s.*/
static char *my_strdup(const char *s) {
    size_t len;
    char *copy;

    if (s == NULL) return NULL;
    len = strlen(s) + 1;

    copy = malloc(len);
    if (copy != NULL) {
        memcpy(copy, s, len);
    }
    return copy;
}

/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;
   Path_T oPNPath;
   Path_T oPPPath;

   size_t parentDepth;
   size_t nodeDepth;
   Path_T prefix;

   size_t declared;
   char **names;
   size_t namesCount;

   size_t i, k, m;

   /* NULL node is invalid */
   if (oNNode == NULL) {
      fprintf(stderr, "CheckerDT_Node_isValid: node is a NULL pointer\n");
      return FALSE;
   }

   /* parent's path must be the longest possible proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if (oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if (oPNPath == NULL || oPPPath == NULL) {
         fprintf(stderr, "CheckerDT_Node_isValid: NULL path on node or parent\n");
         return FALSE;
      }

      if (Path_getSharedPrefixDepth(oPNPath, oPPPath) != Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }

   /* ensure node path is not NULL */
   oPNPath = Node_getPath(oNNode);
   if (oPNPath == NULL) {
      fprintf(stderr, "CheckerDT_Node_isValid: node path is NULL\n");
      return FALSE;
   }

   /* re-check parent-related invariants more explicitly (depth and prefix) */
   if (oNParent != NULL) {
      oPPPath = Node_getPath(oNParent);
      if (oPPPath == NULL) {
         fprintf(stderr, "CheckerDT_Node_isValid: parent path is NULL\n");
         return FALSE;
      }

      parentDepth = Path_getDepth(oPPPath);
      nodeDepth = Path_getDepth(oPNPath);

      if (!(parentDepth + 1 <= nodeDepth)) {
         fprintf(stderr, "CheckerDT_Node_isValid: invalid depths: parentDepth = %lu childDepth = %lu\n",
                 (unsigned long)parentDepth, (unsigned long)nodeDepth);
         return FALSE;
      }

      prefix = NULL;
      if (Path_prefix(oPNPath, parentDepth, &prefix) != SUCCESS) {
         fprintf(stderr, "CheckerDT_Node_isValid: Path_prefix failed for node\n");
         return FALSE;
      }
      if (Path_comparePath(prefix, oPPPath) != 0) {
         fprintf(stderr, "CheckerDT_Node_isValid: parent path is not prefix of child path\n");
         fprintf(stderr, "  parent: %s\n  child:  %s\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         Path_free(prefix);
         return FALSE;
      }
      Path_free(prefix);
   }

   /* child-related checks: parent pointer consistency, declared vs actual, sibling uniqueness */
   declared = Node_getNumChildren(oNNode);
   names = NULL;
   namesCount = 0;

   i = 0;
   while (i < declared) {
      Node_T child = NULL;
      int st = Node_getChild(oNNode, i, &child);

      if (st != SUCCESS) {
         fprintf(stderr, "CheckerDT_Node_isValid: Node_getChild failed at index %lu\n",
                 (unsigned long)i);
         goto free_and_fail;
      }

      if (child != NULL) {
         Node_T cparent = Node_getParent(child);
         Path_T cpath;
         const char *base;
         char *copy;
         char **tmp;

         if (cparent != oNNode) {
            fprintf(stderr, "CheckerDT_Node_isValid: child's parent pointer doesn't point back to parent\n");
            fprintf(stderr, "  parent: %s\n  child:  %s\n",
                    Path_getPathname(Node_getPath(oNNode)),
                    Path_getPathname(Node_getPath(child)));
            goto free_and_fail;
         }

         cpath = Node_getPath(child);
         if (cpath == NULL) {
            fprintf(stderr, "CheckerDT_Node_isValid: child's path is NULL\n");
            goto free_and_fail;
         }

         base = Path_getPathname(cpath);
         if (base == NULL) {
            fprintf(stderr, "CheckerDT_Node_isValid: child's basename is NULL\n");
            goto free_and_fail;
         }

         for (k = 0; k < namesCount; k++) {
            if (strcmp(names[k], base) == 0) {
               fprintf(stderr, "CheckerDT_Node_isValid: duplicate sibling name '%s' under parent %s\n",
                       base, Path_getPathname(Node_getPath(oNNode)));
               goto free_and_fail;
            }
         }

         copy = my_strdup(base);
         if (copy == NULL) {
            fprintf(stderr, "CheckerDT_Node_isValid: memory allocation error\n");
            goto free_and_fail;
         }

         tmp = realloc(names, (namesCount + 1) * sizeof(char *));
         if (tmp == NULL) {
            free(copy);
            fprintf(stderr, "CheckerDT_Node_isValid: memory allocation error (realloc)\n");
            goto free_and_fail;
         }

         names = tmp;
         names[namesCount++] = copy;
      }
      i++;
   }

   /* finished child loop: free allocated sibling name copies */
   for (k = 0; k < namesCount; k++) free(names[k]);
   free(names);

   return TRUE;

free_and_fail:
   for (m = 0; m < namesCount; m++) free(names[m]);
   free(names);
   return FALSE;
}

/* cycle detection helper */
static long visited_index(DynArray_T arr, Node_T node) {
   size_t i;
   for (i = 0; i < DynArray_getLength(arr); i++) {
      if (DynArray_get(arr, i) == node)
         return (long)i;
   }
   return -1;
}

/*
  Traverses tree rooted at oNRoot in pre-order,
  performs node checks, detects cycles, and counts nodes.
*/
static long traverse_and_check(Node_T oNRoot) {
    DynArray_T visited;
    DynArray_T stack;
    size_t top;
    size_t visited_count;

    if (oNRoot == NULL) return 0;

    visited = DynArray_new(16);
    if (visited == NULL) {
        fprintf(stderr, "CheckerDT: DynArray_new failed\n");
        return -1;
    }

    stack = DynArray_new(16);
    if (stack == NULL) {
        fprintf(stderr, "CheckerDT: DynArray_new failed for stack\n");
        DynArray_free(visited);
        return -1;
    }

    DynArray_set(stack, 0, oNRoot);
    top = 1;
    visited_count = 0;

    while (top > 0) {
        size_t numChildren;
        Node_T curr;
        long j;

        top--;
        curr = DynArray_get(stack, top);

        if (visited_index(visited, curr) != -1) {
            fprintf(stderr, "CheckerDT: cycle detected: node %s visited twice\n",
                    Path_getPathname(Node_getPath(curr)));
            DynArray_free(visited);
            DynArray_free(stack);
            return -1;
        }

        DynArray_set(visited, visited_count, curr);
        visited_count++;

        if (!CheckerDT_Node_isValid(curr)) {
            DynArray_free(visited);
            DynArray_free(stack);
            return -1;
        }

        numChildren = Node_getNumChildren(curr);

        j = (long) numChildren - 1;
        while (j >= 0) {
            Node_T child = NULL;
            int st = Node_getChild(curr, (size_t)j, &child);
            if (st != SUCCESS) break;
            if (child != NULL) {
                DynArray_set(stack, top, child);
                top++;
            }
            j--;
        }
    }

    DynArray_free(visited);
    DynArray_free(stack);
    return (long) visited_count;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

    Node_T rootParent;
    long traversal_count;

    if (!bIsInitialized) {
        if (oNRoot != NULL) {
            fprintf(stderr, "CheckerDT_isValid: not initialized but root != NULL\n");
            return FALSE;
        }
        if (ulCount != 0) {
            fprintf(stderr, "CheckerDT_isValid: not initialized but count != 0 (=%lu)\n",
                    (unsigned long)ulCount);
            return FALSE;
        }
        return TRUE;
    }

    if (oNRoot == NULL) {
        if (ulCount != 0) {
            fprintf(stderr, "CheckerDT_isValid: root is NULL but ulCount != 0 (=%lu)\n",
                    (unsigned long)ulCount);
            return FALSE;
        }
        return TRUE;
    }

    rootParent = Node_getParent(oNRoot);
    if (rootParent != NULL) {
        fprintf(stderr, "CheckerDT_isValid: root's parent is not NULL\n");
        return FALSE;
    }

    traversal_count = traverse_and_check(oNRoot);
    if (traversal_count < 0)
        return FALSE;

    if ((size_t)traversal_count != ulCount) {
        fprintf(stderr, "CheckerDT_isValid: ulCount mismatch: recorded=%lu traversed=%ld\n",
                (unsigned long)ulCount, traversal_count);
        return FALSE;
    }

    return TRUE;
}
