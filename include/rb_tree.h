#pragma once
#include <stdbool.h>

#define NODE_SIZE align(sizeof(struct Node))

enum Color
{
    RED,
    BLACK,
};

struct Node
{
    struct Node *left, *right, *parent;
    struct Node *next;
    struct Node *prev;
    bool color;
    size_t value;
};

struct RBTree
{
    struct Node *root;
};

static struct Node RBNIL = {
    .color = BLACK,
    .parent = &RBNIL,
    .left = &RBNIL,
    .right = &RBNIL,
    .prev = NULL,
    .next = NULL,
    .value = 0,
};

struct Node *init_node(void *node_place, size_t value);

void insert_item(struct RBTree *tree, struct Node *node_place);

void remove_item(struct RBTree *tree, struct Node *ptr);

void fix_violation(struct RBTree *tree, struct Node *ptr);

void rotate_left(struct RBTree *tree, struct Node *x);

void rotate_right(struct RBTree *tree, struct Node *x);

struct Node *search(struct RBTree *tree, size_t value);

struct Node *search_smallest_largets(struct RBTree *tree, size_t value);

void print_tree(struct RBTree *tree);

void print_node(struct Node *node);