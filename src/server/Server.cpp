/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 16:47:38 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/05 07:43:32 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * @file Server.cpp
 * @brief Implementation of the Server class handling socket setup, polling,
 *        accepting new clients, and reading client data.
 */

#include "../../includes/Server.hpp"
#include "../../includes/Channel.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/CommandHandler.hpp"
#include "../../includes/Parser.hpp"
#include "../../includes/Replies.hpp"

#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
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

  int yes = 1; // Enable and Disable switch
  setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  // non-blocking I/O for poll-based event loop
  fcntl(_listenFd, F_SETFL, O_NONBLOCK);

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(std::atoi(_port.c_str()));

  if (bind(_listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    throw std::runtime_error("bind() failed");

  // SOMAXCONN = max number of queued pending connections (system-dependent)
  if (listen(_listenFd, SOMAXCONN) < 0)
    throw std::runtime_error("listen() failed");

  addPollFd(_listenFd);
}

/* ============================= */
/*           MAIN LOOP           */
/* ============================= */

/**
 * @brief Main poll loop handling all socket activity.
 *
 * poll() watches an array of file descriptors and sets the 'revents' field
 * when something happens (e.g. data is ready to read). When poll() returns,
 * we scan the vector to see which fd triggered the event.
 */
void Server::mainLoop() {
  while (true) {
        // Refresh POLL events based on pending output
    for (size_t i = 0; i < _pollfds.size(); i++) {
      if (_pollfds[i].fd == _listenFd) {
        _pollfds[i].events = POLLIN;
        continue;
      }

      std::map<int, Client *>::iterator it = _clients.find(_pollfds[i].fd);
      if (it != _clients.end() && it->second->hasPendingSend())
        _pollfds[i].events = POLLIN | POLLOUT;
      else
        _pollfds[i].events = POLLIN;
    }
    if (_pollfds.empty())
      throw std::runtime_error("No fds to poll");

    int ret = poll(&_pollfds[0], _pollfds.size(), -1);
    if (ret < 0)
      throw std::runtime_error("poll() failed");

    for (size_t i = 0; i < _pollfds.size(); i++) {
      if (_pollfds[i].fd == _listenFd && (_pollfds[i].revents & POLLIN)) {
        acceptNewClient();
      }
      // 2. Client Operations
      else {
        int fd = _pollfds[i].fd;
        if (_clients.count(_pollfds[i].fd)) {
          Client *client = _clients[fd];

          // READ (Incoming)
          if (_pollfds[i].revents & POLLIN) {
            if (!handleClientRead(i)) {
              --i;      // Client removed, stay at this index
              continue; // Don't try to write to a dead client
            }
          }

          // WRITE (Outgoing)
          if (_pollfds[i].revents & POLLOUT) {
            while (client->hasPendingSend()) {
              const char *data = NULL;
              size_t len = 0;
              if (!client->peekOutputSlice(data, len) || len == 0) {
                client->clearOutputBuffer();
                break;
              }

              ssize_t sent = send(fd, data, len, MSG_NOSIGNAL);
              if (sent > 0) {
                client->consumeBytes(static_cast<size_t>(sent));
                if (static_cast<size_t>(sent) < len)
                  break; // partial write, wait for next POLLOUT
              } else if (sent == 0) {
                removeClient(fd);
                --i;
                break;
              } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                  break;
                removeClient(fd);
                --i;
                break;
              }
            }
          }
        }
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
/*       MESSAGE EXTRACTION      */
/* ============================= */

/**
 * @brief Extracts complete IRC messages from a client's buffer.
 *
 * Splits on "\r\n" and returns each line as a separate IRC command.
 * Any partial data at the end is left in the buffer.
 */
std::vector<std::string> Server::extractMessages(Client *client) {
  std::vector<std::string> messages;
  std::string &buffer = client->getBufferRef();

  while (true) {
    // 1. Find the newline
    size_t pos = buffer.find('\n');
    if (pos == std::string::npos)
      break;

    // 2. Extract the message (substring before \n)
    std::string msg = buffer.substr(0, pos);

    // 3. Handle optional Carriage Return (\r)
    // If the char before \n is \r, remove it.
    if (!msg.empty() && msg[msg.length() - 1] == '\r') {
      msg.erase(msg.length() - 1);
    }

    messages.push_back(msg);

    // 4. Remove processed line from buffer (pos + 1 to include the \n)
    buffer.erase(0, pos + 1);
  }
  return messages;
}

/* ============================= */
/*     COMMAND DISPATCHING       */
/* ============================= */

/**
 * @brief Parses and dispatches an IRC command.
 * Enforces registration: before PASS/NICK/USER are done, most commands
 * return ERR_NOTREGISTERED.
 */
void Server::handleCommand(Client *client, const std::string &msg) {
  ParsedCommand cmd = Parser::parse(msg);

  // Normalize command name to uppercase
  std::string name = cmd.command;
  for (size_t i = 0; i < name.size(); i++) {
    name[i] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(name[i])));
  }

  // Commands that are allowed even if the client is not fully registered
  bool alwaysAllowed = (name == "PASS" || name == "NICK" || name == "USER" ||
                        name == "PING" || name == "PONG" || name == "QUIT");

  // Block everything else until registration is complete
  if (!alwaysAllowed && !client->isAuthenticated()) {
    sendReply(client->getFd(), ERR_NOTREGISTERED);
    return;
  }

  if (name == "PASS")
    CommandHandler::handlePASS(this, client, cmd);
  else if (name == "NICK")
    CommandHandler::handleNICK(this, client, cmd);
  else if (name == "USER")
    CommandHandler::handleUSER(this, client, cmd);
  else if (name == "JOIN")
    CommandHandler::handleJOIN(this, client, cmd);
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
  else if (name == "MODE")
    CommandHandler::handleMODE(this, client, cmd);
  else if (name == "TOPIC")
    CommandHandler::handleTOPIC(this, client, cmd);
  else if (name == "INVITE")
    CommandHandler::handleINVITE(this, client, cmd);
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

/**
 * @brief Sends the initial welcome numeric to a fully registered client.
 */
void Server::sendWelcome(Client *client) {
  sendReply(client->getFd(), RPL_WELCOME(client->getNickname()));
}

/**
 * @brief Tries to complete client registration.
 *
 * If PASS, NICK, USER, REALNAME are all set and the client is not yet
 * authenticated, mark them as authenticated and send welcome.
 */
void Server::tryRegister(Client *client) {
  if (client->isAuthenticated())
    return;

  if (!isClientFullyRegistered(client))
    return;

  client->setAuthenticated(true);
  sendWelcome(client);
}

/* ============================= */
/*          LOW-LEVEL I/O        */
/* ============================= */

/**
 * @brief Sends a raw IRC reply to a socket.
 *
 * This is a small helper around send() used by the reply macros.
 */
void Server::sendReply(int fd, const std::string &msg) {
  std::map<int, Client *>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    queueMessage(it->second, msg);
    return;
  }
  // Fallback for early replies before a Client object is tracked
  send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
}

/**
 * @brief Queues a message for deferred sending via poll-driven writes.
 */
void Server::queueMessage(Client *client, const std::string &msg) {
  if (!client || msg.empty())
    return;
  client->queueMessage(msg);
}
