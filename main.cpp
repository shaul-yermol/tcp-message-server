#include "Server.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: Server %s [-p port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int opt;
    int port = 5555;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    Server s(port, [] (const std::shared_ptr<const std::vector<char>> message) {
        if (message->size() > 0 )
            printf("Callback with message: %s\n", message->data());
        else 
            printf("Callback with empty message.\n");
    });
    
    std::thread t ([&s](){
        s.Start();
    });

    while(1){
        auto message = s.PopMessage();
        if (message.get() != 0) 
        {
            if (message->size() > 0 )
                printf("Pop message: %s\n", message->data());
            else 
                printf("Pop empty message.\n");
        }
    }
}