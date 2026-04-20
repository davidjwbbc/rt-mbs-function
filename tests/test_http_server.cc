#include <netinet/in.h>
#include <unistd.h>

#include <filesystem>

#include <DirectoryIndexHandler.hh>
#include <DocrootHTTPRequestHandler.hh>
#include <HTTPServer.hh>
#include <SockAddr.hh>

HTTPXPP_USING_NAMESPACE;

int main(int argc, char *argv[])
{
    SockAddr addr(sockaddr_in6{.sin6_family = AF_INET6, .sin6_port = htons(8000), .sin6_addr = IN6ADDR_ANY_INIT});
    auto *indexer = new DirectoryIndexHandler;
    auto *handler = new DocrootHTTPRequestHandler("docroot", std::shared_ptr<DocrootHTTPRequestHandler::IndexHandler>(indexer));
    handler->addMimeType("sdp", "application/session-description");
    HTTPServer svr(addr, std::shared_ptr<HTTPRequestHandler>(handler));
    svr.run();

    sleep(60);

    return 0;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
