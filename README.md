# C-MinorProject

1. Compile the 'JServer.c' file using the command in your terminal  --> gcc JServer.c -o JServer -lsqlite3 -ljansson

2. Run the code and provide a port number --> ./JServer portnumber_of _your_choice (eg. ./JServer 9999) 

3. Compile the 'JClient.c' file using the command in another terminal  -- gcc JClient.c -o JClient -ljansson

4. Run the code and provide an ipaddress and port number --> ./JServer ip_address portnumber_of _your_choice (eg. ./JServer 127.0.0.1 9999) 


