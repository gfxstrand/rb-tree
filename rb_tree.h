/*
 * Copyright © 2017 Jason Ekstrand
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef RB_TREE_H
#define RB_TREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct rb_node {
    uintptr_t parent;
    struct rb_node *left;
    struct rb_node *right;
};

struct rb_tree {
    struct rb_node *root;
};

void rb_tree_init(struct rb_tree *T);

/** Insert a node into a tree at a particular location
 *
 * This function should probably not be used directly as it relies on the
 * caller to ensure that the parent node is correct.  Use rb_tree_insert
 * instead.
 *
 * \param   T           The red-black tree into which to insert the new node
 *
 * \param   parent      The node in the tree that will be the parent of the
 *                      newly inserted node
 *
 * \param   node        The node to insert
 *
 * \param   insert_left If true, the new node will be the left child of
 *                      \p parent, otherwise it will be the right child
 */
void rb_tree_insert_at(struct rb_tree *T, struct rb_node *parent,
                       struct rb_node *node, bool insert_left);

/** Insert a node into a tree
 *
 * \param   T       The red-black tree into which to insert the new node
 *
 * \param   node    The node to insert
 *
 * \param   cmp     A comparison function to use to order the nodes.
 */
static inline void
rb_tree_insert(struct rb_tree *T, struct rb_node *node,
               bool (*cmp)(const struct rb_node *, const struct rb_node *))
{
    /* This function is declared inline in the hopes that the compiler can
     * optimize away the comparison function pointer call.
     */
    struct rb_node *y = NULL;
    struct rb_node *x = T->root;
    bool left = false;
    while (x != NULL) {
        y = x;
        left = cmp(node, x);
        if (left)
            x = x->left;
        else
            x = x->right;
    }

    rb_tree_insert_at(T, y, node, left);
}

/** Remove a node from a tree
 *
 * \param   T       The red-black tree from which to remove the node
 *
 * \param   node    The node to remove
 */
void rb_tree_remove(struct rb_tree *T, struct rb_node *z);

/** Validate a red-black tree
 *
 * This function walks the tree and validates that this is a valid red-
 * black tree.  If anything is wrong, it will assert-fail.
 */
void rb_tree_validate(struct rb_tree *T);

#endif /* RB_TREE_H */
