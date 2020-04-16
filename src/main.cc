#include "../include/http_server.hpp"
#include <iostream>
using namespace std;
int main() {
  hs::epoll_run(8080);
  return 0;
}