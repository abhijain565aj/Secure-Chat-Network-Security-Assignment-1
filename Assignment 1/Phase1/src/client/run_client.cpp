#include "client.hpp"
int main()
{
    Client client;
    client.connectToServer("127.0.0.1", 12345);
    client.eventLoop();
    client.closeConnection();
    return 0;
}