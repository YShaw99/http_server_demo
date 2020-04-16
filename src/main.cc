#include "../include/http_server.hpp"
#include <iostream>
using namespace std;
int main() {
  chdir("./");
  hs::epoll_run(8000);
  return 0;
}