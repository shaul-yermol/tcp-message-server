#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <string.h>

#include "Server.h"

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define MAX_CONCURRENT_CONNECTIONS 5

Server::Server(uint port, DataReceivedCallback callback) 
{
    m_callback = callback;
    m_port = port;
}

#define BUFFER_SIZE 1024
void Server::HandleConnection(int conn_fd)
{
    printf("Started new trhread handling connection...");

    char buf[BUFFER_SIZE] = {0};
    char* next_message_ptr = &buf[0];
    do 
    {
        int read = recv(conn_fd, next_message_ptr, &buf[BUFFER_SIZE] - next_message_ptr, 0);
        next_message_ptr += read;

        if (!read) 
            break; // done reading

        if (read < 0)
            handle_error("Client read failed\n");

        auto bufferForCallback = std::make_shared<std::vector<char>>();

        char* message_start_ptr = &buf[0];
        for (char* curr_ptr = message_start_ptr; curr_ptr < next_message_ptr; curr_ptr++)
        {
            if (*curr_ptr == '\n')
            {
                if ( send(conn_fd, "OK\n", 3, 0) < 0 )
                    handle_error("Client write failed\n");
                auto message = std::make_shared<std::vector<char>>();
                message->resize(curr_ptr - message_start_ptr);
                memcpy(message->data(), message_start_ptr, curr_ptr - message_start_ptr);
                m_callback(message);
                { // scope for mutex
                    std::lock_guard<std::mutex> lock(m_messagesQueueMutex);
                    m_messagesQueue.push(message);
                }
                message_start_ptr = curr_ptr + 1;
            }
        }

        if (message_start_ptr > &buf[0])
        {
            size_t last_message_size = message_start_ptr - &buf[0];
            char tmp_buf[last_message_size] = {0};
            memcpy(tmp_buf, message_start_ptr, next_message_ptr - message_start_ptr);
            memcpy(buf, tmp_buf, next_message_ptr - message_start_ptr);
            next_message_ptr = next_message_ptr - (message_start_ptr - &buf[0]);
            message_start_ptr = &buf[0];
        }

    } while(1);

    printf("Connection closed!\n");
    close(conn_fd);
}

void Server::Start()
{
    int socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    //bind the socket to the address
    if (setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,(void*)&opt,sizeof(int)) < 0 )
    {
        handle_error("error in binding port!");
    }

    sockaddr_in serv_addr;
    
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(m_port);
    serv_addr.sin_addr.s_addr=INADDR_ANY;
 
    if(bind(socketfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
    {
        close(socketfd);
        handle_error("error in binding port!");
    }

    if (listen(socketfd, MAX_CONCURRENT_CONNECTIONS) < 0)
    {
        handle_error("error in listen!");
    }

    printf("waining for connection on port %d...\n", m_port);

    do 
    {
        sockaddr_in client_addr;
        unsigned int client_addr_len =  sizeof(sockaddr_in);
        int conn_fd = accept(socketfd, (sockaddr*)&client_addr, &client_addr_len);
        if (conn_fd < 0) 
        {
            handle_error("error in accept connection!");
        }
        printf("connection accepted!\n");

        m_openThreads.push_back(std::thread(&Server::HandleConnection, this, conn_fd));
    } while (1);
}

Server::~Server()
{

}

std::shared_ptr<std::vector<char>> Server::PopMessage()
{
    std::lock_guard<std::mutex> lock(m_messagesQueueMutex);
    if (m_messagesQueue.size() > 0) {
        auto tmp = m_messagesQueue.front();
        m_messagesQueue.pop();
        return tmp;
    }
    else 
    {
        return 0;
    }
}