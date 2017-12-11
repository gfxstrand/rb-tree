rb-tree: A simple, invasive, red-black tree written in C
========================================================

This is a simple, invasive, liberally licensed red-black tree implementation
written in C.  It's an invasive data structure similar to the [Linux kernel
linked-list][1] where the intention is that you embed a rb_node struct the data
structure you intend to put into the tree.

## Why another red-black tree implementation?

I needed a red-black tree for a project I was working on. Unfortunately, most
of the free red-black tree implementations floating around the Internet are GPL
which is not a liberal enough license for my needs.  This implementation is
distributed under the MIT license which means it can be used in basically
anything.

## Implementation notes

The implementation is mostly based on the one in ["Introduction to Algorithms",
third edition, by Cormen, Leiserson, Rivest, and Stein][2].  There were a few
other key design points:

 * It's an invasive data structure similar to the [Linux kernel linked list].

 * It uses NULL for leaves instead of a sentinel.  This means a few algorithms
   differ a small bit from the ones in "Introduction to Algorithms".

 * All search operations are inlined so that the compiler can optimize away the
   function pointer call.

[1]: https://github.com/torvalds/linux/blob/master/include/linux/list.h
[2]: https://mitpress.mit.edu/books/introduction-algorithms
