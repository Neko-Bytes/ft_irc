/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 16:47:38 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 03:29:18 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * @file Server.cpp
 * @brief Implementation of the Server class handling socket setup, polling,
 *        accepting new clients, and reading client data.
 */

#include "../includes/Server.hpp"
#include "../includes/Channel.hpp"
#include "../includes/Client.hpp"
#include "../includes/CommandHandler.hpp"
#include "../includes/Parser.hpp"

#include <arpa/inet.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

/* ============================= */
/*          CONSTRUCTION         */
/* ============================= */

/**
 * @brief Constructs the Server object with the given port and password.
 */
Server::Server(const std::string &port, const std::string &password)
    : _port(port), _password(password), _listenFd(-1) {}

/**
 * @brief Destructor closes the listening socket if it's still open.
 */
Server::~Server() {
  if (_listenFd != -1)
    close(_listenFd);
}

/**
 * @brief Starts the IRC server.
 *
 * Steps:
 *  - Initialize listening socket
 *  - Enter main poll loop
 */
void Server::run() {
  initSocket();
  mainLoop();
}

/* ============================= */
/*         BASIC GETTERS         */
/* ============================= */

/**
 * @brief Returns the server password.
 */
const std::string &Server::getPassword() const { return _password; }

/* ============================= */
/*         SOCKET SETUP          */
/* ============================= */

/**
 * @brief Initializes the listening socket.
 *
 * Steps:
 *  - Create IPv4 TCP socket
 *  - Enable SO_REUSEADDR
 *  - Set non-blocking mode
 *  - Bind to the configured port
 *  - Listen for connections
 *  - Add to poll list
 */
void Server::initSocket() {
  _listenFd = socket(AF_INET, SOCK_STREAM, 0);
  if (_listenFd < 0)
    throw std::runtime_error("socket() failed");

  int yes = 1;
  setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  fcntl(_listenFd, F_SETFL, O_NONBLOCK);

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(std::atoi(_port.c_str()));

  if (bind(_listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    throw std::runtime_error("bind() failed");

  if (listen(_listenFd, SOMAXCONN) < 0)
    throw std::runtime_error("listen() failed");

  addPollFd(_listenFd);
}

/* ============================= */
/*           MAIN LOOP           */
/* ============================= */

/**
 * @brief Main poll loop handling all socket activity.
 */
void Server::mainLoop() {
  while (true) {
    if (_pollfds.empty())
      throw std::runtime_error("No fds to poll");

    int ret = poll(&_pollfds[0], _pollfds.size(), -1);
    if (ret < 0)
      throw std::runtime_error("poll() failed");

    for (size_t i = 0; i < _pollfds.size(); i++) {
      if (_pollfds[i].fd == _listenFd && (_pollfds[i].revents & POLLIN)) {
        acceptNewClient();
      } else if (_pollfds[i].revents & POLLIN) {
        handleClientRead(i);
      }
    }
  }
}

/* ============================= */
/*       POLL FD MANAGEMENT      */
/* ============================= */

/**
 * @brief Adds a file descriptor to poll monitoring.
 */
void Server::addPollFd(int fd) {
  pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  _pollfds.push_back(pfd);
}

/**
 * @brief Removes a file descriptor from poll monitoring.
 */
void Server::removePollFd(int fd) {
  for (size_t i = 0; i < _pollfds.size(); ++i) {
    if (_pollfds[i].fd == fd) {
      _pollfds.erase(_pollfds.begin() + i);
      break;
    }
  }
}

/* ============================= */
/*        CLIENT HANDLING        */
/* ============================= */

/**
 * @brief Accepts a new client connection.
 */
void Server::acceptNewClient() {
  sockaddr_in clientAddr;
  socklen_t len = sizeof(clientAddr);

  int clientFd = accept(_listenFd, (sockaddr *)&clientAddr, &len);
  if (clientFd < 0)
    return;

  fcntl(clientFd, F_SETFL, O_NONBLOCK);

  _clients[clientFd] = new Client(clientFd);

  addPollFd(clientFd);

  std::cout << "Client connected: fd " << clientFd << std::endl;
}

/**
 * @brief Reads data from a client and dispatches commands.
 */
void Server::handleClientRead(int index) {
  int fd = _pollfds[index].fd;
  char buffer[1024];

  int bytes = recv(fd, buffer, sizeof(buffer), 0);
  if (bytes <= 0) {
    removeClient(fd);
    return;
  }

  Client *c = _clients[fd];
  c->appendToBuffer(std::string(buffer, bytes));

  std::vector<std::string> msgs = extractMessages(c);
  for (size_t i = 0; i < msgs.size(); i++) {
    handleCommand(c, msgs[i]);
  }
}

/**
 * @brief Removes a client from the server.
 */
void Server::removeClient(int fd) {
  removePollFd(fd);

  if (_clients.count(fd)) {
    delete _clients[fd];
    _clients.erase(fd);
  }

  close(fd);

  std::cout << "Client disconnected: fd " << fd << std::endl;
}

/* ============================= */
/*       MESSAGE EXTRACTION      */
/* ============================= */

/**
 * @brief Extracts complete IRC messages from a client's buffer.
 */
std::vector<std::string> Server::extractMessages(Client *client) {
  std::vector<std::string> messages;
  std::string &buffer = client->getBufferRef();

  size_t pos;
  while ((pos = buffer.find("\r\n")) != std::string::npos) {
    messages.push_back(buffer.substr(0, pos));
    buffer.erase(0, pos + 2);
  }
  return messages;
}

/* ============================= */
/*     COMMAND DISPATCHING       */
/* ============================= */

/**
 * @brief Parses and dispatches an IRC command.
 */
void Server::handleCommand(Client *client, const std::string &msg) {
  ParsedCommand cmd = Parser::parse(msg);

  std::string name = cmd.command;
  for (size_t i = 0; i < name.size(); i++)
    name[i] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(name[i])));

  if (name == "PASS")
    CommandHandler::handlePASS(this, client, cmd);
  else if (name == "NICK")
    CommandHandler::handleNICK(this, client, cmd);
  else if (name == "USER")
    CommandHandler::handleUSER(this, client, cmd);
  else if (name == "PART")
    CommandHandler::handlePART(this, client, cmd);
  else if (name == "PRIVMSG")
    CommandHandler::handlePRIVMSG(this, client, cmd);
  else if (name == "PING")
    CommandHandler::handlePING(this, client, cmd);
  else if (name == "PONG")
    CommandHandler::handlePONG(this, client, cmd);
  else if (name == "KICK")
    CommandHandler::handleKICK(this, client, cmd);
  else if (name == "QUIT")
    CommandHandler::handleQUIT(this, client, cmd);
}

/* ============================= */
/*      REGISTRATION HELPERS     */
/* ============================= */

bool Server::nicknameInUse(const std::string &nick) const {
  for (std::map<int, Client *>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second->getNickname() == nick)
      return true;
  }
  return false;
}

bool Server::isClientFullyRegistered(Client *client) const {
  if (!client->hasValidPass())
    return false;
  if (client->getNickname().empty())
    return false;
  if (client->getUsername().empty())
    return false;
  if (client->getRealname().empty())
    return false;
  return true;
}

void Server::sendWelcome(Client *client) {
  int fd = client->getFd();
  std::string nick = client->getNickname();

  std::string msg =
      ":ircserver 001 " + nick + " :Welcome to the IRC server\r\n";

  send(fd, msg.c_str(), msg.size(), 0);
}

void Server::tryRegister(Client *client) {
  if (client->isAuthenticated())
    return;

  if (!isClientFullyRegistered(client))
    return;

  client->setAuthenticated(true);
  sendWelcome(client);
}

/* ============================= */
/*        CHANNEL HELPERS        */
/* ============================= */

/**
 * @brief Retrieves an existing channel or creates a new one.
 *
 * Steps:
 *  - Look for channel name in the _channels map
 *  - If not found, create a new Channel object
 *  - Return the channel pointer
 */
Channel *Server::getOrCreateChannel(const std::string &name) {
  if (_channels.count(name))
    return _channels[name];

  Channel *ch = new Channel(name);
  _channels[name] = ch;
  return ch;
}

/**
 * @brief Deletes a channel if it becomes empty.
 *
 * Steps:
 *  - Check if the channel exists
 *  - If it has zero members, delete it and remove it from the map
 */
void Server::cleanupChannel(const std::string &name) {
  if (!_channels.count(name))
    return;

  Channel *ch = _channels[name];
  if (ch->getClients().empty()) {
    delete ch;
    _channels.erase(name);
  }
}

/**
 * @brief Finds a client by their nickname.
 *
 * Steps:
 *  - Iterate through all connected clients
 *  - Compare nickname with the given nick
 *
 * @param nick Nickname to search for.
 * @return Client* Pointer if found, NULL otherwise.
 */
Client *Server::getClientByNick(const std::string &nick) const {
  for (std::map<int, Client *>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second->getNickname() == nick)
      return it->second;
  }
  return NULL;
}

void Server::sendReply(int fd, const std::string &msg) {
  send(fd, msg.c_str(), msg.size(), 0);
}
