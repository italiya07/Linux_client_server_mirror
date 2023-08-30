// Drashti Italiya (110100401) Section-2
// Harshil Lukhi (110100748) Section-2

#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define PORT 8081
#define MAX_CLIENTS 100
#define MIRROR_SERVER_PORT 8081
#define REDIRECT_CODE 301
#define REDIRECT_MESSAGE "Moved Permanently"
#define MIRROR_SERVER_URL "http://127.0.0.1:8081"
#define BUFFER_SIZE 2048
#define MAX_ARGS 6
#define FGETS "fgets"
#define TARFGETZ "tarfgetz"
#define FILESRCH "filesrch"
#define TARGZF "targzf"
#define GETDIRF "getdirf"
#define SPACE_STRING " "
#define ROOT_DIR "/"
#define TEMP_DIR_ALIAS "/tmp/client_temp_directory_XXXXXX"
#define UNZIP_FLAG "-u"
#define EXIT_FLAG "quit"
#define FILE_FLAG "file"
#define MSG_FLAG "msg"
#define DONE_FLAG "done"
#define UNZIP_FILE_FLAG "unzip"
#define ZIP_FILE_FLAG "zip"
#define FILE_NOT_FOUND_FLAG "No file found"

bool createChild(void (*fn)(char *[], int, int, bool), char *commands[], int numOfCommands, int clientSocket, bool unZipFlag)
{
    pid_t processId = fork();
    if (processId == 0)
    {
        fn(commands, numOfCommands, clientSocket, unZipFlag);
    }
    else if (processId > 0)
    {
        // Parent process
        int status;
        waitpid(processId, &status, 0);

        if (WIFEXITED(status))
        {
            int exitStatus = WEXITSTATUS(status);
            if (exitStatus == 0)
            {
                return true; // Child process exited successfully
            }
            else
            {
                return false; // Child process encountered an error
            }
        }
        else
        {
            // Child process did not exit normally
            perror("Child process did not exit normally");
            return false;
        }
    }
    else
    {
        perror("Error while create fork");
        exit(EXIT_FAILURE);
    }
    return false;
}

int splitArguments(char *commandString, char *seperator, char *commands[])
{
    char *copyOfCommandString = strdup(commandString);
    if (copyOfCommandString == NULL)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    int numOfCommands = 0;
    char *endOfToken;
    char *token = strtok_r(copyOfCommandString, seperator, &endOfToken);

    while (token != NULL)
    {
        commands[numOfCommands++] = strdup(token);
        token = strtok_r(NULL, seperator, &endOfToken);
    }

    commands[numOfCommands] = NULL;

    free(copyOfCommandString);

    return numOfCommands;
}

bool isUnZip(char *commandString)
{
    if (strstr(commandString, UNZIP_FLAG) != NULL)
    {
        return true;
    }
    return false;
}

// Deletes temp dir
void removeTempDirectory(char *fileName)
{
    char deleteCommand[256];
    sprintf(deleteCommand, "rm -r %s", fileName);
    system(deleteCommand);
}

// send message of file or message flag to client to let client know that new chunk of data if file or message

void sendSocketMessage(char *message, int clientSocket)
{
    char buffer[BUFFER_SIZE];
    send(clientSocket, message, strlen(message), 0);
    recv(clientSocket, buffer, sizeof(buffer), 0);
}

void sendFile(char *fileName, int clientSocket, bool unZipFlag)
{
    int sentSize = 0;

    // Declare char arrays to hold file data and socket communication
    char fileBuffer[BUFFER_SIZE], buffer[BUFFER_SIZE];
    sendSocketMessage(FILE_FLAG, clientSocket);

    // Open file for reading in binary mode
    FILE *requested_file = fopen(fileName, "rb");
    if (!requested_file)
    {
        perror("Error opening file");
        return;
    }

    // Read and send the file data in chunks
    ssize_t bytesRead;
    while ((bytesRead = fread(fileBuffer, 1, sizeof(fileBuffer), requested_file)) > 0)
    {
        // Send the current chunk of file data
        sentSize = send(clientSocket, fileBuffer, bytesRead, 0);
        // fprintf(perror, "Bytes sent: %s\n", sentSize);
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    }

    fclose(requested_file);

    // send DONE_FLAG to let client know that chunks of file sending is done
    sendSocketMessage(DONE_FLAG, clientSocket);
    // To let client know that whetehr it has to unzip the file or not
    if (unZipFlag)
    {
        sendSocketMessage(UNZIP_FILE_FLAG, clientSocket);
    }
    else
    {
        sendSocketMessage(ZIP_FILE_FLAG, clientSocket);
    }
}

int isDirectoryEmpty(const char *directoryPath)
{
    DIR *givenDirectory = opendir(directoryPath);

    if (givenDirectory == NULL)
    {
        perror("Error opening directory");
        return false; // Indicate an error
    }

    struct dirent *directoryEntry;
    int isEmpty = true; // Assume directory is empty

    while ((directoryEntry = readdir(givenDirectory)) != NULL)
    {
        if (strcmp(directoryEntry->d_name, ".") != 0 && strcmp(directoryEntry->d_name, "..") != 0)
        {
            isEmpty = false; // Directory is not empty
            break;
        }
    }

    closedir(givenDirectory);

    return isEmpty;
}

char **createTempDirectories()
{
    char tempDir[] = TEMP_DIR_ALIAS;
    // Allocate memory for array of string pointers
    char **tempDirectories = (char **)malloc(2 * sizeof(char *)); // Allocate memory for array of string pointers
    if (mkdtemp(tempDir) == NULL)
    {
        perror("Error occured while creating temporary directory");
        exit(EXIT_FAILURE);
    }
    // Allocate memory and copy the main temp dir path
    tempDirectories[0] = (char *)malloc((strlen(tempDir) + 1) * sizeof(char));
    strcpy(tempDirectories[0], tempDir);

    // Create a subdirectory named 'content' withinmain temp dir
    char contentDir[1024];
    snprintf(contentDir, sizeof(contentDir), "%s/content", tempDir);

    if (mkdir(contentDir, 0700) == -1)
    {
        perror("Error occured while creating temporary directory");
        exit(EXIT_FAILURE);
    }

    // Allocate memory and copy the path of the 'content' subdir
    tempDirectories[1] = (char *)malloc((strlen(contentDir) + 1) * sizeof(char));
    strcpy(tempDirectories[1], contentDir);
    return tempDirectories;
}

void compressFiles(char **tempDirectories, int clientSocket, bool unZipFlag)
{
    int status = 0;

    if (isDirectoryEmpty(tempDirectories[1]))
    {
        //If empty, send message flag and a file not found message to the client
        sendSocketMessage(MSG_FLAG, clientSocket);
        sendSocketMessage(FILE_NOT_FOUND_FLAG, clientSocket);
    }
    else
    {
        char compressCommand[1024], filePath[1024];

        //Createsfile path for the compressed archive (temp.tar.gz)
        sprintf(filePath, "%s/temp.tar.gz", tempDirectories[0]);
        // Constructing compression command using 'tar' utility
        // czf creates new archive, compress using gzip, specify name of archive file
        sprintf(compressCommand, "tar czf %s %s --transform 's/.*\\///'", filePath, tempDirectories[1]);

        status = system(compressCommand);

        if (status != 0)
        {
            perror("Error while compressing files");
        }
        // call the functions
        sendFile(filePath, clientSocket, unZipFlag);
        removeTempDirectory(tempDirectories[0]);
    }
}

// Search for the files and store them in tempdir & compress it
void runFgets(char *commands[], int numOfCommands, int clientSocket, bool unZipFlag)
{
    char **tempDirectories = createTempDirectories();

    int status = 0;

    for (int command = 1; command < numOfCommands; command++)
    {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "find ~/ -name %s -type f -exec cp {} %s \\; -quit", commands[command], tempDirectories[1]);

        // Executes the command
        status = system(cmd);
        if (status != 0)
        {
            fprintf(stderr, "Error searching for %s\n", commands[command]);
        }
    }
    compressFiles(tempDirectories, clientSocket, unZipFlag);
}

// Creates temp.tar.gz file that contains files with file-size in bytes is >=size1 and <=size2
// find ~/ -type f -size +size1c -a -size -size2c -exec tar czvf filename.tar.gz {} +
void runTarfGetz(char *commands[], int numOfCommands, int clientSocket, bool unZipFlag)
{
    char **tempDirectories = createTempDirectories();
    int status = 0;

    char command[1024];
    sprintf(command, "find ~/ -type f -size +%dc -a -size -%dc -exec cp {} %s \\;", atoi(commands[1]), atoi(commands[2]), tempDirectories[1]);
    status = system(command);
    if (status != 0)
    {
        perror("Error while searching for files");
    }

    compressFiles(tempDirectories, clientSocket, unZipFlag);
}

// Return the filename, size(in bytes), and date created
// filename="your_filename"; result=$(find ~/ -type f -name "$filename" | head -n 1); if [ -n "$result" ]; then stat -c "Filename: %n\nSize: %s bytes\nDate Created: %w" "$result"; else echo "File not found"; fi
void runFileSrch(char *commands[], int numOfCommands, int clientSocket, bool unZipFlag)
{
    char command[BUFFER_SIZE];
    sprintf(command, "file_path=$(find ~/ -type f -name \"%s\" -print -quit); [ -n \"$file_path\" ] && stat -c \"File Name:%%n | File size:%%s bytes | Date:%%y\" \"$file_path\" || echo \"No file found\"", commands[1]);
    FILE *fp = popen(command, "r"); // Open the command for reading
    if (fp == NULL)
    {
        perror("Error while finding file");
    }

    char buffer[BUFFER_SIZE];
    // Read chars from file pointed by fp into buffer
    fgets(buffer, sizeof(buffer), fp);
    sendSocketMessage(MSG_FLAG, clientSocket);
    sendSocketMessage(buffer, clientSocket);

    pclose(fp);
}

// Returns temp.tar.gz that contains files with file type listed in the extension list
void runTargzf(char *commands[], int numOfCommands, int clientSocket, bool unZipFlag)
{
    char **tempDirectories = createTempDirectories();

    for (int i = 1; i < numOfCommands; i++)
    {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "find ~/ -type f -name \"*.%s\" -exec cp {} %s \\;", commands[i], tempDirectories[1]);

        int status = system(cmd);
        if (status != 0)
        {
            fprintf(stderr, "Error searching for %s\n", commands[i]);
        }
    }

    compressFiles(tempDirectories, clientSocket, unZipFlag);
}

//Returns temp.tar.gz that contains files whose date of creation is <=date2 and >=date1 
void runGetDirf(char *commands[], int numOfCommands, int clientSocket, bool unZipFlag)
{
    char **tempDirectories = createTempDirectories();
    int status = 0;

    char command[1024];
    sprintf(command, "find ~/ -type f -newermt \"%s 00:00:00\" ! -newermt \"%s 23:59:59\" -exec cp {} %s \\;", commands[1], commands[2], tempDirectories[1]);
    // strcat(command, " -exec tar czvf temp.tar.gz --transform 's/.*\\///' {} +");
    status = system(command);
    if (status != 0)
    {
        perror("Error while searching for files");
    }

    compressFiles(tempDirectories, clientSocket, unZipFlag);
}

void checkAndExecuteRules(char *commandString, int clientSocket)
{
    char *commands[MAX_ARGS];
    int numOfCommands = splitArguments(commandString, SPACE_STRING, commands);
    // check whether recieved command string has UNZIP_FLAG or not
    bool unZipFlag = isUnZip(commandString);

    if (strcmp(commands[0], FGETS) == 0)
    {
        createChild(runFgets, commands, numOfCommands, clientSocket, unZipFlag);
    }
    else if (strcmp(commands[0], TARFGETZ) == 0)
    {
        createChild(runTarfGetz, commands, numOfCommands, clientSocket, unZipFlag);
    }
    else if (strcmp(commands[0], FILESRCH) == 0)
    {
        createChild(runFileSrch, commands, numOfCommands, clientSocket, unZipFlag);
    }
    else if (strcmp(commands[0], TARGZF) == 0)
    {
        createChild(runTargzf, commands, numOfCommands, clientSocket, unZipFlag);
    }
    else if (strcmp(commands[0], GETDIRF) == 0)
    {
        createChild(runGetDirf, commands, numOfCommands, clientSocket, unZipFlag);
    }
}

void handleClient(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    while (1)
    {
        // Recieved command from client into buffer
        if (recv(clientSocket, buffer, sizeof(buffer), 0))
        {
            if (strcmp(buffer, EXIT_FLAG) == 0)
            {
                return;
            }
            else
            {
                checkAndExecuteRules(buffer, clientSocket);
                memset(buffer, 0, sizeof(buffer));
            }
        }
    }
}

void redirectToMirror(int clientSocket)
{
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s\r\n"
             "Location: %s\r\n\r\n",
             REDIRECT_CODE, REDIRECT_MESSAGE, MIRROR_SERVER_URL);

    send(clientSocket, response, strlen(response), 0);
}

int main()
{
    int serverSocket, clientSocket, clientConnections = 0;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("Socket intialization falied");
        exit(EXIT_FAILURE);
    }

    // Bind
    // Clears the serverAddress structure
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Server Socket binding failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Listen puts the socket in a listening state to accept incoming connections
    if (listen(serverSocket, MAX_CLIENTS) == -1)
    {
        perror("Server Socket listing falied");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server is on port %d...\n", PORT);

    // Accept and handle client connections
    while (1)
    {
        // holds the socket descriptor for the accepted client connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            perror("Client accept error");
            exit(1);
        }
        pid_t child_pid;
        if ((child_pid = fork()) == 0)
        {
            close(serverSocket);
            handleClient(clientSocket);
        }
    }
    close(clientSocket);
    return 0;
}