# ipc-named-pipes

IPC using named pipes and signals

The project has been implemented and tested in a Linux environment.

Two .c source files, a .h and makefile header file are contained.

The program consists of two main parts, first the part of the pre-processing in which jobExecutor reads the log of the paths given as input, and after creating the workers, it evenly distributes the directories so that each worker manages approximately the same number of directories. Each worker creates a Trie structure in memory which is used to directly access the files available to him, so he does not need to read the files from the disk every time, which is quite time consuming. In the second part of the program the workers receive queries from jobExecutor which they process and then send the results back to jobExecutor which in turn displays the results to the user.

Here's a brief description of the files ...

main.c: JobExecutor creates a certain number of workers and distributes the directories evenly using named pipes. The procedure followed is as follows: In the beginning, jobExecutor creates for each worker a named pipe by giving it a specific name, and then opens and writes to each named pipe the filepath of the directory following the appropriate allocation. Then each worker reads this information and stores the files that correspond to him as mentioned above. JobExecutor is informed of the number of active workers if the number of workers is greater than the number of directories they are asked to manage. Once the files have been processed, the workers are blocked by waiting for the appropriate signal from jobExecutor. Once the user has given a command through the stdin jobExecutor "awakens" the active workers and sends all the command given by the user. Workers, after processing properly, open from a new named pipe and send the results back to jobExecutor which displays them to the user. This process continues until the user has given the command / exit command which releases the structures used and records all the actions performed by the workers on a log file.

defs.c: Includes the implementation of functions used to create the trie, store files, query queries and other necessary processes required to work properly.

decs.h: Contains the statements of the structures and functions of the program along with the necessary libraries.

Make and make clean compilation command to delete the object file.

./jobExecutor -d <document> -w <number of workers>
