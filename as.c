#include <stdio.h>
#include <netinet/in.h> //sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <sqlite3.h>
#include <jansson.h>

#define FILTER 200

struct emp
{
    int ID;
    char Name[50];
    float Salary;
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
{
    char buffer[1024] = {0};
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
        sqlite3_free(err_msg); // deallocates the memory allocated for the error message
    }

    int numEmployees = 0;

    if (recv(new_server_fd, &numEmployees, sizeof(int), 0) < 0)
    {
        perror("Error while receiving the numEmployees from server");
        return -1;
    }
    printf("The number of array elements is : %d\n", numEmployees);

    if (recv(new_server_fd, &buffer, sizeof(buffer), 0) < 0)
    {
        perror("Error in receiving the json data");
        exit(EXIT_FAILURE);
    }

    printf("Received from client: %s\n", buffer);

    // Parse JSON
    json_error_t error;
    json_t *root = json_loads(buffer, 0, &error);

    if (!root)
    {
        fprintf(stderr, "Error while creating JSON root");
        exit(EXIT_FAILURE);
    }

    // Extract data array
    json_t *json_obj_array = json_object_get(root, "data");
    // size_t num_objects = json_array_size(json_obj_array);

    struct emp employees[100];

    for (int i = 0; i < numEmployees; i++)
    {
        // extract ek ek JSON object from array
        json_t *obj = json_array_get(json_obj_array, i);
        if (!json_is_object(obj))
        {
            fprintf(stderr, "error: Expected object in JSON array.\n");
            exit(EXIT_FAILURE);
            json_decref(root);
            sqlite3_close(db);
            exit(EXIT_FAILURE);
        }

        // Get fields from the object
        json_t *name = json_object_get(obj, "name");
        json_t *ID = json_object_get(obj, "ID");
        json_t *salary = json_object_get(obj, "salary");

        if ((name && !json_is_string(name)) ||
            (ID && !json_is_integer(ID)) ||
            (salary && !json_is_real(salary)))
        {
            fprintf(stderr, "Invalid JSON format in object\n");
            exit(EXIT_FAILURE);
        }

        // JSON se normal variables mai lenge
        const char *empName = json_string_value(name);
        int empID = json_integer_value(ID);
        double empSalary = json_real_value(salary);

        // normal variables ko struct mai store karege
        strncpy(employees[i].Name, empName, sizeof(employees[i].Name) - 1);
        employees[i].Name[sizeof(employees[i].Name) - 1] = '\0'; // Ensure null-termination
        employees[i].ID = empID;
        employees[i].Salary = empSalary;

        // Insert data into the database
        char sql_insert[200]; // Allocate space for the SQL statement
        sprintf(sql_insert, "INSERT INTO employee (id, name, salary) VALUES (%d, '%s', %.2f);",
                employees[i].ID, employees[i].Name, employees[i].Salary);

        check = sqlite3_exec(db, sql_insert, NULL, 0, &err_msg);
        if (check != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            json_decref(root);
            sqlite3_close(db);
            exit(EXIT_FAILURE);
        }
    }

    // Populating values in struct array
    sqlite3_stmt *stmt;
    int ret;
    char *sql_stmt = "SELECT * FROM employee";

    ret = sqlite3_prepare_v2(db, sql_stmt, -1, &stmt, 0);
    if (ret != SQLITE_OK)
    {
        printf("\nUnable to fetch data");
        sqlite3_close(db);
        return 1;
    }

    int num = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Fetch and store data
        employees[num].ID = sqlite3_column_int(stmt, 0);
        const char *name = sqlite3_column_text(stmt, 1);
        snprintf(employees[num].Name, sizeof(employees[num].Name), "%s", name);
        employees[num].Salary = sqlite3_column_double(stmt, 2);
        num++;
    }

    sqlite3_finalize(stmt);

    printf("-----Employee records in the array-----\n");
    for (size_t i = 0; i < 15; i++)
    {
        printf("Name: %s\n", employees[i].Name);
        printf("ID: %d\n", employees[i].ID);
        printf("Salary: %.2f\n\n", employees[i].Salary);
    }
    json_decref(root);
    sqlite3_close(db);
    return 0;
}

int sortDB(int new_server_fd)
{
    char sortDBParameter[10];

    if (recv(new_server_fd, sortDBParameter, sizeof(char) * 10, 0) < 0)
    {
        perror("Error in receiving the sorting parameter");
        exit(EXIT_FAILURE);
    }

    if (strcmp(sortDBParameter, "ID") != 0 && strcmp(sortDBParameter, "salary") != 0 && strcmp(sortDBParameter, "name") != 0)
    {
        printf("Invalid choice. Please choose from ID, salary, or name.\n");

        return 0;
    }

    printf("-----Printing Sorted Values from the Database based on the values in the %s column-----\n", sortDBParameter);

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

    char sortQuery[100]; // Allocate space for the SQL statement

    sprintf(sortQuery, "SELECT * FROM employee ORDER BY %s;", sortDBParameter);

    sortQuery[strcspn(sortQuery, "\n")] = 0;

    check = sqlite3_exec(db, sortQuery, callback, 0, &err_msg);

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

    char colName[100] = {0};
    char optr[3] = {0};
    char value[100] = {0};

    // Now recv the column name
    if (recv(new_server_fd, colName, sizeof(colName), 0) < 0)
    {
        perror("Error in receiving the column name");
        exit(EXIT_FAILURE);
    }
    printf("String : %s \n", colName);

    // Now recv the opertor
    if (recv(new_server_fd, optr, sizeof(optr), 0) < 0)
    {
        perror("Error in sending the operator");
        exit(EXIT_FAILURE);
    }
    printf("Operator : %s\n", optr);

    // Now receive the value
    if (recv(new_server_fd, value, sizeof(value), 0) < 0)
    {
        perror("Error in receiving the value");
        exit(EXIT_FAILURE);
    }
    printf("Value : %s\n", value);
    value[strcspn(value, "\n")] = '\0'; // Remove the newline character if present

    char sortQuery[200]; // Allocate space for the SQL statement

    sprintf(sortQuery, "SELECT * FROM employee WHERE %s %s %s;", colName, optr, value);

    sortQuery[strcspn(sortQuery, "\n")] = 0;

    printf("The sort query is : %s\n", sortQuery);

     printf("-----Printing Searched Values from the Database based on the values provided-----\n");

    check = sqlite3_exec(db, sortQuery, callback, 0, &err_msg);

    int callback_called = 0; // Flag to indicate if callback function was called

    if (check == SQLITE_OK)
    {
        if (callback_called == 0)
        {
            printf("The above record is found in the db.\nIf it is blank, it means no record found according to the inputs provided.\n");
            return 0;
        }
    }
    else
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    return 0;
}

// if (check != SQLITE_OK)
// {
//     fprintf(stderr, "SQL error: %s\n", err_msg);
//     sqlite3_free(err_msg);
// }

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
    printf("-----Printing Values from the Database-----\n");
    char *display = "SELECT * FROM employee";
    check = sqlite3_exec(db, display, callback, 0, &err_msg);
    if (check != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

int main(int argc, char *argv[]) // total no of parameters and port no.
                                 // Argv contains original command line arguments. argv[1] - port no ; [0] - filename
{

    if (argc < 2)
    {
        fprintf(stderr, "Port not provided. Program terminated\n");
        exit(1);
    }

    int server_fd, new_server_fd, portno; // file descriptor

    struct sockaddr_in serv_addr, cli_addr; // addr_in gives internet address
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

    portno = atoi(argv[1]); //
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);     // host to network short
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Correct assignment for server IP address
    // memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero)); // Initialize remaining bytes to zero

    // Bind
    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error - Binding the socket");
        exit(EXIT_FAILURE);
    }
    // we need to typecast sockaddr_in to sock addr

    // Listen
    if (listen(server_fd, 5) < 0) // max no of client that can connect
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    // Accept new connections
    socklen_t cli_len = sizeof(cli_addr); // socklen_t is a 32 bit data type

    new_server_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);

    if (new_server_fd < 0)
    {
        perror("Accepting failed");
        exit(EXIT_FAILURE);
    }

P:
    int switchOpt = 0;

    if (recv(new_server_fd, &switchOpt, sizeof(int), 0) < 0)
    {
        perror("Error in receiving the number of structs");
        exit(EXIT_FAILURE);
    }

    switch (switchOpt)
    {
    case 1:
    {
        insertDB(new_server_fd);
        displayDB();
        break;
    }

    case 2:
    {
        sortDB(new_server_fd);

        break;
    }

    case 3:
    {
        searchDB(new_server_fd);
        break;
    }

    default:
        break;
    }

    char choice;

    if (recv(new_server_fd, &choice, sizeof(int), 0) < 0)
    {
        perror("Error in receiving the choice");
        exit(EXIT_FAILURE);
    }

    if (choice == 'Y' || choice == 'y')
    {
        goto P;
    }
    else
    {
        printf("Exiting Program");
        close(new_server_fd);
        close(server_fd);
    }
    return 0;
}