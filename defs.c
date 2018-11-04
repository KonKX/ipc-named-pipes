#include "decs.h"

int getFifoName(int id, char *name, size_t name_max)
{
    snprintf(name, name_max, "FIFO_%d", id);
    return 0;
}

void cont(int signum)
{
    return;
}

int addMapNode(char *filename, char **filemap, int lines, fileNode **head)
{

    fileNode *newNode = malloc(sizeof(fileNode));

    if(newNode == NULL)
    {
        printf("Unable to allocate memory for new node\n");
        return EXIT_FAILURE;
    }

    strcpy(newNode -> fileName, filename);
    newNode -> lines = lines;
    newNode -> fileMap = filemap;
    newNode -> next = NULL;

    fileNode *current = *head;

    if(current != NULL)
    {
        while (current -> next != NULL)
            current = current -> next;
        current -> next = newNode;
    }
    else *head = newNode;

    return 0;
}

int addLogNode(long long arrivalTime, char *queryType, char *key, logNode **head)
{

    logNode *newNode = malloc(sizeof(logNode));

    if(newNode == NULL)
    {
        printf("Unable to allocate memory for new node\n");
        return EXIT_FAILURE;
    }

    strcpy(newNode -> queryType, queryType);
    strcpy(newNode -> key, key);
    newNode -> arrivalTime = arrivalTime;
    newNode -> pathnames = NULL;
    newNode -> next = NULL;
    newNode -> counter = 0;

    logNode *current = *head;

    if(current != NULL)
    {
        while (current -> next != NULL)
            current = current -> next;
        current -> next = newNode;
    }
    else *head = newNode;


    return 0;
}

long long current_timestamp()
{
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);

    return milliseconds;
}

TrieNode *getNode(void) //get a new tree node
{
    TrieNode *ptrNode = malloc(sizeof(TrieNode));

    ptrNode -> listPtr = NULL;
    ptrNode -> postingListPtr = NULL;

    return ptrNode;
}

void insertNode(TrieNode *root, char *key, int id, char *filename)
{
    int i;
    int keyLength = strlen(key);
    int array_length;
    void *t;

    TrieNode *temp = root; //pointer used for tree traversal
    ListNode *temp1; //pointer used for list traversal
    PostingListNode *temp2; //pointer used for posting list traversal

    for (i = 0; i < keyLength; i++)
    {

        temp1 = temp -> listPtr;

        if(temp -> listPtr == NULL) //list is empty
        {
            temp -> listPtr = malloc(sizeof(ListNode));
            temp -> listPtr -> c = key[i];
            temp -> listPtr -> nextListNode = NULL;
            temp -> listPtr -> nextTrieNode = getNode();
            temp1 = temp -> listPtr;
        }
        else
        {
            while(temp1 -> c != key[i] && temp1 -> nextListNode != NULL)
            {
                temp1 = temp1 -> nextListNode;
            }
            if (temp1 -> c != key[i] && temp1 -> nextListNode == NULL) //we have a new key
            {
                ListNode *newNode = malloc(sizeof(ListNode));
                newNode -> c = key[i];
                newNode -> nextListNode = NULL;
                newNode -> nextTrieNode = getNode();
                temp1 -> nextListNode = newNode;
                temp1 = temp1 -> nextListNode;
            }
        }
        temp = temp1 -> nextTrieNode; //traverse the tree
    }

    //update posting list

    temp2 = temp -> postingListPtr;

    if (temp -> postingListPtr == NULL) //posting list is empty
    {
        temp -> postingListPtr = malloc(sizeof(PostingListNode));
        strcpy(temp -> postingListPtr -> filename, filename);
        temp -> postingListPtr -> counter = 1;
        temp -> postingListPtr -> inLines = NULL;
        t = realloc(temp -> postingListPtr -> inLines, sizeof(int)); //array of row numbers
        temp -> postingListPtr -> inLines = t;
        temp -> postingListPtr -> inLines[0] = id;
        temp -> postingListPtr -> inLinesLength = 1;
        temp -> postingListPtr -> nextPostingListNode = NULL;
        temp2 = temp -> postingListPtr;
    }
    else
    {
        while(strcmp(temp2 -> filename, filename) && temp2 -> nextPostingListNode != NULL)
        {
            temp2 = temp2 -> nextPostingListNode;
        }
        if (strcmp(temp2 -> filename, filename) && temp2 -> nextPostingListNode == NULL) //we have a new file
        {
            PostingListNode *newPostingListNode = malloc(sizeof(PostingListNode));
            strcpy(newPostingListNode -> filename, filename);
            newPostingListNode -> counter = 1;
            t = realloc(newPostingListNode -> inLines, sizeof(int)); //array of row numbers
            newPostingListNode -> inLines = t;
            newPostingListNode -> inLines[0] = id;
            newPostingListNode -> inLinesLength = 1;
            newPostingListNode -> nextPostingListNode = NULL;
            temp2 -> nextPostingListNode = newPostingListNode;
        }
        else
        {
            if (id != temp2 -> inLines[temp2 -> inLinesLength - 1])
            {
                t = realloc(temp2 -> inLines, (temp2 -> inLinesLength + 1) * sizeof(int));
                temp2 -> inLines = t;
                temp2 -> inLines[temp2 -> inLinesLength] = id;
                temp2 -> inLinesLength++;
            }

            temp2 -> counter++;
        }
    }

}

int search_q(TrieNode *root, char **t, int c, fileNode *files, logNode *log, long long timeStamp)
{
    int i, j, fd, num;
    int keyLength, flag;
    TrieNode *temp;
    ListNode *temp1;
    PostingListNode *temp2;
    PostingListNode *temp3;
    char buff[BUFFER_SIZE];
    char fifo_name[NAME_SIZE];

    memset(buff, '\0', sizeof(buff));
    memset(fifo_name, '\0', sizeof(fifo_name));

    char *pCursor = buff;

    fileNode *tmp;
    logNode *tmp1;
    void *tm;

    PostingListNode **PostingListPtrs = malloc(c * sizeof(PostingListNode*)); //array of pointers to posting lists
    for(i = 0; i < c; i++) PostingListPtrs[i] = NULL;

    for(i = 0; i < c; i++)
    {
        temp = root;
        keyLength = strlen(t[i]);
        flag = 0;

        for (j = 0; j < keyLength; j++)
        {
            temp1 = temp -> listPtr;

            if (temp1 == NULL)
            {
                flag = 1;
                break;
            }

            while(temp1 -> c != t[i][j] && temp1 -> nextListNode != NULL)
            {
                temp1 = temp1 -> nextListNode;
            }
            if (temp1 -> c != t[i][j] && temp1 -> nextListNode == NULL)
            {
                flag = 1;
                break;
            }
            temp = temp1 -> nextTrieNode;
        }

        temp2 = temp -> postingListPtr;

        if (flag == 1)
        {
            continue;
        }

        PostingListPtrs[i] = temp2; //address of posting list

    }
    for(i = 0; i < c; i++)
    {

        if (PostingListPtrs[i] != NULL)
        {
            temp3 = PostingListPtrs[i];

            while(temp3 != NULL)
            {
                tmp = files;

                while(strcmp(tmp -> fileName, temp3 -> filename))
                {
                    tmp = tmp -> next;
                }

                pCursor += sprintf(pCursor, "KEYWORD: %s IN ", t[i]);
                pCursor += sprintf(pCursor, "FILE: %s\n", temp3 -> filename);

                tmp1 = log;
                while(tmp1 != NULL)
                {
                    if ((tmp1 -> arrivalTime == timeStamp) && !strcmp(tmp1 -> key, t[i]))
                    {
                        tm = realloc(tmp1 -> pathnames, (tmp1 -> counter + 1) * sizeof(char*));
                        if (tm == NULL)
                        {
                            printf("Failed to resize memory block\n");
                            return EXIT_FAILURE;
                        }
                        tmp1 -> pathnames = tm;
                        tmp1 -> pathnames[tmp1 -> counter] = malloc(strlen(temp3 -> filename) + 1);
                        strcpy(tmp1 -> pathnames[tmp1 -> counter], temp3 -> filename);
                        tmp1 -> counter++;
                    }
                    tmp1 = tmp1 -> next;
                }

                for(j = 0; j < temp3 -> inLinesLength; j++)
                {
                    pCursor += sprintf(pCursor, "ROW: %d\n", temp3 -> inLines[j]);
                    pCursor += sprintf(pCursor, "%s\n", tmp -> fileMap[temp3 -> inLines[j]]);

                }

                temp3 = temp3 -> nextPostingListNode;

            }
        }
    }

    getFifoName(getpid(), fifo_name, sizeof(fifo_name));

    if(access(fifo_name, F_OK) < 0)
    {
        if (mkfifo(fifo_name, 0666) < 0)
        {
            perror("Error in mkfifo");
            return EXIT_FAILURE;
        }
        kill(getppid(), SIGUSR1);
    }
    if ((fd = open(fifo_name, O_WRONLY)) < 0)
    {
        perror("Cannot open FIFO for write");
        return EXIT_FAILURE;
    }
    if ((num = write(fd, buff, sizeof(buff))) < 0)
    {
        perror("Cannot write FIFO");
        return EXIT_FAILURE;
    }

    close(fd);


    return 0;
}

int maxcount_q(TrieNode *root, char *key, logNode *log, long long timeStamp)
{
    int i, j, fd, num;
    int keyLength, flag;
    TrieNode *temp;
    ListNode *temp1;
    PostingListNode *temp2;
    PostingListNode *temp3;
    fileCounter filecounter;

    char fifo_name[NAME_SIZE];
    char buffer[NAME_SIZE];


    logNode *tmp1;
    void *tm;

    memset(buffer, '\0', sizeof(buffer));
    memset(fifo_name, '\0', sizeof(fifo_name));
    char *pCursor = buffer;

    temp = root;
    keyLength = strlen(key);
    flag = 0;

    for (j = 0; j < keyLength; j++)
    {
        temp1 = temp -> listPtr;

        if (temp1 == NULL)
        {
            flag = 1;
            break;
        }

        while(temp1 -> c != key[j] && temp1 -> nextListNode != NULL)
        {
            temp1 = temp1 -> nextListNode;
        }
        if (temp1 -> c != key[j] && temp1 -> nextListNode == NULL)
        {
            flag = 1;
            break;
        }
        temp = temp1 -> nextTrieNode;
    }

    temp2 = temp -> postingListPtr;

    if (flag == 1) temp2 = NULL;

    if (temp2 != NULL)
    {
        strcpy(filecounter.filename, temp2 -> filename);
        filecounter.counter = temp2 -> counter;

        while (temp2 != NULL)
        {
            tmp1 = log;
            while(tmp1 != NULL)
            {
                if ((tmp1 -> arrivalTime == timeStamp) && !strcmp(tmp1 -> key, key))
                {
                    tm = realloc(tmp1 -> pathnames, (tmp1 -> counter + 1) * sizeof(char*));
                    if (tm == NULL)
                    {
                        printf("Failed to resize memory block\n");
                        return EXIT_FAILURE;
                    }
                    tmp1 -> pathnames = tm;
                    tmp1 -> pathnames[tmp1 -> counter] = malloc(strlen(temp2 -> filename) + 1);
                    strcpy(tmp1 -> pathnames[tmp1 -> counter], temp2 -> filename);
                    tmp1 -> counter++;
                }
                tmp1 = tmp1 -> next;
            }

            if (temp2 -> counter > filecounter.counter)
            {
                strcpy(filecounter.filename, temp2 -> filename);
                filecounter.counter = temp2 -> counter;
            }
            temp2 = temp2 -> nextPostingListNode;
        }
    }
    else
    {
        strcpy(filecounter.filename, "NULL");
        filecounter.counter = 0;
    }

    pCursor += sprintf(pCursor, "%s ", filecounter.filename);
    pCursor += sprintf(pCursor, "%d", filecounter.counter);


    getFifoName(getpid(), fifo_name, sizeof(fifo_name));

    if(access(fifo_name, F_OK) < 0)
    {
        if (mkfifo(fifo_name, 0666) < 0)
        {
            perror("Error in mkfifo");
            return EXIT_FAILURE;
        }
        kill(getppid(), SIGUSR1);
    }
    if ((fd = open(fifo_name, O_WRONLY)) < 0)
    {
        perror("Cannot open FIFO for write");
        return EXIT_FAILURE;
    }
    if ((num = write(fd, buffer, sizeof(buffer))) < 0)
    {
        perror("Cannot write FIFO");
        return EXIT_FAILURE;
    }

    close(fd);



    return 0;
}

int mincount_q(TrieNode *root, char *key, logNode *log, long long timeStamp)
{
    int i, j, fd, num;
    int keyLength, flag;
    TrieNode *temp;
    ListNode *temp1;
    PostingListNode *temp2;
    PostingListNode *temp3;
    fileCounter filecounter;

    char fifo_name[NAME_SIZE];
    char buffer[NAME_SIZE];

    logNode *tmp1;
    void *tm;

    memset(buffer, '\0', sizeof(buffer));
    memset(fifo_name, '\0', sizeof(fifo_name));
    char *pCursor = buffer;

    temp = root;
    keyLength = strlen(key);
    flag = 0;

    for (j = 0; j < keyLength; j++)
    {
        temp1 = temp -> listPtr;

        if (temp1 == NULL)
        {
            flag = 1;
            break;
        }

        while(temp1 -> c != key[j] && temp1 -> nextListNode != NULL)
        {
            temp1 = temp1 -> nextListNode;
        }
        if (temp1 -> c != key[j] && temp1 -> nextListNode == NULL)
        {
            flag = 1;
            break;
        }
        temp = temp1 -> nextTrieNode;
    }

    temp2 = temp -> postingListPtr;

    if (flag == 1) temp2 = NULL;

    if (temp2 != NULL)
    {
        strcpy(filecounter.filename, temp2 -> filename);
        filecounter.counter = temp2 -> counter;

        while (temp2 != NULL)
        {
            tmp1 = log;
            while(tmp1 != NULL)
            {
                if ((tmp1 -> arrivalTime == timeStamp) && !strcmp(tmp1 -> key, key))
                {
                    tm = realloc(tmp1 -> pathnames, (tmp1 -> counter + 1) * sizeof(char*));
                    if (tm == NULL)
                    {
                        printf("Failed to resize memory block\n");
                        return EXIT_FAILURE;
                    }
                    tmp1 -> pathnames = tm;
                    tmp1 -> pathnames[tmp1 -> counter] = malloc(strlen(temp2 -> filename) + 1);
                    strcpy(tmp1 -> pathnames[tmp1 -> counter], temp2 -> filename);
                    tmp1 -> counter++;
                }
                tmp1 = tmp1 -> next;
            }

            if ((temp2 -> counter < filecounter.counter) && (temp2 -> counter >= 1))
            {
                strcpy(filecounter.filename, temp2 -> filename);
                filecounter.counter = temp2 -> counter;
            }
            else if (temp2 -> counter == 1)
            {
                strcpy(filecounter.filename, temp2 -> filename);
                filecounter.counter = temp2 -> counter;
            }
            temp2 = temp2 -> nextPostingListNode;
        }
    }
    else
    {
        strcpy(filecounter.filename, "NULL");
        filecounter.counter = 0;
    }

    pCursor += sprintf(pCursor, "%s ", filecounter.filename);
    pCursor += sprintf(pCursor, "%d", filecounter.counter);


    getFifoName(getpid(), fifo_name, sizeof(fifo_name));

    if(access(fifo_name, F_OK) < 0)
    {
        if (mkfifo(fifo_name, 0666) < 0)
        {
            perror("Error in mkfifo");
            return EXIT_FAILURE;
        }
        kill(getppid(), SIGUSR1);
    }
    if ((fd = open(fifo_name, O_WRONLY)) < 0)
    {
        perror("Cannot open FIFO for write");
        return EXIT_FAILURE;
    }
    if ((num = write(fd, buffer, sizeof(buffer))) < 0)
    {
        perror("Cannot write FIFO");
        return EXIT_FAILURE;
    }

    close(fd);



    return 0;
}

int wc_q(fileNode *files)
{
    int i, j, len, bCount, wCount, lCount, fd, num, flag;
    char last;
    char buffer[NAME_SIZE];
    char fifo_name[NAME_SIZE];
    fileNode *temp;

    char *pCursor = buffer;

    memset(buffer, '\0', sizeof(buffer));
    memset(fifo_name, '\0', sizeof(fifo_name));

    bCount = wCount = lCount = 0;
    temp = files;

    while(temp != NULL)
    {
        for(i = 0; i < temp -> lines; i++)
        {
            flag = 0;
            len = strlen(temp -> fileMap[i]);
            bCount += len;

            last = temp -> fileMap[i][0];
            for(j = 0; j < len; j++)
            {
                if((temp -> fileMap[i][j] == ' ' || temp -> fileMap[i][j] == '\0') && last != ' ')
                {
                    wCount++;
                }
                last = temp -> fileMap[i][j];
            }
            for(j = 0; j < len; j++)
            {
                if(temp -> fileMap[i][j] == '\n')
                {
                    flag = 1;
                    lCount++;
                }
            }
            if (!flag) lCount++;
        }

        temp = temp -> next;
    }


    pCursor += sprintf(pCursor, "%d ", bCount);
    pCursor += sprintf(pCursor, "%d ", wCount);
    pCursor += sprintf(pCursor, "%d", lCount);

    getFifoName(getpid(), fifo_name, sizeof(fifo_name));

    if(access(fifo_name, F_OK) < 0)
    {
        if (mkfifo(fifo_name, 0666) < 0)
        {
            perror("Error in mkfifo");
            return EXIT_FAILURE;
        }
        kill(getppid(), SIGUSR1);
    }
    if ((fd = open(fifo_name, O_WRONLY)) < 0)
    {
        perror("Cannot open FIFO for write");
        return EXIT_FAILURE;
    }
    if ((num = write(fd, buffer, sizeof(buffer))) < 0)
    {
        perror("Cannot write FIFO");
        return EXIT_FAILURE;
    }

    close(fd);


    return 0;
}

void free_trie(TrieNode *node) //recursive
{
    int i;
    ListNode *temp1 = node -> listPtr;
    PostingListNode *temp2 = node -> postingListPtr;

    if (node == NULL) return;

    while(temp1)
    {
        free_trie(temp1 -> nextTrieNode);

        ListNode *tmp1 = temp1;
        temp1 = temp1 -> nextListNode;
        free(tmp1);
    }
    while(temp2)
    {
        PostingListNode *tmp2 = temp2;
        temp2 = temp2 -> nextPostingListNode;
        free(tmp2 -> inLines);
        free(tmp2);
    }

    free(node);
}
