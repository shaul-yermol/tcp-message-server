#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <string.h>
#include <algorithm>

#include "Server.h"

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define MAX_CONCURRENT_CONNECTIONS 5

Server::Server(uint port, DataReceivedCallback callback) 
{
    m_callback = callback;
    m_port = port;
    m_shouldStopPipeFD[0] = 0;
    m_shouldStopPipeFD[1] = 0;
    m_listeningSocketFD = 0;
    m_shouldStop = false;
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
                m_messagesQueueCondVar.notify_one();
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

    } while(!m_shouldStop);

    printf("Connection closed!\n");
    close(conn_fd);
}

void Server::Stop()
{
    m_shouldStop = true;
    if (m_shouldStopPipeFD[1])
        write(m_shouldStopPipeFD[1],"0", 1);
    m_messagesQueueCondVar.notify_all();
}

void Server::Start()
{
    if (pipe(m_shouldStopPipeFD) < 0) 
    {
        handle_error("error in opening pipe!");
    }
    m_listeningSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    //bind the socket to the address
    if (setsockopt(m_listeningSocketFD,SOL_SOCKET,SO_REUSEADDR,(void*)&opt,sizeof(int)) < 0 )
    {
        handle_error("error in binding port!");
    }

    sockaddr_in serv_addr;
    
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(m_port);
    serv_addr.sin_addr.s_addr=INADDR_ANY;
 
    if(bind(m_listeningSocketFD,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
    {
        close(m_listeningSocketFD);
        handle_error("error in binding port!");
    }

    if (listen(m_listeningSocketFD, MAX_CONCURRENT_CONNECTIONS) < 0)
    {
        close(m_listeningSocketFD);
        handle_error("error in listen!");
    }

    printf("watining for connection on port %d...\n", m_port);

    do 
    {
        sockaddr_in client_addr;
        unsigned int client_addr_len =  sizeof(sockaddr_in);

        fd_set rfds;
        int nFDS = 0;
        FD_ZERO(&rfds);
        FD_SET(m_listeningSocketFD, &rfds);
        nFDS = std::max(nFDS, m_listeningSocketFD+1);
        FD_SET(m_shouldStopPipeFD[0], &rfds);
        nFDS = std::max(nFDS, m_shouldStopPipeFD[0] + 1);

        int retval = select(nFDS, &rfds, NULL, NULL, NULL);
        /* Don't rely on the value of tv now! */

        if (retval == -1)
            handle_error("select()");
        else if (retval) 
        {
            if (FD_ISSET(m_shouldStopPipeFD[0], &rfds))
            {
                printf("Listening thread received stop command.\n");
                break;
            }
        }
        printf("Incoming connection from client.\n");

        int conn_fd = accept(m_listeningSocketFD, (sockaddr*)&client_addr, &client_addr_len);
        if (conn_fd < 0) 
        {
            handle_error("listening socket closed, server stopped\n");
            break;
        }
        printf("connection accepted!\n");

        m_openThreads.push_back(std::thread(&Server::HandleConnection, this, conn_fd));
    } while (!m_shouldStop);

    if (m_listeningSocketFD)
        close(m_listeningSocketFD);
}

Server::~Server()
{
    if (!m_shouldStop) {
        Stop();
    }
    std::for_each(m_openThreads.begin(), m_openThreads.end(), [](std::thread& t) {
        if (t.joinable()) {
            t.join();
        }
    });
}

std::shared_ptr<std::vector<char>> Server::PopMessage()
{
    std::lock_guard<std::mutex> lock(m_messagesQueueMutex);
    if (m_shouldStop) {
        return NULL;
    }
    if (m_messagesQueue.size() > 0) {
        auto tmp = m_messagesQueue.front();
        m_messagesQueue.pop();
        return tmp;
    }
    return NULL;
}

std::shared_ptr<std::vector<char>> Server::PopMessageBlocking()
{
    std::unique_lock<std::mutex> lock(m_messagesQueueMutex);
    m_messagesQueueCondVar.wait(lock, [this] { 
        return (this->m_messagesQueue.size() > 0 || m_shouldStop); 
    });
    if (m_shouldStop) {
        return NULL;
    }
    auto tmp = m_messagesQueue.front();
    m_messagesQueue.pop();
    lock.unlock();
    return tmp;
}