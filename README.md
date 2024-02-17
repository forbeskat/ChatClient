Chat Client
=========

This is a chat client program inspired by classical chat systems such as IRC. It allows users to communicate synchronously with a server in real-time over a TCP connection. The program supports two types of clients: receivers, who can read messages from the server, and senders, who can send messages to the server. The server partitions clients into rooms, enabling multiple groups of people to converse independently.

Features
--------

Two types of clients: receiver and sender.   
Messages are exchanged between clients and the server using a defined protocol.  
Support for concurrency and synchronization primitives.  
Clients are organized into rooms for group communication.  
Command-line interface for joining rooms, leaving rooms, and quitting the application.  

Functionality
---------------

**Receiver:**

Receives messages from the server in real-time.  
Terminates connection by simply closing its socket.  
Usage: ./receiver [server_address] [port] [username] [room]  

**Sender:**

Logs into the server with a specified username.  
Reads input from stdin for messages and commands.  
Commands start with the / character and include /join [room name], /leave, and /quit.  
Usage: ./sender [server_address] [port] [username]  


Instructions for Running the Application:
------------------

Clone or download the repository containing the source code for the chat client program.  
Navigate to the directory containing the source code.  
Compile the source code using a C++ compiler.  
Run the server program on a specified port.  
Open terminals for sender and receiver clients.  
Run the sender client with the appropriate command-line arguments.  
Run the receiver client with the appropriate command-line arguments.  
Begin communicating in real-time with other users on the server.  


