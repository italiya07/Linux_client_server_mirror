// Drashti Italiya (110100401) Section-2
// Harshil Lukhi (110100748) Section-2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define SERVER_IP "127.0.0.1"
#define MIRROR_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MIRROR_PORT 8081
#define BUFFER_SIZE 2048
#define ACK_STR "Recieved"
#define FILE_NAME "temp.tar.gz"
#define MAX_ARGS 6
#define SPACE_STRING " "
#define FGETS "fgets"
#define TARFGETZ "tarfgetz"
#define FILESRCH "filesrch"
#define TARGZF "targzf"
#define GETDIRF "getdirf"
#define UNZIP_FLAG "-u"
#define EXIT_FLAG "quit"
#define FILE_FLAG "file"
#define MSG_FLAG "msg"
#define DONE_FLAG "done"
#define UNZIP_FILE_FLAG "unzip"
#define ZIP_FILE_FLAG "zip"
#define FILE_NOT_FOUND_FLAG "No file found"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"
#define MOVED_FLAG "HTTP/1.1 301 Moved Permanently"

bool isValidDate(char *dateString)
{
    int year, month, day;
    // sscanf to parse the string into year, month, and day integers
    if (sscanf(dateString, "%d-%d-%d", &year, &month, &day) == 3)
    {
        if (year >= 1000 && year <= 9999 && month >= 1 && month <= 12 && day >= 1 && day <= 31)
        {
            return true;
        }
    }
    return false;
}

int splitArguments(char *commandString, char *seperator, char *commands[])
{
    char *copyOfCommandString = strdup(commandString);
    if (copyOfCommandString == NULL)
    {
        perror("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    int numOfCommands = 0;
    char *endOfToken;
    // Tokenizes the copied command string using the separator
    // Returns the first token & updates endOfToken to point to the remaining portion of the string
    char *token = strtok_r(copyOfCommandString, seperator, &endOfToken);

    while (token != NULL)
    {
        commands[numOfCommands++] = strdup(token);
        token = strtok_r(NULL, seperator, &endOfToken);
    }

    // Add NULL to indicate the end of arguments
    commands[numOfCommands] = NULL;

    free(copyOfCommandString);

    return numOfCommands;
}

bool validateCommandString(char *commandString)
{
    char *commands[MAX_ARGS];
    int numOfCommands = splitArguments(commandString, SPACE_STRING, commands);
    // Array will hold the individual arguments extracted from the command string

    // Checks if the first arg matches the string constant FGETS
    if (strcmp(commands[0], FGETS) == 0)
    {
        if (numOfCommands > 5 || numOfCommands < 2)
        {
            printf("Usage: fgets file1 file2 file3 file4\n");
            return false;
        }
        return true;
    }
    else if (strcmp(commands[0], TARFGETZ) == 0)
    {
        if (numOfCommands > 4 || (numOfCommands < 4 && strcmp(commands[numOfCommands - 1], UNZIP_FLAG) == 0))
        {
            printf("Usage: tarfgetz size1 size2 <-u>\n");
            return false;
        }
        else if (atoi(commands[0]) && atoi(commands[1]))
        {
            printf("Usage: tarfgetz size1 size2 <-u> [size1 and size2 must be number]\n");
            return false;
        }
        return true;
    }
    else if (strcmp(commands[0], FILESRCH) == 0)
    {
        if (numOfCommands != 2)
        {
            printf("Usage: filesrch filename\n");
            return false;
        }
        return true;
    }
    else if (strcmp(commands[0], TARGZF) == 0)
    {
        if (numOfCommands > 5 || numOfCommands < 2 || (numOfCommands == 2 && strcmp(commands[numOfCommands - 1], UNZIP_FLAG) == 0))
        {
            printf("Usage: targzf <extension list [up to 4 extensions]> <-u>\n");
            return false;
        }

        return true;
    }
    else if (strcmp(commands[0], GETDIRF) == 0)
    {
        if (numOfCommands > 4 || (numOfCommands < 4 && strcmp(commands[numOfCommands - 1], UNZIP_FLAG) == 0))
        {
            printf("Usage: getdirf date1 date2 <-u>\n");
            return false;
        }

        // Check for the valid date
        else if (isValidDate(commands[0]) && isValidDate(commands[1]))
        {
            printf("Usage: getdirf date1 date2 <-u> [date1 and date2 must be valid date]\n");
            return false;
        }
        return true;
    }
    printf("Invalid Command\n");
    return false;
}

void receiveFile(int clientSocket)
{
    // Opens a file named temp.tar.gz in binary write mode
    FILE *receivedFile = fopen(FILE_NAME, "wb");
    if (!receivedFile)
    {
        perror("Error opening file\n");
    }
    char buffer[BUFFER_SIZE];

    ssize_t bytesReceived;
    char tempStr[BUFFER_SIZE];
    //repeatedly receives data from the server/mirror and writes to opened file
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, 1, bytesReceived, receivedFile);

        // If recieves done flag, it breaks & send ack to server
        if (strcmp(buffer,DONE_FLAG) == 0)
        {
            send(clientSocket, ACK_STR, strlen(ACK_STR), 0);
            break;
        }
        memset(buffer, 0, sizeof(buffer));
        //sends ack back toserver/mirror after each chunk of data received
        send(clientSocket, ACK_STR, strlen(ACK_STR), 0);
    }

    fclose(receivedFile);

    memset(buffer, 0, sizeof(buffer));
    // Receives response from the server in buffer
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    // Send ack back to server
    send(clientSocket, ACK_STR, strlen(ACK_STR), 0);

    if (strcmp(buffer, UNZIP_FILE_FLAG) == 0)
    {
        // tar -xvzf temp.tar.gz &>/dev/null
        char unZipCommand[BUFFER_SIZE];
        // snprintf to format str that represents the command 
        snprintf(unZipCommand, sizeof(unZipCommand), "tar -xvzf %s &>/dev/null", FILE_NAME);
        //  to execute the command stored in the unZipCommand array
        int status = system(unZipCommand);
        if (status != 0)
        {
            perror("Error while unzipping tar.gz\n");
        }
    }
}

void receiveReponse(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    // Receives response from the server in buffer
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
     // Send ack back to server
    send(clientSocket, ACK_STR, strlen(ACK_STR), 0);

    //If the received data indicates MSG_FLAG, 
    //the function receives the actual message, prints it,sends ack
    if (strcmp(buffer, MSG_FLAG) == 0)
    {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("%s\n", buffer);
        send(clientSocket, ACK_STR, strlen(ACK_STR), 0);
    }
    else if (strcmp(buffer, FILE_FLAG) == 0)
    {
        memset(buffer, 0, sizeof(buffer));
        receiveFile(clientSocket);
    }
}

int main()
{
    // It creates a socket, defines the server's address, and connects to the server using the socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        perror("Error creating socket\n");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Error connecting to server\n");
        close(clientSocket);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    // Reserves initial message from server into buffer
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    // If recved message containes MOVED_FLAG that means server is redirecting client to mirror
    if (strstr(buffer, MOVED_FLAG) != NULL)
    {
        printf("Redirecting to mirror\n");
        // Close connection from the original server 
        close(clientSocket);
        // Create new socket and create connection to the mirror
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1)
        {
            perror("Error creating socket\n");
            return 1;
        }
        struct sockaddr_in mirrorAddress;
        mirrorAddress.sin_family = AF_INET;
        mirrorAddress.sin_port = htons(MIRROR_PORT);
        mirrorAddress.sin_addr.s_addr = inet_addr(MIRROR_IP);

        if (connect(clientSocket, (struct sockaddr *)&mirrorAddress, sizeof(mirrorAddress)) == -1)
        {
            perror("Error connecting to mirror");
            close(clientSocket);
            return 1;
        }
    }

    while (1)
    {
        char commandString[BUFFER_SIZE];
        printf(COLOR_GREEN "C$ " COLOR_RESET);
        // Reads the stdin and store it in string array buffer
        fgets(commandString, sizeof(commandString), stdin);
        // Remove newline
        commandString[strlen(commandString) - 1] = '\0'; 

        if (strcmp(commandString, EXIT_FLAG) == 0)
        {
            printf("Exiting...\n");
            // Send exit flag message as data to the server through socket
            send(clientSocket, EXIT_FLAG, strlen(EXIT_FLAG), 0);
            break;
        }

        // Validate the user's command
        if (!validateCommandString(commandString))
        {
            continue;
        }

        //Send the validated command to the server/mirror
        send(clientSocket, commandString, strlen(commandString), 0);
        // Receive and process the response from the server/mirror
        receiveReponse(clientSocket);
    }

    // Close the client socket and exit the program
    close(clientSocket);
    return 0;
}
