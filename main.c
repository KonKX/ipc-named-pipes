#include "decs.h"

int main(int argc, char *argv[])
{

    int opt = 0;
    char* docfile = NULL;
    char* numWorkers = NULL;
    int workersA = 0;
    // run with ./jobExecutor –d docfile –w numWorkers

    while ((opt = getopt(argc, argv, "d:w:")) != -1)
    {
        switch(opt)
        {
        case 'd':
            docfile = optarg;
            break;
        case 'w':
            numWorkers = optarg;
            break;
        }
    }

    int workersNum = atoi(numWorkers);
    int lines = 1;
    char ch;

    FILE *fp = fopen(docfile, "r");
    if (fp == NULL)
    {
        perror("Could not open the file.");
        return EXIT_FAILURE;
    }
    else //count number of lines
    {
        ch=fgetc(fp);
        while(ch!=EOF)
        {
            if(ch=='\n')
            {
                lines++;
            }
            ch=fgetc(fp);
        }
    }
    rewind(fp); //reset pointer

    struct stat st = {0};

    if (stat("log", &st) == -1) //check if directory exists
    {
        mkdir("log", 0700);
    }

    int i, j, fd, num, status, n = 0;
    pid_t pid;
    pid_t pid_arr[workersNum];
    char buffer[BUFFER_SIZE];
    char fifo_name[NAME_SIZE];
    memset(buffer, '\0', sizeof(buffer));
    memset(fifo_name, '\0', sizeof(fifo_name));

    char **fifoNames = malloc(workersNum * sizeof(char*));
    for(i = 0; i < workersNum; i++)
    {
        getFifoName(i, fifo_name, sizeof(fifo_name));
        fifoNames[i] = malloc(strlen(fifo_name) + 1);
        strcpy(fifoNames[i], fifo_name);
        if (mkfifo(fifoNames[i], 0666) < 0)
        {
            perror("Error in mkfifo");
            return EXIT_FAILURE;
        }
    }

    for(i = 0; i < workersNum; i++)
    {
        if((pid = fork())< 0)
        {
            perror("Error in fork");
            return EXIT_FAILURE;
        }
        else if(!pid) break;
        pid_arr[i] = pid;
    }
    if(pid == 0) //I'm the child
    {
        //printf("I'm child %d\n", i);
        char path[NAME_SIZE];
        memset(path, '\0', sizeof(path));
        int j, n, total_n = 0;
        DIR *d;
        struct dirent *dir;

        TrieNode *root = NULL; //Tree root
        root = getNode();

        if ((fd = open(fifoNames[i], O_RDONLY)) < 0)
        {
            perror("Cannot open FIFO for read");
            return EXIT_FAILURE;
        }

        char **pathNames = NULL; //array of path names
        void *tmp;
        int m = 1;
        int c = 0;
        char full_path[NAME_SIZE];
        memset(full_path, '\0', sizeof(full_path));

        fileNode *head = NULL;
        logNode *log = NULL;

        while ((num = read(fd, buffer, sizeof(buffer))) > 0)
        {
            while(sscanf(buffer + total_n, "%[^\n]%n", path, &n) == 1)
            {
                printf("I'm child %d and I got %s\n", i, path);
                total_n = total_n + n + 1;
                strcpy(full_path, path);

                if ((d = opendir(path)) != NULL)
                {
                    while ((dir = readdir(d)) != NULL)
                    {
                        if (strcmp(dir -> d_name, ".") && strcmp(dir -> d_name, ".."))
                        {
                            tmp = realloc(pathNames, m * sizeof(char*));
                            if (tmp == NULL)
                            {
                                printf("Failed to resize memory block\n");
                                return EXIT_FAILURE;
                            }
                            pathNames = tmp;
                            pathNames[c] = malloc(sizeof(full_path));

                            strcat(full_path, "/");
                            strcat(full_path, dir -> d_name);
                            strcpy(pathNames[c], full_path);

                            memset(full_path, '\0', sizeof(full_path));
                            strcpy(full_path, path);

                            c++;
                            m++;

                            //printf("%s\n", dir -> d_name);
                        }
                    }
                    closedir(d);
                }
            }
            FILE *f;
            char **filemap;

            int l = 0;
            int line = 1;

            for (j = 0; j < c; j++)
            {
                printf("About to open %s\n", pathNames[j]);
                f = fopen(pathNames[j], "r");

                if (f == NULL)
                {
                    printf("Error opening input file\n");
                    return -1;
                }
                else //count number of lines in file
                {
                    ch = fgetc(f);
                    while(ch != EOF)
                    {
                        if(ch == '\n')
                        {
                            line++;
                        }
                        ch = fgetc(f);
                    }
                    rewind(f);
                }

                filemap = malloc(line * sizeof(char*));

                memset(buffer, '\0', sizeof(buffer));

                while(fgets(buffer, sizeof(buffer), f) != NULL)
                {
                    filemap[l] = malloc((strlen(buffer) + 1) * sizeof(char));
                    sscanf(buffer,"%[^\n]", filemap[l]);
                    l++;
                }

                char key[NAME_SIZE]; //key buffer
                int k;
                char *sc = NULL;
                int wtf;
                total_n = 0;

                for(k = 0; k < line; k++) //create index
                {
                    while (sscanf(filemap[k] + total_n, "%s%n", key, &n) == 1)
                    {
                        insertNode(root, key, k, pathNames[j]);
                        total_n += n;
                    }
                    total_n = 0;
                }

                addMapNode(pathNames[j], filemap, line, &head);

                l = 0;
                line = 1;
            }
        }
        if (num < 0)
        {
            perror("Cannot read from FIFO");
            return EXIT_FAILURE;
        }
        close(fd);

        if(head != NULL)
        {
            memset(buffer, '\0', sizeof(buffer));
            strcpy(buffer, "active");
            if ((fd = open(fifoNames[i], O_WRONLY)) < 0)
            {
                perror("Cannot open FIFO for write");
                return EXIT_FAILURE;
            }
            if ((num = write(fd, buffer, strlen(buffer))) < 0)
            {
                perror("Cannot write FIFO");
                return EXIT_FAILURE;
            }
            close(fd);
        }
        else
        {
            memset(buffer, '\0', sizeof(buffer));
            strcpy(buffer, "not active");
            if ((fd = open(fifoNames[i], O_WRONLY)) < 0)
            {
                perror("Cannot open FIFO for write");
                return EXIT_FAILURE;
            }
            if ((num = write(fd, buffer, strlen(buffer))) < 0)
            {
                perror("Cannot write FIFO");
                return EXIT_FAILURE;
            }
            close(fd);
        }

        signal(SIGUSR1, cont);
        pause(); //waiting for parent signal...

        //ready for query

        memset(buffer, '\0', sizeof(buffer));

        char **t = NULL; //array of keys
        int g;
        long long timeStamp;

        while(strcmp(buffer, "/exit"))
        {

            if ((fd = open(fifoNames[i], O_RDONLY)) < 0)
            {
                perror("Cannot open FIFO for read");
                return EXIT_FAILURE;
            }

            if ((num = read(fd, buffer, sizeof(buffer))) < 0)
            {
                perror("Cannot read from FIFO");
                return EXIT_FAILURE;
            }

            close(fd);

            //printf("I'm child %d and my command is %s \n", i , buffer);

            char command[NAME_SIZE];
            char key[NAME_SIZE];
            memset(command, '\0', sizeof(command));
            memset(key, '\0', sizeof(key));

            void *tm; //temp variable

            g = 0; //number of keys
            m = 1;

            sscanf(buffer, "%s%n", command, &n);
            total_n += n;


            if (!strcmp(command, "/search"))
            {
                while (sscanf(buffer + total_n, "%s%n", key, &n) == 1)
                {
                    timeStamp = current_timestamp();
                    addLogNode(timeStamp, "/search", key, &log);

                    tm = realloc(t, m * sizeof(char*));
                    if (tm == NULL)
                    {
                        printf("Failed to resize memory block\n");
                        return EXIT_FAILURE;
                    }
                    t = tm;
                    t[g] = malloc((strlen(key) + 1) * sizeof(char));
                    strcpy(t[g], key);

                    g++;
                    m++;
                    total_n += n;

                }

                if (head != NULL)
                {
                    search_q(root, t, g, head, log, timeStamp);

                }
            }
            else if (!strcmp(command, "/maxcount"))
            {

                sscanf(buffer + total_n, "%s%n", key, &n);

                timeStamp = current_timestamp();
                addLogNode(timeStamp, "/maxcount", key, &log);

                if (head != NULL)
                {
                    maxcount_q(root, key, log, timeStamp);
                }
            }
            else if (!strcmp(command, "/mincount"))
            {
                sscanf(buffer + total_n, "%s%n", key, &n);

                timeStamp = current_timestamp();
                addLogNode(timeStamp, "/mincount", key, &log);

                if (head != NULL)
                {
                    mincount_q(root, key, log, timeStamp);
                }
            }
            else if (!strcmp(command, "/wc"))
            {
                if (head != NULL)
                {
                    wc_q(head);
                }
            }


            if (strcmp(buffer, "/exit"))
            {
                signal(SIGUSR1, cont);
                pause(); //waiting for parent signal...
                memset(buffer, '\0', sizeof(buffer));
                memset(command, '\0', sizeof(command));
                memset(key, '\0', sizeof(key));
                total_n = 0;
            }

        }

        if (head != NULL)
        {
            char logfile[NAME_SIZE];
            snprintf(logfile, NAME_SIZE, "log/Worker_%d", getpid());
            strcat(logfile,".txt");

            FILE *pf = fopen(logfile, "w");

            logNode *tem = NULL;
            tem = log;
            while(tem != NULL)
            {
                fprintf(pf, "%lld ms : ", tem -> arrivalTime);
                fprintf(pf, "%s : ", tem -> queryType);
                fprintf(pf, "%s : ", tem -> key);
                for(j = 0; j < tem -> counter; j++)
                    fprintf(pf, "%s : ", tem -> pathnames[j]);
                fprintf(pf, "\n");
                tem = tem -> next;
            }
        }

        printf("Exiting worker with process ID %d\n", getpid());

        //free memory
        free_trie(root);
        for(j = 0; j < c; j++)
            free(pathNames[j]);
        free(pathNames);

        fileNode *temp = NULL;
        temp = head;
        while(temp)
        {
            fileNode *tmp = temp;
            for(j = 0; j < tmp -> lines; j++)
                free(tmp -> fileMap[j]);
            free(tmp -> fileMap);
            temp = temp -> next;
            free(tmp);
        }
        logNode *temp1 = NULL;
        temp1 = log;
        while(temp1)
        {
            logNode *tmp1 = temp1;
            for(j = 0; j < tmp1 -> counter; j++)
                free(tmp1 -> pathnames[j]);
            free(tmp1 -> pathnames);
            temp1 = temp1 -> next;
            free(tmp1);
        }

        exit(0);

    }
    else //parent
    {
        //puts("I'm the writer");
        int chunkSize, k;

        for (j = 0; j < workersNum; j++)
        {
            if ((fd = open(fifoNames[j], O_WRONLY)) < 0)
            {
                perror("Cannot open FIFO for write");
                return EXIT_FAILURE;
            }

            chunkSize = lines / workersNum;

            if (chunkSize == 0) chunkSize++; //more readers than paths in file

            for (k = 0; k < chunkSize; k++)
            {
                if ((chunkSize * workersNum) < lines) chunkSize++;

                if (fgets(buffer, sizeof(buffer), fp))
                {
                    if ((num = write(fd, buffer, strlen(buffer))) < 0)
                    {
                        perror("Cannot write FIFO");
                        return EXIT_FAILURE;
                    }
                }
                else break;
            }

            close(fd);
        }
        for (j = 0; j < workersNum; j++) //check how many workers have files
        {
            memset(buffer, '\0', sizeof(buffer));
            if ((fd = open(fifoNames[j], O_RDONLY)) < 0)
            {
                perror("Cannot open FIFO for read");
                return EXIT_FAILURE;
            }
            do
            {
                if ((num = read(fd, buffer, sizeof(buffer))) < 0)
                {
                    perror("Cannot read from FIFO");
                    return EXIT_FAILURE;
                }
            }
            while (num > 0);

            if(!strcmp(buffer, "active")) workersA++;

            close(fd);
        }
        //end of preprocessing

        int total_n = 0;
        char command[BUFFER_SIZE];
        memset(command, '\0', sizeof(command));

        while(strcmp(command, "/exit"))
        {
            memset(buffer, '\0', sizeof(buffer));
            memset(command, '\0', sizeof(command));
            fgets(buffer, sizeof(buffer), stdin);
            sscanf(buffer, "%s%n", command, &n);
            total_n += n;

            if (!strcmp(command, "/search"))
            {
                for (j = 0; j < workersA; j++)
                {
                    kill(pid_arr[j], SIGUSR1);

                    if ((fd = open(fifoNames[j], O_WRONLY)) < 0)
                    {
                        perror("Cannot open FIFO for write");
                        return EXIT_FAILURE;
                    }
                    if ((num = write(fd, buffer, strlen(buffer) - 1)) < 0) //-1 to skip \n
                    {
                        perror("Cannot write FIFO");
                        return EXIT_FAILURE;
                    }
                    close(fd);
                }
                int flag;

                while(1)
                {
                    flag = 0;
                    for(j = 0; j < workersA; j++)
                    {
                        memset(fifo_name, '\0', sizeof(fifo_name));
                        getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));

                        if (access(fifo_name, F_OK) < 0)
                        {
                            flag = 1;
                            signal(SIGUSR1, cont);
                            pause(); //waiting for child signal...
                        }
                    }
                    if (!flag) break;
                }

                for(j = 0; j < workersA; j++)
                {
                    memset(fifo_name, '\0', sizeof(fifo_name));
                    getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));

                    if ((fd = open(fifo_name, O_RDONLY)) < 0)
                    {
                        perror("Cannot open FIFO for read");
                        return EXIT_FAILURE;
                    }
                    do
                    {
                        if ((num = read(fd, buffer, sizeof(buffer))) < 0)
                        {
                            perror("Cannot read from FIFO");
                            return EXIT_FAILURE;
                        }
                    }
                    while (num > 0);
                    close(fd);

                    printf("%s\n", buffer);

                }
            }
            else if(!strcmp(command, "/maxcount"))
            {
                for (j = 0; j < workersA; j++)
                {
                    kill(pid_arr[j], SIGUSR1);

                    if ((fd = open(fifoNames[j], O_WRONLY)) < 0)
                    {
                        perror("Cannot open FIFO for write");
                        return EXIT_FAILURE;
                    }
                    if ((num = write(fd, buffer, strlen(buffer) - 1)) < 0) //-1 to skip \n
                    {
                        perror("Cannot write FIFO");
                        return EXIT_FAILURE;
                    }
                    close(fd);
                }
                int flag;

                while(1)
                {
                    flag = 0;
                    for(j = 0; j < workersA; j++)
                    {
                        memset(fifo_name, '\0', sizeof(fifo_name));
                        getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));

                        if (access(fifo_name, F_OK) < 0)
                        {
                            flag = 1;
                            signal(SIGUSR1, cont);
                            pause(); //waiting for child signal...
                        }
                    }
                    if (!flag) break;
                }

                fileCounter *counter = calloc(workersA, sizeof(fileCounter));
                fileCounter max;
                int l = 0;

                for(j = 0; j < workersA; j++)
                {
                    memset(fifo_name, '\0', sizeof(fifo_name));
                    getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));

                    if ((fd = open(fifo_name, O_RDONLY)) < 0)
                    {
                        perror("Cannot open FIFO for read");
                        return EXIT_FAILURE;
                    }
                    do
                    {
                        if ((num = read(fd, buffer, sizeof(buffer))) < 0)
                        {
                            perror("Cannot read from FIFO");
                            return EXIT_FAILURE;
                        }
                    }
                    while (num > 0);

                    sscanf(buffer, "%s%d", counter[l].filename, &counter[l].counter);
                    l++;
                    close(fd);
                }

                max.counter = -1;
                for (j = 0; j < workersA; j++)
                {
                    if (counter[j].counter > max.counter)
                        max = counter[j];
                }
                for (j = 0; j < workersA; j++)
                {
                    if (max.counter == counter[j].counter)
                    {
                        if (strcmp(max.filename, counter[j].filename) > 0)
                            max = counter[j];

                    }
                }

                printf("File with max count: %s, word appears %d times\n", max.filename, max.counter);
            }
            else if(!strcmp(command, "/mincount"))
            {
                for (j = 0; j < workersA; j++)
                {
                    kill(pid_arr[j], SIGUSR1);

                    if ((fd = open(fifoNames[j], O_WRONLY)) < 0)
                    {
                        perror("Cannot open FIFO for write");
                        return EXIT_FAILURE;
                    }
                    if ((num = write(fd, buffer, strlen(buffer) - 1)) < 0) //-1 to skip \n
                    {
                        perror("Cannot write FIFO");
                        return EXIT_FAILURE;
                    }
                    close(fd);
                }
                int flag;

                while(1)
                {
                    flag = 0;
                    for(j = 0; j < workersA; j++)
                    {
                        memset(fifo_name, '\0', sizeof(fifo_name));
                        getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));

                        if (access(fifo_name, F_OK) < 0)
                        {
                            flag = 1;
                            signal(SIGUSR1, cont);
                            pause(); //waiting for child signal...
                        }
                    }
                    if (!flag) break;
                }

                fileCounter *counter = calloc(workersA, sizeof(fileCounter));
                fileCounter min;
                int l = 0;

                for(j = 0; j < workersA; j++)
                {
                    memset(fifo_name, '\0', sizeof(fifo_name));
                    getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));

                    if ((fd = open(fifo_name, O_RDONLY)) < 0)
                    {
                        perror("Cannot open FIFO for read");
                        return EXIT_FAILURE;
                    }
                    do
                    {
                        if ((num = read(fd, buffer, sizeof(buffer))) < 0)
                        {
                            perror("Cannot read from FIFO");
                            return EXIT_FAILURE;
                        }
                    }
                    while (num > 0);

                    sscanf(buffer, "%s%d", counter[l].filename, &counter[l].counter);
                    l++;
                    close(fd);
                }

                min.counter = 10000;
                for (j = 0; j < workersA; j++)
                {
                    if ((counter[j].counter < min.counter) && (counter[j].counter >= 1))
                        min = counter[j];

                }
                if (min.counter == 10000)
                {
                       min.counter = 0;
                       strcpy(min.filename, "NULL");
                }
                for (j = 0; j < workersA; j++)
                {
                    if (min.counter == counter[j].counter)
                    {
                        if (strcmp(min.filename, counter[j].filename) > 0)
                            min = counter[j];

                    }
                }

                printf("File with min count: %s, word appears %d times\n", min.filename, min.counter);
            }
            else if(!strcmp(command, "/wc"))
            {
                for (j = 0; j < workersA; j++)
                {
                    kill(pid_arr[j], SIGUSR1);

                    if ((fd = open(fifoNames[j], O_WRONLY)) < 0)
                    {
                        perror("Cannot open FIFO for write");
                        return EXIT_FAILURE;
                    }
                    if ((num = write(fd, buffer, strlen(buffer) - 1)) < 0) //-1 to skip \n
                    {
                        perror("Cannot write FIFO");
                        return EXIT_FAILURE;
                    }
                    close(fd);
                }
                int flag;

                while(1)
                {
                    flag = 0;
                    for(j = 0; j < workersA; j++)
                    {
                        memset(fifo_name, '\0', sizeof(fifo_name));
                        getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));

                        if (access(fifo_name, F_OK) < 0)
                        {
                            flag = 1;
                            signal(SIGUSR1, cont);
                            pause(); //waiting for child signal...
                        }
                    }
                    if (!flag) break;
                }

                int bc, wc, lc, b_total, w_total, l_total;

                b_total = w_total = l_total = 0;

                for(j = 0; j < workersA; j++)
                {
                    memset(fifo_name, '\0', sizeof(fifo_name));
                    getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));

                    if ((fd = open(fifo_name, O_RDONLY)) < 0)
                    {
                        perror("Cannot open FIFO for read");
                        return EXIT_FAILURE;
                    }
                    do
                    {
                        if ((num = read(fd, buffer, sizeof(buffer))) < 0)
                        {
                            perror("Cannot read from FIFO");
                            return EXIT_FAILURE;
                        }
                    }
                    while (num > 0);

                    sscanf(buffer, "%d%d%d", &bc, &wc, &lc);

                    b_total += bc;
                    w_total += wc;
                    l_total += lc;

                    close(fd);
                }

                printf("Total bytes: %d Total words: %d Total lines: %d\n", b_total, w_total, l_total);
            }
            else if(!strcmp(command, "/exit"))
            {
                for (j = 0; j < workersNum; j++)
                {
                    kill(pid_arr[j], SIGUSR1);

                    if ((fd = open(fifoNames[j], O_WRONLY)) < 0)
                    {
                        perror("Cannot open FIFO for write");
                        return EXIT_FAILURE;
                    }
                    if ((num = write(fd, buffer, 5)) < 0)
                    {
                        perror("Cannot write FIFO");
                        return EXIT_FAILURE;
                    }
                    close(fd);
                }

            }
            else printf("Wrong input\n");

            total_n = 0;

        }

        while(wait(&status) > 0);
    }

    for (j = 0; j < workersNum; j++)
    {
        unlink(fifoNames[j]);
        free(fifoNames[j]);
    }

    for(j = 0; j < workersA; j++)
    {
        memset(fifo_name, '\0', sizeof(fifo_name));
        getFifoName(pid_arr[j], fifo_name, sizeof(fifo_name));
        unlink(fifo_name);
    }

    return 0;
}
