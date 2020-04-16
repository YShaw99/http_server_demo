#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#define FD_MAX_NUM 10000
#define SERVER_PORT 8000

using namespace std;

namespace hs {

// v
void exit_error(string info);

// send error,for example 404.
void send_error(int fd_client, int status, const char *title, const char *text);
// v
int init_listen_fd(int port, int fd_epoll);

// v
void epoll_run(int port);

int do_accept(int fd_listen, int fd_epoll);

void do_read(int fd_client, int fd_epoll);

void del_node(int fd_client, int fd_epoll);

int get_line(int sock, string buff, int size);

void send_file(int fd_client, const char *filename);

// determines the type of a file
const char *get_file_type(char *filename);

void send_respond_head(int fd_client, int no, const char *desp,
                       const char *type, long len);

void http_request(const char *request, int fd_client);

/*
/// all char* string
/// epoll_event event_nodes[FD_MAX_NUM];-》vector
///-define
/// do_read char line[1024]
map<int, pair<string, int>> m; // fd , {ip,port}
int server_port;
int fd_max_num;
string path;

public:
hs(string path = "./", int server_port = 8080, int fd_max_num = 10000) {
this->path = path;
this->server_port = server_port;
this->fd_max_num = fd_max_num;
}

*/

}; // namespace hs
