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
