#include "Server.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

int main(int argc, char *argv[])
{
    // Parsing the command line arguments.
    int opt;
    int port = -1;
    int timeout = 0;
    if (argc < 3) {
        fprintf(stderr, "Usage: %s -p <port> [-t timeout_seconds]\n", argv[0]);
            exit(EXIT_FAILURE);
    }
    while ((opt = getopt(argc, argv, "t:p:")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        case 't':
            timeout = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s -p <port> [-t timeout_seconds]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (port == -1) {
        fprintf(stderr, "Usage: %s -p <port> [-t timeout_seconds]\n", argv[0]);
        exit(EXIT_FAILURE);        
    }

    volatile bool running = true;

    // Sample code to show usage of Server class constructor
    Server* s = new Server(port, [] (const std::shared_ptr<const std::vector<char>> message) {
        if (message->size() > 0 )
            printf("Callback with message: %s\n", message->data());
        else 
            printf("Callback with empty message.\n");
    });
    
    // Sample code to show usage of Start() function. Keep in mind, that this function is blocking.
    std::thread t ([&s](){
        s->Start();
    });
    
    // Sample code to show usage of PopMessage() function
    std::thread t1 ([&s, &running](){
        while(running){
            auto message = s->PopMessage();
            if (message.get() != 0)
            {
                if (message->size() > 0 )
                    printf("Pop message (polling): %s\n", message->data());
                else 
                    printf("Pop empty message (polling).\n");
            }
        }
    });

    // Sample code to show usage of PopMessageBlocking() function
    std::thread t2 ([&s, &running](){
        while(running){
            auto message = s->PopMessageBlocking();
            if (message.get() != 0)
            {
                if (message->size() > 0 )
                    printf("Pop message (blocking): %s\n", message->data());
                else 
                    printf("Pop empty message (blocking).\n");
            }
        }
    });

    for (int i = 0; i < timeout || timeout==0; i++) 
    {
        sleep(1);
        printf(".\n");
    }
    
    printf("Stop server!\n");
    running = false;

    // Sample code to show usage of Stop() function
    s->Stop();

    printf("Joining listnenig thread...\n");
    t.join();
    printf("Joining polling thread...\n");
    t1.join();
    printf("Joining polling thread...\n");
    t2.join();

    delete(s);
}