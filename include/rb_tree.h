#pragma once
#include <stdbool.h>

#define NODE_SIZE sizeof(struct Node)

enum Color
{
    RED,
    BLACK,
};

struct Node
{
    size_t value;
    struct Node *left, *right, *parent;
    bool color;
    // data
    struct Node *next;
};

struct RBTree
{
    struct Node *root;
};

struct Node *init_node(void *node_place, size_t value);

void insert_item(struct RBTree *tree, struct Node *node_place);

void remove_item(struct RBTree *tree, struct Node *ptr);

void fixViolation(struct RBTree *tree, struct Node *ptr);

void rotateLeft(struct RBTree *tree, struct Node *ptr);

void rotateRight(struct RBTree *tree, struct Node *ptr);

struct Node *search(struct RBTree *tree, size_t value);

struct Node *searchSmallestLargets(struct RBTree *tree, size_t value);

void print_tree(struct RBTree *tree);

void print_node(struct Node *node);