#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#define BUFFER_SIZE 48000
#define NAME_SIZE 1000

typedef struct PostingListNode
{
    char filename[NAME_SIZE];
    int counter;
    int *inLines;
    int inLinesLength;
    struct PostingListNode *nextPostingListNode;
} PostingListNode;

typedef struct TrieNode
{
    struct ListNode *listPtr;
    struct PostingListNode *postingListPtr;
} TrieNode;

typedef struct ListNode
{
    char c;
    struct ListNode *nextListNode;
    struct TrieNode *nextTrieNode;
} ListNode;

typedef struct fileNode
{
    int lines;
    char fileName[NAME_SIZE];
    char **fileMap;
    struct fileNode *next;
} fileNode;

typedef struct fileCounter
{
    char filename[NAME_SIZE];
    int counter;
} fileCounter;

typedef struct logNode
{
    long long arrivalTime;
    char queryType[NAME_SIZE];
    char key[NAME_SIZE];
    char **pathnames;
    int counter;
    struct logNode *next;
} logNode;

int getFifoName(int, char *, size_t );
void cont(int );
int addMapNode(char *, char **, int, fileNode **);
TrieNode *getNode(void);
void insertNode(TrieNode *, char *, int, char *);
int search_q(TrieNode *, char **, int c, fileNode *, logNode *, long long);
int maxcount_q(TrieNode *, char *, logNode *, long long);
int mincount_q(TrieNode *, char *, logNode *, long long);
int wc_q(fileNode *);
long long current_timestamp();
int addLogNode(long long, char *, char *, logNode **);
void free_trie(TrieNode *);
