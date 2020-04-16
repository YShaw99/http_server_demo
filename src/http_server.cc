#include "../include/http_server.hpp"

void hs::exit_error(string info) {
  cerr << info << endl;
  exit(-1);
}

void hs::send_error(int fd_client, int status, const char *title,
                    const char *text) {

  char buff[4096] = {0};

  sprintf(buff, "HTTP/1.1 %d %s\r\n", status, title);
  sprintf(buff + strlen(buff), "Content-Type:%s\r\n", "text/html");
  sprintf(buff + strlen(buff), "Content-Length:%d\r\n", -1);
  sprintf(buff + strlen(buff), "Connection: close\r\n");

  cout << "send :" << buff << endl;
  send(fd_client, buff, strlen(buff), 0);
  send(fd_client, "\r\n", 2, 0);

  memset(buff, 0, sizeof(buff));

  sprintf(buff, "<html><head><title>%d %s</title></head>", status, title);
  sprintf(buff + strlen(buff),
          "<h2 align=\"center\">%d %s</h2>\n", //
          status, title);
  sprintf(buff + strlen(buff), "%s\n", text);
  sprintf(buff + strlen(buff), "\n</body>\n</html>\n");

  send(fd_client, buff, strlen(buff), 0);
}

// epoll的主体函数
//创建一个epoll，并进行初始化
//
void hs::epoll_run(int port) {
  //创建epoll
  int i = 0, fd_epoll, fd_listen;

  fd_epoll = epoll_create(FD_MAX_NUM);
  if (fd_epoll == -1) {
    exit_error("epoll create error");
  }

  //委托内核检测添加到树上的结点
  epoll_event event_nodes[FD_MAX_NUM];

  //加入监听套接字
  fd_listen = init_listen_fd(port, fd_epoll);
  cout << "epoll_run success" << endl;
  //处理请求
  while (1) {

    int res = epoll_wait(fd_epoll, event_nodes, FD_MAX_NUM, 500);
    if (res == -1) {
      exit_error("epoll wait error!");
    }
    //处理每个类型请求
    //待改善功能：目前只处理连接&读
    for (int i = 0; i < res; i++) {
      epoll_event *event = &event_nodes[i];

      if (!(event->events & EPOLLIN)) {
        cout << "非输入事件" << endl;
        continue;
      }

      if (event->data.fd == fd_listen) {
        do_accept(fd_listen, fd_epoll); //处理连接
      } else {
        do_read(event->data.fd, fd_epoll); //读取数据
      }
    }
  }
}

//创建并初始化一个监听套接字;
//将监听套接字加入到epoll
int hs::init_listen_fd(int port, int fd_epoll) {
  int fd_listen = socket(AF_INET, SOCK_STREAM, 0); // ipv4  TCP
  if (fd_listen < 0) {
    exit_error("socket error");
  }

  //初始化sockaddr_in
  sockaddr_in addr_server;
  memset(&addr_server, 0, sizeof(sockaddr_in));

  addr_server.sin_family = AF_INET;
  addr_server.sin_port = htons(port);
  addr_server.sin_addr.s_addr = htonl(INADDR_ANY);

  //端口复用
  int flag = 1;
  setsockopt(fd_listen, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

  // bind and listen
  int res = bind(fd_listen, (sockaddr *)&addr_server, sizeof(addr_server));
  if (res == -1) {

    exit_error("bind error");
  }

  res = listen(fd_listen, 64);
  if (res == -1) {
    exit_error("listen error");
  }

  //监听套接字的事件初始化
  epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = fd_listen;

  res = epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_listen, &event);
  if (res == -1) {
    exit_error("listen error");
  }
  cout << "init success ,listen fd:" << fd_listen << endl;
  return fd_listen;
}

//处理连接请求，打印客户端信息
// client fd设为非阻塞，挂载到epoll树上
int hs::do_accept(int fd_listen, int fd_epoll) {
  // client地址信息
  sockaddr_in addr_client;
  socklen_t len = sizeof(addr_client);
  int fd_client = accept(fd_listen, (sockaddr *)&fd_client, &len);
  if (fd_client == -1) {
    exit_error("accept error");
  }

  //打印客户端信息
  char ip_client[64];
  int port_client = ntohs(addr_client.sin_port);
  inet_ntop(AF_INET, &addr_client.sin_addr.s_addr, ip_client, 64);

  memset(ip_client, 0, 64);
  cout << "来自" << ip_client << ":" << port_client << "的新连接,fd_client为"
       << fd_client << endl;

  //设置非阻塞
  int flag = fcntl(fd_client, F_GETFL);
  flag |= O_NONBLOCK;
  fcntl(fd_client, F_SETFL, flag);

  //读数据、设为ET模式
  epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = fd_client;

  //挂载
  int res = epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_client, &event);
  if (res == -1) {
    char *info = new char[64];
    sprintf(info, "%s:%d add error, client fd:%d", ip_client, port_client,
            fd_client);
    exit_error(string(info));
  }
  ///-/ m[fd_client] = {string(ip_client), port_client};

  return 0;
}

// epoll树上删除节点
void hs::del_node(int fd_client, int fd_epoll) {
  int res = epoll_ctl(fd_epoll, EPOLL_CTL_DEL, fd_client, NULL);
  if (res == -1) {
    char *info = new char[64];
    ///-/sprintf(info, "%s:%d del error, client fd:%d",
    ///-/m[fd_client].first.c_str(),
    ///-/        m[fd_client].second, fd_client);
    sprintf(info, " del error, client fd:%d", fd_client);
    exit_error(string(info));
  } else {
    cout << "close client fd:" << fd_client << endl;
    close(fd_client);
    ///-/cout << m[fd_client].first << ":" << m[fd_client].second << "exit";
    ///-/m.erase(fd_client);
  }
}

void hs::do_read(int fd_client, int fd_epoll) {
  char line[1024] = {0};
  int len = get_line(fd_client, line, sizeof(line));
  if (len == 0) {
    del_node(fd_client, fd_epoll);
  } else {
    cout << "----------请求头----------" << endl;
    cout << "请求行:" << line << endl;
    // ET模式循环读取
    while (1) {

      char buff[1024] = {0};
      len = get_line(fd_client, buff, sizeof(buff));

      if ((buff[0] == '\n') || (len == -1)) {
        break;
      }
      cout << buff << endl;
    }
    cout << "---------- End ----------" << endl;
  }

  if (strncasecmp("get", line, 3) == 0) {
    http_request(line, fd_client);

    del_node(fd_client, fd_epoll);
  }
}

///-??
int hs::get_line(int sock, char *buff, int size) {
  int i = 0;
  char c = '\0';
  int n;
  while ((i < size - 1) && (c != '\n')) {
    n = recv(sock, &c, 1, 0);

    if (n > 0) {
      if (c == '\r') {
        buff[i] = c;
        recv(sock, &c, 1, 0);
        if (c == '\n') {
          buff[i] = c;
          i++;
          break;
        }
      }
      buff[i] = c;
      i++;
    } else {
      c = '\n';
    }
  }

  buff[i] = '\0';
  if (n == -1) {
    i = n;
  }

  return i;
}

void hs::send_file(int fd_client, const char *filename) {

  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    send_error(fd_client, 404, "Not Found", "No such file or direntry");
    exit_error("open file error");
  }

  //循环读文件
  ///-/*
  char buff[4096] = {0};
  int len = 0, res = 0;
  while ((len = read(fd, buff, sizeof(buff))) > 0) {
    res = send(fd_client, buff, len, 0);
    if (res == -1) {

      if (errno == EAGAIN) {
        cerr << "send error" << endl;
        continue;
      } else if (errno == EAGAIN) {
        cerr << "send error" << endl;
        continue;
      } else {
        cerr << "send error" << endl;
        exit(1);
      }
    }
  }
  if (len == -1) {
    cerr << "read file error" << endl;
    exit(1);
  }

  close(fd);
}

// determines the type of a file
///-/*
const char *hs::get_file_type(char *filename) {
  char *dot;
  dot = strrchr(filename, '.');
  if (dot == nullptr)
    return "text/plain; charset=utf-8";
  if (strcmp(dot, ".html") == 0)
    return "text/html; charset=utf-8";
  ///.........

  ///  map<const char *, const char *> type{{}, //
  ///                                    {}};
  return "text/plain; charset=utf-8";
}

///-/*
void hs::http_request(const char *request, int fd_client) {
  //拆分http请求行
  char method[12], path[1024], protocol[12];
  sscanf(request, "%[^ ] %[^ ] %[^ ] ", method, path, protocol);
  //获取文件属性
  char *file = path + 1;

  struct stat st; //要加上struct!
  int res = stat(file, &st);
  if (res == -1) {
    cout << "404 error" << endl;
    send_error(fd_client, 404, "Not Found", "No such file or direntry");
    return;
  }

  cout << "request:" << method << " " << path << " " << protocol << endl;

  //如果请求的是文件
  if (S_ISREG(st.st_mode)) {
    //发送消息报头
    send_respond_head(fd_client, 200, "OK", get_file_type(file), st.st_size);

    //发送文件内容
    send_file(fd_client, file);
  }
}

///-/*
void hs::send_respond_head(int fd_client, int no, const char *desp,
                           const char *type, long len) {
  char buff[1024] = {0};
  //状态行
  sprintf(buff, "http/1.1 %d %s\r\n", no, desp);
  cout << "状态行 " << buff << endl;
  send(fd_client, buff, strlen(buff), 0);
  //消息报头
  sprintf(buff, "Content-Type:%s\r\n", type);
  sprintf(buff + strlen(buff), "Content-Length:%ld\r\n", len);
  send(fd_client, buff, strlen(buff), 0);
  cout << "消息报头 " << buff << endl;

  //空行
  send(fd_client, "\r\n", 2, 0);
}
///待修改
/// m[fd_client] = {string(ip_client), port_client};
/// sprintf(info, "%s:%d del error, client fd:%d", m[fd_client].first.c_str(),
///  cout << m[fd_client].first << ":" << m[fd_client].second << "exit";
