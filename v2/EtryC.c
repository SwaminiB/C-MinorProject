#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sqlite3.h>

#define FILTER 200

struct employee
{
    int empID;
    char empName[50];
    float empSalary;
};

// void insertEmployee{

//     char *mystring;
// mystring = (char *)malloc(sizeof(char) * (size + 1));
// }

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(1);
    }

    int portno = atoi(argv[2]);

    struct hostent *server;
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host");
        exit(1);
    }

    // Socket Creation
    int sock = 0;
    sock = socket(AF_INET, SOCK_STREAM, 0); // Created socketfd

    if (sock < 0)
    {
        perror("Error opening the socket");
        exit(EXIT_FAILURE);
    }
    // alt way
    //  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    //      error("Socket Creation Error\n");
    //      return -1;
    //  }

    struct sockaddr_in serv_addr;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    // bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    // Connect to Server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    int option;
    printf("Choose an option\n1.Insert\n2.Sort\n3.Search\n0.Exit");
    scanf("%d", &option);

    if (send(sock, &option, sizeof(int), 0) < 0)
    {
        perror("Error in sending the option of switch case");
        exit(EXIT_FAILURE);
    }

    switch (option)
    {

    case 1:
    {
        printf("you are inside the case 1 in switch case which performs Insert ");
        int num_structs;
        printf("Enter the number of employees");
        scanf("%d", &num_structs);

        // Abhi usko array of structs mai lena hai
        struct employee toSendEmp[num_structs];

        for (int i = 0; i < num_structs; i++)
        {
            printf("Enter data for struct\n");

            printf("Enter emp id : ");
            scanf("%d", &toSendEmp[i].empID);

            printf("Enter emp name : ");
            scanf("%s", toSendEmp[i].empName);

            printf("Enter emp salary : ");
            scanf("%f", &toSendEmp[i].empSalary);
        }

        // now send them to server
        // number of structs send karege pehle
        if (send(sock, &num_structs, sizeof(int), 0) < 0)
        {
            perror("Error in sending the number of structs");
            exit(EXIT_FAILURE);
        }

        // now sending the structs
        if (send(sock, toSendEmp, sizeof(struct employee) * num_structs, 0) < 0)
        {
            perror("Error in sending the structs");
            exit(EXIT_FAILURE);
        }
        break;
        
    }

    case 2:
    {
        // printf("you are inside the case 2 in switch case which performs SORT BY SALARY \n");to 

        char  sortChoice[10];
        printf("Choose how you want to sort the db\nID\nsalary\nname\n");
        scanf(" %[^\n]", sortChoice);

        if (send(sock, sortChoice, strlen(sortChoice) + 1, 0) < 0)
        {
            perror("Error in sending the filter parameter");
            exit(EXIT_FAILURE);
        }



        break;
    }

    case 3:
    {
        printf("you are inside the case 3 in switch case which performs SEARCH\n");
        char filterDB[FILTER];

        printf("Enter the command to filter: \n");
        // Use %[^\n] to read until newline character
        scanf(" %[^\n]", filterDB);

        // Now send the filter parameter
        if (send(sock, filterDB, strlen(filterDB) + 1, 0) < 0)
        {
            perror("Error in sending the filter parameter");
            exit(EXIT_FAILURE);
        }

        printf("SENT");
        break;
    }

    case 0:
    {
        printf("Value of option in case 0 is %d\n", option);
        printf("Exiting the program\n");
        close(sock);
        break;
    }
    }

    return 0;
}
