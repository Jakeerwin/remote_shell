# Remote Shell Assignment

## Description
This project implements a simple remote shell using C and TCP sockets.
The client connects to a server, sends shell commands, and displays the output.

## How to Compile
Use the provided Makefile:
```sh
make
```

## How to Run
### 1. Start the Server
```sh
./server
```

### 2. Start the Client (in another terminal)
```sh
./client <server-ip>
```
Example:
```sh
./client 127.0.0.1
```

## Quit
To exit the shell from client, type:
```sh
quit
```

## Notes
- Works only with commands that do not require `stdin`.
- Handles multiple clients using `fork()`.

## Clean Up
```sh
make clean
```