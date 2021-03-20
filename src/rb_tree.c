#include <stddef.h>
#include <stdio.h>
#include "rb_tree.h"

struct Node *init_node(void *node_place, size_t value)
{
    *(struct Node *)node_place = (struct Node){
        .color = BLACK,
        .left = NULL,
        .right = NULL,
        .parent = NULL,
        .next = NULL,
        .prev = NULL,
        .value = value,
    };
    return node_place;
}

void insert_item(struct RBTree *tree, struct Node *z)
{
    struct Node *y = NULL;
    struct Node *x = tree->root;
    z->color = RED;
    while (x)
    {
        y = x;

        if (z->value < x->value)
            x = x->left;
        else if (z->value > x->value)
            x = x->right;
        else
        {
            x->next = z;
            z->prev = x;
            return;
        }
    }

    z->parent = y;
    if (y == NULL)
    {
        tree->root = z;
        z->color = BLACK;
        return;
    }
    else if (z->value < y->value)
        y->left = z;
    else
        y->right = z;

    // RB Insert Fixup
    fix_violation(tree, z);
}

void transplant(struct RBTree *tree, struct Node *u, struct Node *v)
{
    if (!u->parent)
        tree->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v)
        v->parent = u->parent;
}

void remove_fixup(struct RBTree *tree, struct Node *x)
{
    struct Node *w;
    while (x != tree->root && x->color == BLACK)
    {
        if (x == x->parent->left)
        {
            w = x->parent->right;
            if (w->color == RED)
            {
                w->color = BLACK;
                x->parent->color = RED;
                rotate_left(tree, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK)
            {
                w->color = RED;
                x = x->parent;
            }
            else
            {
                if (w->right->color == BLACK)
                {
                    w->left->color = BLACK;
                    w->color = RED;
                    rotate_right(tree, w);
                    w = w->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rotate_left(tree, x->parent);
                x = tree->root;
            }
        }
        else
        {
            w = x->parent->left;
            if (w->color == RED)
            {
                w->color = BLACK;
                x->parent->color = RED;
                rotate_right(tree, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK)
            {
                w->color = RED;
                x = x->parent;
            }
            else
            {
                if (w->left->color == BLACK)
                {
                    w->right->color = BLACK;
                    w->color = RED;
                    rotate_left(tree, w);
                    w = w->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rotate_right(tree, x->parent);
                x = tree->root;
            }
        }
    }
    x->color = BLACK;
    return;
}

struct Node *tree_minimum(struct Node *z)
{
    for (; z->left; z = z->left)
        ;
    return z;
}

void remove_item(struct RBTree *tree, struct Node *node)
{
    if (node->prev)
    {
        node->prev->next = node->next;
        if (node->next)
            node->next->prev = node->prev;
        return;
    }
    else if (node->next)
    {
        struct Node *next = node->next;
        next->color = node->color;
        next->left = node->left;
        next->right = node->right;
        next->parent = node->parent;
        if (node == tree->root)
        {
            tree->root = next;
        }
        else
        {
            if (node->parent->left == node)
                node->parent->left = next;
            else
                node->parent->right = next;
        }
        return;
    }

    struct Node *y, *z, *x;
    bool y_original_color;

    y = z = node;
    y_original_color = y->color;
    if (!z->left)
    {
        x = z->right;
        transplant(tree, z, z->right);
    }
    else if (!z->right)
    {
        x = z->left;
        transplant(tree, z, z->left);
    }
    else
    {
        y = tree_minimum(z->right);
        y_original_color = y->color;
        x = y->right;
        if (y->parent == z)
        {
            if (x)
                x->parent = y;
        }
        else
        {
            transplant(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }
    if (y_original_color == BLACK && x)
        remove_fixup(tree, x);
}

void fix_violation(struct RBTree *tree, struct Node *z)
{
    struct Node *y;
    if (!z->parent || !z->parent->parent)
    {
        tree->root->color = BLACK;
        return;
    }

    while (z->parent && z->parent->color == RED)
    {
        if (z->parent == z->parent->parent->left)
        {
            y = z->parent->parent->right;
            if (y && y->color == RED)
            {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            }
            else
            {
                if (z == z->parent->right)
                {
                    z = z->parent;
                    rotate_left(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotate_right(tree, z->parent->parent);
            }
        }
        else
        {
            y = z->parent->parent->left; // uncle
            if (y && y->color == RED)
            {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            }
            else
            {
                if (z == z->parent->left)
                {
                    z = z->parent;
                    rotate_right(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotate_left(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

void rotate_left(struct RBTree *tree, struct Node *x)
{
    struct Node *y = x->right;
    x->right = y->left;
    if (y->left)
        y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

void rotate_right(struct RBTree *tree, struct Node *x)
{
    struct Node *y = x->left;
    x->left = y->right;
    if (y->right)
        y->right->parent = x;
    y->parent = x->parent;
    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;
    y->right = x;
    x->parent = y;
}

struct Node *search(struct RBTree *tree, size_t value)
{
    struct Node *z = tree->root;

    while (z)
    {
        if (z->value == value)
            return z;

        if (z->value < value)
            z = z->right;
        else
            z = z->left;
    }

    return NULL;
}

struct Node *search_smallest_largets(struct RBTree *tree, size_t value)
{
    struct Node *z = tree->root;
    struct Node *res = NULL;

    while (z)
    {
        if (z->value == value)
            return z;

        if (z->value < value)
            z = z->right;
        else
        {
            res = z;
            z = z->left;
        }
    }

    return res;
}

void print_tree(struct RBTree *tree)
{
    printf("Tree:\n");
    if (tree->root)
        print_node(tree->root);
    printf("Tree END\n");
}

void print_node(struct Node *node)
{
    if (!node->left && !node->right)
        printf("Leaf %zu(%s)\n", node->value, node->color == RED ? "RED" : "BLACK");
    if (node->left)
        printf("From %zu(%s) to %zu(%s)\n", node->value, node->color == RED ? "RED" : "BLACK", node->left->value, node->left->color == RED ? "RED" : "BLACK");
    if (node->right)
        printf("From %zu(%s) to %zu(%s)\n", node->value, node->color == RED ? "RED" : "BLACK", node->right->value, node->right->color == RED ? "RED" : "BLACK");

    if (node->left)
        print_node(node->left);
    if (node->right)
        print_node(node->right);
}