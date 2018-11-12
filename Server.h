#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include <thread>
#include <memory> 
#include <vector>
#include <queue>
#include <mutex>

typedef void (*DataReceivedCallback)(const std::shared_ptr<const std::vector<char>> message);

class Server
{
public:
    Server(uint port, DataReceivedCallback callback);
    void Start();
    ~Server();
    std::shared_ptr<std::vector<char>> PopMessage();
protected:
    void HandleConnection(int conn_fd);
    uint m_port;
    DataReceivedCallback m_callback;
    std::vector<std::thread> m_openThreads;
    std::queue<std::shared_ptr<std::vector<char>>> m_messagesQueue;
    std::mutex m_messagesQueueMutex;
};

#endif