# Linux_client_server_mirror

# System Programming

This repository contains different low-level codes implemented using C language utilizing System calls. 

## Project Objective

To learn a different aspect of Linux and low-level programming using a practical implementation of scripts that can be used to reduce repetitive tasks.

## ncpmvdir.c

When you want to bulk copy or move particular sets of files (in this case files that match particular sets of extensions) from the source directory to the destination directory, ncpmvdir.c is a useful program to do that.

This program can be used to copy or move a directory tree rooted at a specific path in the home directory to a specific destination directory in the home directory minus the file types specified in the extension list.


bash
  Usage: ncpmvdir [source_dir] [destination_dir] [options] <extension list>



## prcinfo.c

This program can be useful for searching processes in the process tree rooted at a specific process and outputs the requested information based on the input parameters.


bash
  Usage: prcinfo [root_process] [process_id1] ... [process_id5] [options]
  OPTIONS:
  -nd: lists the PIDs of all the non-direct descendants of process_id1
  -dd: lists the PIDs of all immediate descendants of process_id1
  -sb: lists the PIDs of all the sibling's processes of process_id1
  -sz: lists the PIDs of all siblings processes of process_id1 that are defunct
  -gc: lists the PIDs of all the grandchildren of process_id1
  -zz: prints the status of process_id1 (defunct or non-defunct)
  -zc: lists the PIDs of all the direct descendants of process_id1that are currently in defunct state



## deftreeminus.c

This program can be used for searching defunct processes in a process tree rooted at the specified process and forcefully terminates their parent processes based on the input parameters.

bash
  Usage: deftreeminus [option1] [option2] [-processid]
  OPTION1:
    -t: forcefully terminates parent processes(whose elapsed time is greater than PROC_ELTIME) of all the defunct processes in the process tree rooted at root_process
    -b: forcefully terminates all the processes in the process tree rooted at root_process that have >= NO_OF_DFCS, defunct child
  OPTION2:
    PROC_ELTIME: The elapsed time of the process in the minutes since it was created
    NO_OF_DFCS: The number of default child processes


## mshell.c

This is the custom terminal implementation that goes into an infinite waiting loop waiting for the user's commands. It supports operators like piping, redirection, conditional execution, background processing, and sequential execution.

bash
  Example: mshell$ ls -l ~/home -S -n | wc | wc -w


## backup.sh

This program creates a complete backup of all .txt files at directory ~/ by tarring files into cb***.tar stored at ~/home/backup/cb every 2 minutes.

bash
  Usage: ./backup.sh

## client.c, server.c and mirror.c

This is a client-server project and can be load balanced with a mirror.
The client can request a file or set of files from the server. The server searches for the files in its root directory and returns the tar.gz file. Multiple clients can connect to the server from different machines and can request files. 

The first 6 client requests will be handled by the server and the next 6 client requests will be handled by mirror. The remaining client connection will alternate between server and mirror wherein the first client connection will be handled by the server.

bash
    List of client commands:

    1) fgets file1 file2 ... file4
    - Server will search for file1...file4 and return temp.tar.gz if any file found else "No file found"

    2) tarfgetz size1 size2 <-u>
    - Server will return temp.tar.gz of files that of size1<=size<=size2 and -u will unzip temp.tar.gz at client size

    3) filesrch filename
    - Server will find the filename and returns the filename, file size, and date created

    4) targzf <extension_list> <-u>
    - Server will return temp.tar.gz that contains all the files belonging to file types <extension_list>

    5) getdirf date1 date2 <-u>
    - Server will return temp.tar.gz of files that of date1<=date<=date2 and -u will unzip temp.tar.gz at client size
