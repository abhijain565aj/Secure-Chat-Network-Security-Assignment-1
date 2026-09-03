#include "server.hpp"

int main()
{
    Server server(12345);
    server.startServer();
    server.eventLoop();
    server.stopServer();

    return 0;
}