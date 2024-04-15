#include <netdb.h> //defines the hostent structure

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <unistd.h>

#include <sqlite3.h>

#include <jansson.h>

#define FILTER 200

struct employee {
  int empID;
  char empName[50];
  float empSalary;
};

int main(int argc, char * argv[]) // total no of parameters in argc
//  argv[0] - filename ; argv[1] - seerver ip ; argv[2] - portno ;
{
  if (argc < 3) {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(1);
  }

  int portno = atoi(argv[2]);

  struct hostent * server; // store host info like hostname,      protocol,etc
  server = gethostbyname(argv[1]); // gets data from argv[1] ie ip addr of server
  if (server == NULL) {
    fprintf(stderr, "Error, no such host");
    exit(1);
  }

  // Socket Creation
  int sock = 0;
  sock = socket(AF_INET, SOCK_STREAM, 0); // Created socketfd

  if (sock < 0) {
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
  memcpy( & serv_addr.sin_addr.s_addr, server -> h_addr_list[0], server -> h_length); // copies data hostent *server to srv_addr

  // Connect to Server
  if (connect(sock, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0) {
    perror("Connection Failed");
    exit(EXIT_FAILURE);
  }

  int option;
  char choice;
  do {
    printf("Choose an option\n1.Insert\n2.Sort\n3.Search\n");
    scanf("%d", & option);
    if (option < 1 || option > 3) {
      printf("Invalid option. Please enter a number between 1 and 3.\n");
      return 1;
    }

    if (send(sock, & option, sizeof(int), 0) < 0) {
      perror("Error in sending the option of switch case");
      exit(EXIT_FAILURE);
    }

    switch (option) {
    case 1: {
      char name[100];
      int ID;
      float salary;

      json_t * base = json_object();
      if (!base) {
        fprintf(stderr, "Error creating base JSON object\n");
        exit(EXIT_FAILURE);
      }

      int numEmployees = 0;

      json_t * json_obj_array = json_array();
      if (!json_obj_array) {
        fprintf(stderr, "Error creating JSON array\n");
        json_decref(base); // Free previously allocated memory
        exit(EXIT_FAILURE);
      }

      printf("How many employess do you want to insert ?");
      scanf("%d", & numEmployees);
      if (numEmployees <= 0) {
        printf("Please enter a valid number of employees");
        return 0;
      }

      for (int i = 0; i < numEmployees; i++) {

        printf("Enter employee name: ");
        scanf("%s", name);

        // Validate name
        if (strlen(name) == 0) {
          fprintf(stderr, "Invalid employee name\n");
          exit(EXIT_FAILURE);
          break;
        }

        printf("Enter Employee ID: ");
        scanf("%d", & ID);

        // Validate ID
        if (ID <= 0) {
          fprintf(stderr, "Invalid employee ID\n");
          exit(EXIT_FAILURE);
          return 0;
        }

        printf("Enter Employee Salary: ");
        scanf("%f", & salary);

        // Validate salary
        if (salary <= 0.00) {
          fprintf(stderr, "Invalid employee salary\n");
          exit(EXIT_FAILURE);
          break;
        }

        // Creating json obj using jansson
        json_t * root = json_object();
        if (!root) {
          fprintf(stderr, "Error creating JSON object for employee %d\n", i);
          json_decref(base);
          json_decref(json_obj_array);
          exit(EXIT_FAILURE);
        }

        json_object_set_new(root, "name", json_string(name));
        json_object_set_new(root, "ID", json_integer(ID));
        json_object_set_new(root, "salary", json_real(salary));

        // // Validate the JSON object
        // if (!json_object_get(root, "name") || !json_object_get(root, "ID") || !json_object_get(root, "salary") ||
        //     !json_is_string(json_object_get(root, "name")) || !json_is_integer(json_object_get(root, "ID")) ||
        //     !json_is_real(json_object_get(root, "salary")) || name[0] == '\0')
        // {
        //     fprintf(stderr, "error: Invalid JSON format in object %d.\n", i);
        //     exit(EXIT_FAILURE);
        // }

        json_array_append_new(json_obj_array, root);
      }

      json_object_set_new(base, "data", json_obj_array); // Add array to a base object

      char * json_str = json_dumps(base, 0); // Convert JSON to string//

      if (send(sock, & numEmployees, sizeof(int), 0) < 0) {

        perror("Error while sending the numEmployees to server");
        return -1;
      }

      if (send(sock, json_str, strlen(json_str), 0) < 0) // sending data the server
      {

        perror("Error while sending the data to server");
        return -1;
      }

      // Cleanup
      free(json_str);
      json_decref(base);
      json_decref(json_obj_array);

      break;
    }

    case 2: {

      char sortChoice[10];
      printf("Choose how you want to sort the db\nID\nsalary\nname\n");
      scanf(" %[^\n]", sortChoice);

      int len = strlen(sortChoice);
      if (len > 0 && sortChoice[len - 1] == '\n') {
        sortChoice[len - 1] = '\0';
      }

      // Validate sortChoice
      if (strcmp(sortChoice, "ID") != 0 && strcmp(sortChoice, "salary") != 0 && strcmp(sortChoice, "name") != 0) {
        printf("Invalid choice. Please choose from ID, salary, or name.\n");
        return 0;
      }

      if (send(sock, sortChoice, strlen(sortChoice) + 1, 0) < 0) {
        perror("Error in sending the filter parameter");
        exit(EXIT_FAILURE);
      }

      break;
    }

    case 3: {
      // COLUMN NAME
      char colName[100] = {
        0
      };

      printf("Enter column name : \n");
      scanf("%s", colName);

      colName[strcspn(colName, "\n")] = '\0'; // Remove newline character

      // Send ColumnName
      if (send(sock, colName, strlen(colName) + 1, 0) < 0) {
        perror("Error in sending the columne name");
        exit(EXIT_FAILURE);
      }

      // OPERATOR
      char optr[3] = {
        0
      };

      printf("Enter Operator : \n");
      scanf("%s", optr);

      optr[strcspn(optr, "\n")] = '\0'; // Remove newline character

      // Send Operator
      if (send(sock, & optr, sizeof(optr), 0) < 0) {
        perror("Error in sending the operator parameter");
        exit(EXIT_FAILURE);
      }

      // VALUE
      char value[100] = {
        0
      };
      printf("Enter Value : \n");
      scanf("%s", value);

      value[strcspn(value, "\n")] = '\0'; // Remove newline character

      // Send Value
      if (send(sock, value, strlen(value), 0) < 0) {
        perror("Error in sending the value parameter");
        exit(EXIT_FAILURE);
      }

      break;
    }

    default:
      printf("Invalid option\n");
      break;
    }
    printf("Do you want to continue?\nPress 'Y' or 'y' to continue or any other key to exit\n");
    scanf(" %c", & choice);

    if (choice == 'Y' || choice == 'y') {

      if (send(sock, & choice, sizeof(char), 0) < 0) {
        perror("Error in sending the option to continue switch case or not");
        exit(EXIT_FAILURE);
      }
    }

  } while (choice == 'Y' || choice == 'y');

  return 0;
}