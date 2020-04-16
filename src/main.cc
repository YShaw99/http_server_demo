#include "../include/http_server.hpp"
#include <iostream>
using namespace std;
#define SERVER_PORT 8000
int main() {
  chdir("./");
  hs::epoll_run(SERVER_PORT);
  return 0;
}