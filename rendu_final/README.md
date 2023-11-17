# Network Communication Programs

This repository contains a set of network communication programs implemented in C. The programs include a client program, a server, a sender, and a receiver.

## Compilation

### Prerequisites
- Make sure you have `gcc` installed on your system.

### Compilation Command
To compile all the programs, run the following command in the terminal:
- make clean
- make sender
- make receiver

To run the server program in one terminal, use the following command:
- make run-server
This will execute the server with the default parameter (PORT = 8080).

To run the client program in other terminals, use the following command:
- make run-client
This will execute the client with default parameters (HOST = 127.0.0.1 and PORT = 8080).
You can modify these default parameters in the Makefile file.

The sender and receiver program will be execute with default parameters (FILL_ADDR = 127.0.0.1, FILE_PORT = 8888).
You can modify these default parameters in the common.h file.


### The terminal client supports the following commands:
## Users
/nick <your_pseudo> - Set your nickname
/who - List online users
/whois <username> - Display information about a specific user
/msgall <message> - Send a message to all online users
/msg <username> <message> - Send a private message to a specific user

## Chat room
/create <channel_name> - Create a new channel
/channel_list - List of channels
/join <channel_name> - Join an existing channel
/quit <channel_name> - Quit a channel
# Bonus : 
/channel_members <channel_name> - List of channel member

## File transfer
/send <username> <file_path> - Send a file to a specific user

## Bonus
/help - Display this help message with all the available commands
/kill_server - Stop the chat