#include "sylar/sylar.h"
#include "sylar/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>   // memset
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>   // inet_addr (如果需要指定 IP)

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int sock = 0;

void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "test logger sock = " << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);              // big endian
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    connect(sock, (const sockaddr*)&addr, sizeof(addr));
    sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ, [](){
        SYLAR_LOG_INFO(g_logger) << "connected";
    });

    sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE, [](){
        SYLAR_LOG_INFO(g_logger) << "connected";
    });

    //
}

void test1() {
    sylar::IOManager iom;
    iom.schedule(&test_fiber);
}

int main(int argc, char** argv) {
    test1();
    return 0;
}