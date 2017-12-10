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

#include "rb_tree.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define RB_TREE_ENABLE_ASSERTS

#ifdef RB_TREE_ENABLE_ASSERTS
#   define RB_TREE_ASSERT(...) assert(__VA_ARGS__)
#else
#   define RB_TREE_ASSERT(...) (void)0
#endif

static bool
rb_node_is_black(struct rb_node *n)
{
    /* NULL nodes are leaves and therefore black */
    return (n == NULL) || (n->parent & 1);
}

static bool
rb_node_is_red(struct rb_node *n)
{
    return !rb_node_is_black(n);
}

static void
rb_node_set_black(struct rb_node *n)
{
    n->parent |= 1;
}

static void
rb_node_set_red(struct rb_node *n)
{
    n->parent &= ~1ull;
}

static void
rb_node_copy_color(struct rb_node *dst, struct rb_node *src)
{
    dst->parent = (dst->parent & ~1ull) | (src->parent & 1);
}

static struct rb_node *
rb_node_parent(struct rb_node *n)
{
    struct rb_node *p = (struct rb_node *)(n->parent & ~1ull);
    RB_TREE_ASSERT(p == NULL || n == p->left || n == p->right);
    return p;
}

static void
rb_node_set_parent(struct rb_node *n, struct rb_node *p)
{
    n->parent = (n->parent & 1) | (uintptr_t)p;
}

static struct rb_node *
rb_node_minimum(struct rb_node *node)
{
    while (node->left)
        node = node->left;
    return node;
}

void
rb_tree_init(struct rb_tree *T)
{
    T->root = NULL;
}

/**
 * Replace the subtree of T rooted at u with the subtree rooted at v
 *
 * This is called RB-transplant in CLRS.
 *
 * Both nodes are assumed to be non-leaves
 */
static void
rb_tree_splice(struct rb_tree *T, struct rb_node *u, struct rb_node *v)
{
    RB_TREE_ASSERT(u);
    RB_TREE_ASSERT(v);

    struct rb_node *p = rb_node_parent(u);
    if (p == NULL) {
        RB_TREE_ASSERT(T->root == u);
        T->root = v;
    } else if (u == p->left) {
        p->left = v;
    } else {
        p->right = v;
    }
    rb_node_set_parent(v, p);
}

static void
rb_tree_rotate_left(struct rb_tree *T, struct rb_node *x)
{
    RB_TREE_ASSERT(x);
    RB_TREE_ASSERT(x->right);

    struct rb_node *y = x->right;
    x->right = y->left;
    if (y->left)
        rb_node_set_parent(y->left, x);
    rb_tree_splice(T, x, y);
    y->left = x;
    rb_node_set_parent(x, y);
}

static void
rb_tree_rotate_right(struct rb_tree *T, struct rb_node *y)
{
    RB_TREE_ASSERT(y);
    RB_TREE_ASSERT(y->left);

    struct rb_node *x = y->left;
    y->left = x->right;
    if (x->right)
        rb_node_set_parent(x->right, y);
    rb_tree_splice(T, y, x);
    x->right = y;
    rb_node_set_parent(y, x);
}

void
rb_tree_insert_at(struct rb_tree *T, struct rb_node *parent,
                  struct rb_node *node, bool insert_left)
{
    RB_TREE_ASSERT(node);

    /* This sets null children, parent, and a color of red */
    memset(node, 0, sizeof(*node));

    if (parent == NULL) {
        RB_TREE_ASSERT(T->root == NULL);
        T->root = node;
        rb_node_set_black(node);
        return;
    }

    if (insert_left) {
        RB_TREE_ASSERT(parent->left == NULL);
        parent->left = node;
    } else {
        RB_TREE_ASSERT(parent->right == NULL);
        parent->right = node;
    }
    rb_node_set_parent(node, parent);

    /* Now we do the insertion fixup */
    struct rb_node *z = node;
    while (rb_node_is_red(rb_node_parent(z))) {
        struct rb_node *z_p = rb_node_parent(z);
        struct rb_node *z_p_p = rb_node_parent(z_p);
        RB_TREE_ASSERT(z_p_p != NULL);
        if (z_p == z_p_p->left) {
            struct rb_node *y = z_p_p->right;
            if (rb_node_is_red(y)) {
                rb_node_set_black(z_p);
                rb_node_set_black(y);
                rb_node_set_red(z_p_p);
                z = z_p_p;
            } else {
                if (z == z_p->right) {
                    z = z_p;
                    rb_tree_rotate_left(T, z);
                    /* We changed z */
                    z_p = rb_node_parent(z);
                    z_p_p = rb_node_parent(z_p);
                }
                rb_node_set_black(z_p);
                rb_node_set_red(z_p_p);
                rb_tree_rotate_right(T, z_p_p);
            }
        } else {
            struct rb_node *y = z_p_p->left;
            if (rb_node_is_red(y)) {
                rb_node_set_black(z_p);
                rb_node_set_black(y);
                rb_node_set_red(z_p_p);
                z = z_p_p;
            } else {
                if (z == z_p->left) {
                    z = z_p;
                    rb_tree_rotate_right(T, z);
                    /* We changed z */
                    z_p = rb_node_parent(z);
                    z_p_p = rb_node_parent(z_p);
                }
                rb_node_set_black(z_p);
                rb_node_set_red(z_p_p);
                rb_tree_rotate_left(T, z_p_p);
            }
        }
    }
    rb_node_set_black(T->root);
}

void
rb_tree_remove(struct rb_tree *T, struct rb_node *z)
{
    struct rb_node *x;
    struct rb_node *y = z;
    bool y_was_black = rb_node_is_black(y);
    if (z->left == NULL) {
        x = z->right;
        rb_tree_splice(T, z, z->right);
    } else if (z->right == NULL) {
        x = z->left;
        rb_tree_splice(T, z, z->left);
    } else {
        /* Find the minimum sub-node of z->right */
        y = rb_node_minimum(z->right);
        y_was_black = rb_node_is_black(y);

        x = y->right;
        if (rb_node_parent(y) == z) {
            if (x)
                rb_node_set_parent(x, y);
        } else {
            rb_tree_splice(T, y, y->right);
            y->right = z->right;
            rb_node_set_parent(y->right, y);
        }
        rb_tree_splice(T, z, y);
        y->left = z->left;
        rb_node_set_parent(y->left, y);
        rb_node_copy_color(y, z);
    }

    if (!y_was_black)
        return;

    /* Fixup RB tree after the delete */
    while (x != T->root && rb_node_is_black(x)) {
        if (x == rb_node_parent(x)->left) {
            struct rb_node *w = rb_node_parent(x)->right;
            if (rb_node_is_red(w)) {
                rb_node_set_black(w);
                rb_node_set_red(rb_node_parent(x));
                rb_tree_rotate_left(T, rb_node_parent(x));
                w = rb_node_parent(x)->right;
            }
            if (rb_node_is_black(w->left) && rb_node_is_black(w->right)) {
                rb_node_set_red(w);
                x = rb_node_parent(x);
            } else {
                if (rb_node_is_black(w->right)) {
                    rb_node_set_black(w->left);
                    rb_node_set_red(w);
                    rb_tree_rotate_right(T, w);
                    w = rb_node_parent(x)->right;
                }
                rb_node_copy_color(w, rb_node_parent(x));
                rb_node_set_black(rb_node_parent(x));
                rb_node_set_black(w->right);
                rb_tree_rotate_left(T, rb_node_parent(x));
                x = T->root;
            }
        } else {
            struct rb_node *w = rb_node_parent(x)->left;
            if (rb_node_is_red(w)) {
                rb_node_set_black(w);
                rb_node_set_red(rb_node_parent(x));
                rb_tree_rotate_right(T, rb_node_parent(x));
                w = rb_node_parent(x)->left;
            }
            if (rb_node_is_black(w->right) && rb_node_is_black(w->left)) {
                rb_node_set_red(w);
                x = rb_node_parent(x);
            } else {
                if (rb_node_is_black(w->left)) {
                    rb_node_set_black(w->right);
                    rb_node_set_red(w);
                    rb_tree_rotate_left(T, w);
                    w = rb_node_parent(x)->left;
                }
                rb_node_copy_color(w, rb_node_parent(x));
                rb_node_set_black(rb_node_parent(x));
                rb_node_set_black(w->left);
                rb_tree_rotate_right(T, rb_node_parent(x));
                x = T->root;
            }
        }
    }
    rb_node_set_black(x);
}

struct rb_node *
rb_tree_first(struct rb_tree *T)
{
    return rb_node_minimum(T->root);
}

struct rb_node *
rb_node_next(struct rb_node *node)
{
    if (node->right) {
        /* If we have a right child, then the next thing (compared to this
         * node) is the left-most child of our right child.
         */
        return rb_node_minimum(node->right);
    } else {
        /* If node doesn't have a right child, crawl back up the to the
         * left until we hit a parent to the right.
         */
        struct rb_node *p = rb_node_parent(node);
        while (p && node == p->right) {
            node = p;
            p = rb_node_parent(node);
        }
        RB_TREE_ASSERT(p == NULL || node == p->left);
        return p;
    }
}

static void
validate_rb_node(struct rb_node *n, int black_depth)
{
    if (n == NULL) {
        assert(black_depth == 0);
        return;
    }

    if (rb_node_is_black(n)) {
        black_depth--;
    } else {
        assert(rb_node_is_black(n->left));
        assert(rb_node_is_black(n->right));
    }

    validate_rb_node(n->left, black_depth);
    validate_rb_node(n->right, black_depth);
}

void
rb_tree_validate(struct rb_tree *T)
{
    if (T->root == NULL)
        return;

    RB_TREE_ASSERT(rb_node_is_black(T->root));

    unsigned black_depth = 0;
    for (struct rb_node *n = T->root; n; n = n->left) {
        if (rb_node_is_black(n))
            black_depth++;
    }

    validate_rb_node(T->root, black_depth);
}
