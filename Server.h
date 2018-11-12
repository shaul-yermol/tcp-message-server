#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include <thread>
#include <memory> 
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

typedef void (*DataReceivedCallback)(const std::shared_ptr<const std::vector<char>> message);

class Server
{
public:
    Server(uint port, DataReceivedCallback callback);

    // Start listening for new connections (blocking operation)
    void Start();
    // Stop listening  for new connections (non-blocking operation)
    void Stop();

    // Non-blocking (polling) pop messages. 
    // Returns null pointer if there are no messages in queue.
    std::shared_ptr<std::vector<char>> PopMessage();

    // Blocking pop messages. Alwais return a valid message.
    std::shared_ptr<std::vector<char>> PopMessageBlocking();

    ~Server();

protected:
    // Connection handler, should run on a separate thread.
    void HandleConnection(int conn_fd);

    // Data members.
    uint m_port;
    DataReceivedCallback m_callback;
    std::vector<std::thread> m_openThreads;
    std::queue<std::shared_ptr<std::vector<char>>> m_messagesQueue;
    std::mutex m_messagesQueueMutex;
    std::condition_variable m_messagesQueueCondVar;
    bool m_shouldStop;
    int m_shouldStopPipeFD[2];
    int m_listeningSocketFD;
};

#endif
