PROJECT 2 - MULTI-THREADING AND NETWORK CONNECTIONS
========================================

Files included:
--------------
- dbserver.c    : Main implementation of the multi-threaded database server
- Makefile      : Build instructions for the project
- testing.sh    : Test script to verify server functionality
- proj2.h       : Provided header file (not modified)
- queue.*       : Work queue implementation
- table.*       : Functions for handling table updates


Implementation Details:
---------------------
1. The server has 6 threads:
   - Main thread: Handles console commands
   - Listener thread: Accepts connections on a TCP socket
   - 4 Worker threads: Process database operations: read/write/delete requests

2. The database:
   - Stores key-value pairs in files named /tmp/data.X where X is the index
   - Supports up to 200 keys
   - Implements three states for keys: INVALID, BUSY, and VALID
   - Uses mutexes to prevent race conditions

3. Operations:
   - Write (W): Store a value for a key
   - Read (R): Retrieve a value for a key
   - Delete (D): Remove a key and its value

4. Synchronization:
   - Uses mutexes to protect the database table and work queue
   - Uses condition variables to signal worker threads




Building the project:
-------------------
To build the project, simply run:
    make

This will compile both the dbserver and dbtest programs.

Running the server:
-----------------
To run the server:
    ./dbserver [port]

Where [port] is an optional parameter to specify the port (default is 5000).

Server Commands:
--------------
The server supports the following commands on the console:
- stats  : Display server statistics
- quit   : Terminate the server

Testing:
-------
A comprehensive test script is provided in testing.sh. To run the tests:
    bash testing.sh



Notes:
-----
The server uses the working directory /tmp to store database files. Make sure this directory is accessible and writable.