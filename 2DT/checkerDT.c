/*Implements internal invariant checks for the Directory
  Tree (DT) data structure. Ensures node validity, parent–child
  relationships, prefix correctness.*/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"
#include "nodeDT.h"

#define CHECKERDT_INIT_CAPACITY 16

/*
  Return index of node in visited array, or -1 if not present.
*/
static long CheckerDT_visited_index(DynArray_T arr, Node_T node) {
   size_t i;
   for (i = 0; i < DynArray_getLength(arr); i++) {
      if (DynArray_get(arr, i) == node)
         return (long)i;
   }
   return -1;
}

/*
  Returns a newly allocated duplicate of string s.
*/
static char *CheckerDT_my_strdup(const char *s) {
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

/*--------------------------------------------------------------------*/
/* Validate a single DT node according to all structural invariants.  */
/*--------------------------------------------------------------------*/
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;
   Path_T oPNPath;
   Path_T oPPPath;

   size_t parentDepth;
   size_t nodeDepth;
   Path_T prefix;

   size_t declared;
   char **names = NULL;
   size_t namesCount = 0;

   size_t i;
   size_t k;

   /* NULL node is invalid */
   if (oNNode == NULL) {
      fprintf(stderr, "CheckerDT_Node_isValid: node is a NULL pointer\n");
      return FALSE;
   }

   /* node path cannot be NULL */
   oPNPath = Node_getPath(oNNode);
   if (oPNPath == NULL) {
      fprintf(stderr, "CheckerDT_Node_isValid: node path is NULL\n");
      return FALSE;
   }

   /* parent-child prefix checks */
   oNParent = Node_getParent(oNNode);
   if (oNParent != NULL) {

      oPPPath = Node_getPath(oNParent);
      if (oPPPath == NULL) {
         fprintf(stderr, "CheckerDT_Node_isValid: parent path is NULL\n");
         return FALSE;
      }

      if (Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
          Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath),
                 Path_getPathname(oPNPath));
         return FALSE;
      }

      parentDepth = Path_getDepth(oPPPath);
      nodeDepth = Path_getDepth(oPNPath);

      if (parentDepth + 1 > nodeDepth) {
         fprintf(stderr,
                 "CheckerDT_Node_isValid: invalid depths: parent=%lu child=%lu\n",
                 (unsigned long)parentDepth,
                 (unsigned long)nodeDepth);
         return FALSE;
      }

      prefix = NULL;
      if (Path_prefix(oPNPath, parentDepth, &prefix) != SUCCESS) {
         fprintf(stderr, "CheckerDT_Node_isValid: Path_prefix failed\n");
         return FALSE;
      }

      if (Path_comparePath(prefix, oPPPath) != 0) {
         fprintf(stderr,
                 "CheckerDT_Node_isValid: parent path is not prefix of child path\n");
         fprintf(stderr, "  parent: %s\n  child:  %s\n",
                 Path_getPathname(oPPPath),
                 Path_getPathname(oPNPath));

         Path_free(prefix);
         return FALSE;
      }

      Path_free(prefix);
   }

   /* child checks: duplicate names, ordering, and parent pointer consistency */
   declared = Node_getNumChildren(oNNode);

   for (i = 0; i < declared; i++) {
      Node_T child = NULL;
      int status = Node_getChild(oNNode, i, &child);

      if (status != SUCCESS) {
         fprintf(stderr,
                 "CheckerDT_Node_isValid: Node_getChild failed at index %lu\n",
                 (unsigned long)i);

         if (names != NULL) {
            for (k = 0; k < namesCount; k++) free(names[k]);
            free(names);
         }
         return FALSE;
      }

      if (child == NULL) continue;

      /* child->parent must match oNNode */
      if (Node_getParent(child) != oNNode) {
         fprintf(stderr,
                 "CheckerDT_Node_isValid: child's parent pointer mismatch\n");
         fprintf(stderr, "  parent: %s\n  child:  %s\n",
                 Path_getPathname(Node_getPath(oNNode)),
                 Path_getPathname(Node_getPath(child)));

         if (names != NULL) {
            for (k = 0; k < namesCount; k++) free(names[k]);
            free(names);
         }
         return FALSE;
      }

      /* prevent duplicate sibling names and enforce lexicographic order */
      {
         Path_T cpath;
         const char *base;
         char *copy;
         char **tmp;

         cpath = Node_getPath(child);
         if (cpath == NULL) {
            fprintf(stderr, "CheckerDT_Node_isValid: child path is NULL\n");

            if (names != NULL) {
               for (k = 0; k < namesCount; k++) free(names[k]);
               free(names);
            }
            return FALSE;
         }

         base = Path_getPathname(cpath);
         if (base == NULL) {
            fprintf(stderr, "CheckerDT_Node_isValid: child basename is NULL\n");

            if (names != NULL) {
               for (k = 0; k < namesCount; k++) free(names[k]);
               free(names);
            }
            return FALSE;
         }

         /* enforce lexicographic order among siblings */
         if (namesCount > 0 &&
             strcmp(names[namesCount-1], base) > 0) {
            fprintf(stderr,
                    "CheckerDT_Node_isValid: children out of order under parent %s\n",
                    Path_getPathname(Node_getPath(oNNode)));

            if (names != NULL) {
               for (k = 0; k < namesCount; k++) free(names[k]);
               free(names);
            }
            return FALSE;
         }

         /* check for duplicate sibling names */
         for (k = 0; k < namesCount; k++) {
            if (strcmp(names[k], base) == 0) {
               fprintf(stderr,
                       "CheckerDT_Node_isValid: duplicate sibling '%s'\n",
                       base);

               if (names != NULL) {
                  for (k = 0; k < namesCount; k++) free(names[k]);
                  free(names);
               }
               return FALSE;
            }
         }

         /* add base to names array */
         copy = CheckerDT_my_strdup(base);
         if (copy == NULL) {
            fprintf(stderr, "CheckerDT_Node_isValid: malloc failed\n");

            if (names != NULL) {
               for (k = 0; k < namesCount; k++) free(names[k]);
               free(names);
            }
            return FALSE;
         }

         tmp = realloc(names, (namesCount + 1) * sizeof(char *));
         if (tmp == NULL) {
            free(copy);
            fprintf(stderr, "CheckerDT_Node_isValid: realloc failed\n");

            if (names != NULL) {
               for (k = 0; k < namesCount; k++) free(names[k]);
               free(names);
            }
            return FALSE;
         }

         names = tmp;
         names[namesCount++] = copy;
      }
   }

   /* free sibling names */
   if (names != NULL) {
      for (k = 0; k < namesCount; k++) free(names[k]);
      free(names);
   }

   return TRUE;
}

/*
   Performs a preorder traversal of the subtree rooted at oNRoot.
   Checks each node for validity, detects cycles, and counts all nodes.
   Returns:
      the number of nodes visited on success
      -1 if any invariant check fails or a cycle is detected
*/
static long CheckerDT_traverse_and_check(Node_T oNRoot) {
   DynArray_T visited;
   DynArray_T stack;
   size_t top = 0;
   size_t visited_count = 0;

   if (oNRoot == NULL) return 0;

   visited = DynArray_new(CHECKERDT_INIT_CAPACITY);
   if (visited == NULL) {
      fprintf(stderr, "CheckerDT: DynArray_new failed (visited)\n");
      return -1;
   }

   stack = DynArray_new(CHECKERDT_INIT_CAPACITY);
   if (stack == NULL) {
      fprintf(stderr, "CheckerDT: DynArray_new failed (stack)\n");
      DynArray_free(visited);
      return -1;
   }

   (void)DynArray_set(stack, 0, oNRoot);
   top = 1;

   while (top > 0) {
      size_t numChildren;
      Node_T curr;

      top--;
      curr = DynArray_get(stack, top);

      if (CheckerDT_visited_index(visited, curr) != -1) {
         fprintf(stderr, "CheckerDT: cycle detected: node %s visited twice\n",
                 Path_getPathname(Node_getPath(curr)));

         DynArray_free(visited);
         DynArray_free(stack);
         return -1;
      }

      (void)DynArray_set(visited, visited_count, curr);
      visited_count++;

      if (!CheckerDT_Node_isValid(curr)) {
         DynArray_free(visited);
         DynArray_free(stack);
         return -1;
      }

      numChildren = Node_getNumChildren(curr);

      /* push children in reverse order */
      {
         long j = (long)numChildren - 1;
         while (j >= 0) {
            Node_T child = NULL;

            if (Node_getChild(curr, (size_t)j, &child) == SUCCESS &&
                child != NULL) {

               (void)DynArray_set(stack, top, child);
               top++;
            }
            j--;
         }
      }
   }

   DynArray_free(visited);
   DynArray_free(stack);
   return (long)visited_count;
}

/*--------------------------------------------------------------------*/
/* Validate entire DT according to high-level invariants.             */
/*--------------------------------------------------------------------*/
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   Node_T rootParent;
   long traversal_count;

   if (!bIsInitialized) {
      if (oNRoot != NULL) {
         fprintf(stderr,
                 "CheckerDT_isValid: not initialized but root != NULL\n");
         return FALSE;
      }
      if (ulCount != 0) {
         fprintf(stderr,
                 "CheckerDT_isValid: not initialized but ulCount=%lu\n",
                 (unsigned long)ulCount);
         return FALSE;
      }
      return TRUE;
   }

   if (oNRoot == NULL) {
      if (ulCount != 0) {
         fprintf(stderr,
                 "CheckerDT_isValid: root NULL but ulCount=%lu\n",
                 (unsigned long)ulCount);
         return FALSE;
      }
      return TRUE;
   }

   rootParent = Node_getParent(oNRoot);
   if (rootParent != NULL) {
      fprintf(stderr, "CheckerDT_isValid: root's parent not NULL\n");
      return FALSE;
   }

   traversal_count = CheckerDT_traverse_and_check(oNRoot);
   if (traversal_count < 0)
      return FALSE;

   if ((size_t)traversal_count != ulCount) {
      fprintf(stderr,
              "CheckerDT_isValid: count mismatch: recorded=%lu, actual=%ld\n",
              (unsigned long)ulCount,
              traversal_count);
      return FALSE;
   }

   return TRUE;
}
