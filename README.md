# Server.cpp

This project is a simple example of a TCP message server written in C++. The server should listen on a given TCP port, receive messages from clients and print them to `stdout`. For every message received it should reply the client with "OK" message. 
### Message format
The message can contain any printable characters, `\n` caracter is a message delimiter. Maximum message length should not exceed `1024` characters (including delimiters).
### Build
##### Requirements
Linux, g++, make
##### Build process
You can use `make` command in the terminal.
### Testing
1. Run server from the terminal using this command:
`Server -p [port number] `
You can specify maximum timeout the server should work: 
`Server -p [port number] -t [time in seconds]`
2. Run one (or more) clients from a terminal using this command:
`nc localhost [port number]`
3. Type messages in the `nc` prompt, `enter` key (i.e. `\n`) can be used to finish a message.
4. You can terminate the server and clients at any time using `Ctrl + C ` keys combination or wait for the server to finish working after the specifyed timeout (`-t` flag)
### Known issues
- No under-load test was performed, only simple acceptance.
- The server can not be gracefully closed, only terminated by kill signal (`Ctrl + C`) or finish after a timeout.
- Each new connection will increase memory use by some small amount  (a leak), due to storage of all std::pthread objects. They will be released when the object is deleted.
