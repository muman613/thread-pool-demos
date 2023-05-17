//
// Created by Michael Uman on 10/28/20.
//
#include "fooSocketMan.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <memory.h>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

namespace foo {
namespace net {
std::string dump_vector(const dataPacket &vec) {
  std::ostringstream os;

  os << "[";
  size_t t = 0;
  for (const auto n : vec) {
    if ((t != 0) && (t != vec.size()))
      os << " ";
    os << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)n;
    t++;
  }
  os << "]";

  return os.str();
}

std::ostream &operator<<(std::ostream &os, const dataPacket &vec) {
  os << "[";
  size_t t = 0;
  for (const auto n : vec) {
    if ((t != 0) && (t != vec.size()))
      os << " ";
    os << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)n;
    t++;
  }
  os << "]";
  return os;
}

static int make_client_socket(const char *hostname, uint16_t port) {
  /* Create a socket point */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("ERROR opening socket");
    return -1;
  }

  struct hostent *server = gethostbyname(hostname);
  if (server == nullptr) {
    perror("Unable to resolve host");
    return -1;
  }

  struct sockaddr_in serv_addr;
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(port);

  /* Now connect to the server */
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR connecting");
    return -1;
  }

  return sockfd;
}

static int make_server_socket(uint16_t port) {
  int sock;
  struct sockaddr_in name;

  /* Create the socket. */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  struct timeval tv;
  tv.tv_sec = RECV_TIMEOUT_SEC;
  tv.tv_usec = RECV_TIMEOUT_USEC;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

  // Disable "address already in use" error message
  int yes = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(sock, 1) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return sock;
}

#if 0
        static void init_sockaddr (struct sockaddr_in *name,
                                   const char *hostname,
                                   uint16_t port)
        {
          struct hostent *hostinfo = nullptr;

          name->sin_family = AF_INET;
          name->sin_port = htons (port);
          hostinfo = gethostbyname (hostname);
          if (hostinfo == nullptr)
          {
            (Error, "Unknown host ", hostname);
            exit (EXIT_FAILURE);
          }
          name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
        }
#endif

SocketHandler::SocketHandler(const std::string &name, const std::string &host,
                             uint16_t port, bool server)
    : name_(name), host_(host), server_(server), port_(port) {
  if (server) {
    // open server port
    socket_fd_ = make_server_socket(port);
  } else {
    socket_fd_ = make_client_socket(host.c_str(), port);
  }
}

SocketHandler::~SocketHandler() {
  if (socket_fd_ != -1) {
    close(socket_fd_);
    socket_fd_ = -1;
  }
}

bool SocketHandler::onReceivePacket(const dataPacket &pkt) {

  if (receivedCb_) {
    receivedCb_(name_, socket_fd_, pkt);
  }

  return false;
}

bool SocketHandler::onReceivePacket(size_t index, const dataPacket &pkt) {

  if (receivedCb_) {
    receivedCb_(name_, clientFds_[index], pkt);
  }

  return false;
}

bool SocketHandler::readPacket(dataPacket &data) const {

  MacroHeader hdr{};

  memset(&hdr, 0, sizeof(hdr));

  auto bytes_read = read(socket_fd_, &hdr, IPC_PACKET_HEADER_SIZE);
  if (bytes_read != IPC_PACKET_HEADER_SIZE) {
    return false;
  }

  if (ntohs(hdr.sop) != IPC_PACKET_HEADER_SOP) {
    return false;
  }

  auto dataSize = ntohs(hdr.size) - IPC_PACKET_HEADER_SIZE;
  auto *packetBuffer = new uint8_t[dataSize];

  bytes_read = read(socket_fd_, packetBuffer, dataSize);
  data = dataPacket(packetBuffer, packetBuffer + dataSize);

  return true;
}

bool SocketHandler::writePacket(dataPacket &data) const {
  MacroHeader hdr;

  hdr.sop = htons(IPC_PACKET_HEADER_SOP);
  hdr.size = htons(IPC_PACKET_HEADER_SIZE + data.size());

  auto bytes_written = write(socket_fd_, &hdr, IPC_PACKET_HEADER_SIZE);
  if (bytes_written != IPC_PACKET_HEADER_SIZE) {
    return false;
  }

  bytes_written = write(socket_fd_, data.data(), data.size());

  return false;
}

int SocketHandler::accept() {

  assert(server_ == true); // Accept only allowed on server Socket
  sockaddr_storage remoteaddr = {0};
  socklen_t addr_size = sizeof(remoteaddr);

  int client_fd =
      ::accept(socket_fd_, (struct sockaddr *)&remoteaddr, &addr_size);
  clientFds_.push_back(client_fd);

  if (connectCb_) {
    connectCb_(name_, client_fd);
  }

  return client_fd;
}

bool SocketHandler::has_client(int fd, size_t &index) {
  size_t i = 0;
  for (auto client_fd : clientFds_) {
    if (fd == client_fd) {
      index = i;
      return true;
    }
    i++;
  }

  return false;
}

bool SocketHandler::is_server(int fd) const {
  return server_ && (fd == socket_fd_);
}

bool SocketHandler::readPacket(size_t client_id, dataPacket &data) {
  bool result = false;
  MacroHeader hdr{};
  int fd = clientFds_[client_id];

  memset(&hdr, 0, sizeof(hdr));

  auto bytes_read = read(fd, &hdr, IPC_PACKET_HEADER_SIZE);
  if (bytes_read != IPC_PACKET_HEADER_SIZE) {
    return false;
  }

  if (ntohs(hdr.sop) != IPC_PACKET_HEADER_SOP) {
    return false;
  }

  auto dataSize = ntohs(hdr.size) - IPC_PACKET_HEADER_SIZE;
  auto *packetBuffer = new uint8_t[dataSize];

  bytes_read = read(fd, packetBuffer, dataSize);
  if (bytes_read == dataSize) {
    data = std::move(dataPacket(packetBuffer, packetBuffer + dataSize));
    result = true;
  }

  return result;
}

bool SocketHandler::writePacket(size_t client_id, dataPacket &data) {
  int fd = clientFds_[client_id];

  MacroHeader hdr = {.sop = htons(IPC_PACKET_HEADER_SOP),
                     .size = htons(IPC_PACKET_HEADER_SIZE + data.size())};

  auto bytes_written = write(fd, &hdr, IPC_PACKET_HEADER_SIZE);

  if (bytes_written != IPC_PACKET_HEADER_SIZE) {
    return false;
  }

  bytes_written = write(fd, data.data(), data.size());

  return true;
}

/*---------------------------------------------------------------------------*/

SocketManager::SocketManager(const std::string &mgrName) : name_(mgrName) {}

SocketManager::~SocketManager() {
  if (pollfds_) {
    free(pollfds_);
    pollfds_ = nullptr;
  }
}

bool SocketManager::addSocketHandler(const SocketHandlerPtr &handler) {
  if (state_ != CONFIG) {
    std::cerr << "Unable to add socket due invalid state" << std::endl;
    return false;
  }
  auto newHandlerName = handler->name();
  bool result = false;

  // If the socket handler name is not found, add it to the vector
  if (recvHandlerMap_.find(newHandlerName) == recvHandlerMap_.end()) {
    recvHandlerMap_[newHandlerName] = handler;
    result = true;
  } else {
    std::cerr << "Unable to add socket due to duplication" << std::endl;
  }

  return result;
}

bool SocketManager::processInbound() {
  bool result = false;
  //  size_t fd_count = recvHandlerMap_.size();

  while (1) {
    int poll_count = poll(pollfds_, fd_count_, 0);

    if (poll_count == 0) {
      return true;
    } else if (poll_count < 0) {
      perror("Poll error");
      return false;
    }

    //    , "loop");

    size_t client_index;
    dataPacket data;

    for (size_t x = 0; x < fd_count_; x++) {
      if (pollfds_[x].revents & POLLIN) {
        auto fd = pollfds_[x].fd;

        auto handler = getSocketByFd(fd);

        if (handler->is_server(fd)) {
          auto new_fd = handler->accept();
          addFd(new_fd);
        } else if (handler->has_client(fd, client_index)) {
          if (handler->readPacket(client_index, data)) {
            handler->onReceivePacket(client_index, data);
          } else {
            ::close(pollfds_[x].fd);
            delFd(x);
          }
        } else {
          if (handler->readPacket(data)) {
            handler->onReceivePacket(data);
          } else {
            ::close(pollfds_[x].fd);
            delFd(x);
          }
        }
      }
    }
  }

  return result;
}

bool SocketManager::open() {
  assert(state_ != OPEN);

  allocate_pollfds();
  state_ = OPEN;

  return true;
}

void SocketManager::close() {
  if (pollfds_) {
    free(pollfds_);
    pollfds_ = nullptr;
  }
  state_ = CONFIG;
}

// bool SocketManager::connect()
//{
//  return false;
//}

SocketHandlerPtr SocketManager::getSocketByName(const std::string &name) {
  auto socketHandler = recvHandlerMap_.find(name);

  if (socketHandler != recvHandlerMap_.end()) {
    return recvHandlerMap_[name];
  }

  return SocketHandlerPtr();
}

SocketHandlerPtr SocketManager::getSocketByFd(int fd) {
  for (const auto &handler : recvHandlerMap_) {
    auto handler_fd = handler.second->fd();
    if (handler_fd == fd) {
      return handler.second;
    }
    auto socketHandler = handler.second;
    size_t i = 0;

    if (socketHandler->has_client(fd, i)) {
      return handler.second;
    }
  }

  return SocketHandlerPtr();
}

bool SocketManager::addSocketHandler(const std::string &name,
                                     const std::string &host, uint16_t port,
                                     bool server) {
  // LOG_MSG, true, __PRETTY_FUNCTION__);
  // LOG_VAR(name);
  // LOG_END;

  if (state_ != CONFIG) {
    std::cerr << "Unable to add socket due invalid state" << std::endl;
    return false;
  }
  bool result = false;

  // If the socket handler name is not found, add it to the vector
  if (recvHandlerMap_.find(name) == recvHandlerMap_.end()) {
    SocketHandlerPtr p =
        std::make_shared<SocketHandler>(name, host, port, server);
    if (p->isOpen()) {
      recvHandlerMap_[name] = p;
      result = true;
    }
  }

  return result;
}

void SocketManager::allocate_pollfds() {
  assert(state_ != OPEN);

  size_t numFds = recvHandlerMap_.size();

  pollfds_ = (pollfd *)malloc(numFds * sizeof(pollfd));

  for (const auto &handler_tuple : recvHandlerMap_) {
    [[maybe_unused]] auto name = handler_tuple.first;
    [[maybe_unused]] auto handler = handler_tuple.second;
    [[maybe_unused]] auto port = handler->port();
    auto fd = handler->fd();

    addFd(fd);
  }
}

void SocketManager::addFd(int newfd) {
  // If we don't have room, double the space in the pfds array
  if (fd_count_ == fd_size_) {
    fd_size_ *= 2;
    pollfds_ = (pollfd *)realloc(pollfds_, sizeof(pollfd) * fd_size_);
  }

  // Add fd to set
  // POLLIN - ready to read on incoming connection
  pollfds_[fd_count_].fd = newfd;
  pollfds_[fd_count_].events = POLLIN;

  fd_count_++;
}

void SocketManager::delFd(int i) {

  // Copy the one from the end over this one
  pollfds_[i] = pollfds_[fd_count_ - 1];

  fd_count_--;
}

bool SocketManager::addReceiveCB(const std::string &name, SocketReceivedCB cb) {
  bool result = false;
  // If the socket handler name is not found, add it to the vector
  if (recvHandlerMap_.find(name) != recvHandlerMap_.end()) {
    auto handler = recvHandlerMap_[name];
    handler->setReceiveCB(std::move(cb));
    result = true;
  }

  return result;
}

bool SocketManager::addConnectCB(const std::string &name, SocketConnectCB cb) {
  bool result = false;
  // If the socket handler name is not found, add it to the vector
  if (connHandlerMap_.find(name) != connHandlerMap_.end()) {
    auto handler = connHandlerMap_[name];
    handler->setConnectCB(std::move(cb));
    result = true;
  }

  return result;
}

bool SocketManager::addDisconnectCB(const std::string &name,
                                    SocketConnectCB cb) {
  bool result = false;
  // If the socket handler name is not found, add it to the vector
  if (discHandlerMap_.find(name) != discHandlerMap_.end()) {
    auto handler = discHandlerMap_[name];
    handler->setDisconnectCB(std::move(cb));
    result = true;
  }

  return result;
}
} // namespace net
} // namespace foo
