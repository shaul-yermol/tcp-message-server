# Server.cpp

This project is a simple example of a TCP message server written in C++. The server should listen on a given port, receive connections from clients and print them to `stdout`. For every message received it should reply the client with "OK" message. 
### Message format
The message can contain any printable characters, `\n` caracter is a message delimiter. Maximum message length should not exceed `1024` characters (including delimiters).
### Build
##### Requirements
Linux, g++, make
##### Build process
You can use `make` command in the terminal.
### Testing
Run server from the terminal using this command:
`Server -p [port number] `
Run multiple clients from terminal using this command:
`nc localhost [port number]`
Type messages in the `nc` prompt, `enter` key (i.e. `\n`) can be used to finish a message.
You can terminate the server and clients at any time using `Ctrl + C ` keys combination.
### Known issues
- No under-load test was performed, only simple acceptance.
- The server can not be gracefully closed, only terminated by kill signal (`Ctrl + C`)
- Each new connection will increase memory use by some small amount  (a leak), due to storage of all std::pthread objects.