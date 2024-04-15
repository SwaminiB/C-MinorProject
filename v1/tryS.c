#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <sqlite3.h>


#define FILTER 200


struct employee
{
    int empID;
    char empName[50];
    float empSalary;
};



// callback function for displaying the data
static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int insertDB(int new_server_fd)
{ // Receive the number of structs
    int num_structs;

    if (recv(new_server_fd, &num_structs, sizeof(int), 0) < 0)
    {
        perror("Error in receiving the number of structs");
        exit(EXIT_FAILURE);
    }

    // Receive the structs
    struct employee received_emp[num_structs];

    if (recv(new_server_fd, received_emp, sizeof(struct employee) * num_structs, 0) < 0)
    {
        perror("Error in receiving the structs");
        exit(EXIT_FAILURE);
    }

    printf("Received %d strutures from client\n", num_structs);

    sqlite3 *db;
    char *err_msg = 0;
    int check;

    // Open database
    check = sqlite3_open("test.db", &db);
    if (check != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Create table
    const char *sql_create = "CREATE TABLE IF NOT EXISTS employee (id INTEGER PRIMARY KEY , name TEXT NOT NULL, salary REAL NOT NULL);";
    check = sqlite3_exec(db, sql_create, NULL, 0, &err_msg);
    if (check != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg); // deallocates the memory allocated for the error message using sqlite3_malloc() during the execution of sqlite3_exec().
    }

    // Insert data into the database
    for (int i = 0; i < num_structs; i++)
    {
        char sql_insert[200]; // Allocate space for the SQL statement
        sprintf(sql_insert, "INSERT INTO employee (id, name, salary) VALUES (%d, '%s', %.2f);",
                received_emp[i].empID, received_emp[i].empName, received_emp[i].empSalary);

        check = sqlite3_exec(db, sql_insert, NULL, 0, &err_msg);
        if (check != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }

    return 0;
}

int sortDB()
{ printf("Sorting values by salary\n");

sqlite3 *db;
    char *err_msg = 0;
    int check;

    // Open database
    check = sqlite3_open("test.db", &db);
    if (check != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Sort values by ID
    char *sort = "SELECT * FROM employee ORDER BY salary ";
    check = sqlite3_exec(db, sort, callback, 0, &err_msg);
    if (check != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    return 0;
}

int searchDB(int new_server_fd)
{

    sqlite3 *db;
    char *err_msg = 0;
    int check;

    // Open database
    check = sqlite3_open("test.db", &db);
    if (check != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    char filterDB[FILTER];
     if (recv(new_server_fd, &filterDB, sizeof(filterDB), 0) < 0)
    {
        perror("Error in receiving the number of structs");
        exit(EXIT_FAILURE);
    }

    printf("Received filter parameter: %s\n", filterDB);

    filterDB[strcspn(filterDB , "\n")] = 0 ;

    check = sqlite3_exec(db,filterDB,callback,0,&err_msg);

    if (check != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    return 0;
}

void displayDB()
{
    sqlite3 *db;
    char *err_msg = 0;
    int check;

    // Open database
    check = sqlite3_open("test.db", &db);
    if (check != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    }

    // Display inserted records
    char *display = "SELECT * FROM employee";
    check = sqlite3_exec(db, display, callback, 0, &err_msg);
    if (check != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf(stderr, "Port not provided. Program terminated");
        exit(1);
    }

    int server_fd, new_server_fd, portno;

    struct sockaddr_in serv_addr, cli_addr;
    // int addrlen = sizeof(serv_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0); // Created socketfd

    if (server_fd < 0)
    {
        perror("Socket Creation Failed.Error opening the socket");
        exit(EXIT_FAILURE);
    }

    // Add setsockopt to allow reusing the address
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt failed");
    }

    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Correct assignment for server IP address
    // memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero)); // Initialize remaining bytes to zero

    // Bind
    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error - Binding the socket");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 5) < 0)
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    // Accept new connections
    socklen_t cli_len = sizeof(cli_addr);

    new_server_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);

    if (new_server_fd < 0)
    {
        perror("Accepting failed");
        exit(EXIT_FAILURE);
    }

    int switchOpt = 0;

    if (recv(new_server_fd, &switchOpt, sizeof(int), 0) < 0)
    {
        perror("Error in receiving the number of structs");
        exit(EXIT_FAILURE);
    }

    printf("%d", switchOpt);

    switch (switchOpt)
    {
    case 1:
    {
        printf("You are in case 1 in switch case\n");
        insertDB(new_server_fd);
        displayDB();
        break;
    }

    case 2:
    {
        printf("You are in case 2 in switch case WHICH PERFORMS SORT BY SALARY\n");
        sortDB();

        break;
    }

    case 3:
    {
        printf("You are in case 3 in switch case which performs SEARCH\n");
        searchDB(new_server_fd);
        break;
    }

    default:
        break;
    }

    close(new_server_fd);
    close(server_fd);

    return 0;
}