#ifndef LZSS_H
#define LZSS_H

#include <stdio.h>

typedef unsigned long long ull;

#define INDEX_BIT 12
#define LENGTH_BIT 4
#define WINDOW_SIZE ((ull)1<<(INDEX_BIT))
#define RAW_LOOK_AHEAD_SIZE ((ull)1<<(LENGTH_BIT))
#define BREAK_EVEN ((1+(INDEX_BIT)+(LENGTH_BIT))/9)
#define LOOK_AHEAD_SIZE ((RAW_LOOK_AHEAD_SIZE)+(BREAK_EVEN))
#define TREE_ROOT WINDOW_SIZE
#define END_OF_STREAM 0
#define UNUSED 0
#define MOD_WINDOW(x) ((x)&((WINDOW_SIZE)-1))

struct Tree{
	int parent;
	int smaller_child;
	int larger_child;	
};

typedef struct Tree Tree;

extern void InitOutputBuffer(void);
extern int FlushOutputBuffer(void);
extern int OutputChar(int data);
extern int OutputPair(int position,int length);
extern void InitInputBuffer(void);
extern int InputBit(void);
extern int LZSSCompress(FILE *input_text_file);
extern int AddString(int new_node,int *match_position);
extern void InitTree(int root);
extern void DeleteString(int node);
extern void RaiseNode(int old_node,int new_node);
extern void ReplaceNode(int old_node,int new_node);
extern int LeftMost(int node);
extern uint32_t LZSSExpand(FILE *output);

#endif
