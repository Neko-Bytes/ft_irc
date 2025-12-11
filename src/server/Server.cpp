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

/* ================================ */
/*          SIGNAL HANDLING         */
/* ================================ */

/* @brief
 * A static variable belongs to the class itself and not specifically
 * to an object. So it is essentially a global variable but under Server
 * namespace (which is why _signal is used as static bool here).
 * This can be accessed by any function using Server::_signal.
 */

bool Server::_signal = false;

/* @brief
 * This signal handler will be called by OS whenever a signal input is detected.
 * Why static again?
 * In C++, a normal (non-static) member function like void Server::myHandler(int
 * sig) actually has two arguments. The compiler secretly rewrites it to: void
 * Server::myHandler(Server* this, int sig). Since this would not be accepted by
 * signal(), we use static to state that this method is independent of object.
 */
void Server::signalHandler(int signum) {
  (void)signum; // Silence unused warning
  std::cout << "\nSignal received! Shutting down..." << std::endl;
  Server::_signal = true; // Flip the switch
}

/* ============================= */
/*          CONSTRUCTION         */
/* ============================= */

/**
 * @brief Constructs the Server object with the given port and password.
 */
Server::Server(const std::string &port, const std::string &password)
    : _port(port), _password(password), _listenFd(-1) {}

/**
 * @brief Destructor cleans all client and channel maps and closes the server
 * socket.
 */
Server::~Server() {
  // 1. Close all client sockets and free memory
  for (std::map<int, Client *>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    close(it->first);  // Close the socket
    delete it->second; // Free the Client object
  }
  _clients.clear();

  // 2. Free all Channel objects
  for (std::map<std::string, Channel *>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    delete it->second;
  }
  _channels.clear();

  // 3. Close the listener socket
  if (_listenFd != -1) {
    close(_listenFd);
  }

  std::cout << "Server shutdown: All resources freed." << std::endl;
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
  while (_signal == false) {
    // === PHASE 1: PREPARE POLLFDS ===
    // Update events flags based on buffer status
    for (size_t i = 0; i < _pollfds.size(); i++) {
      if (_pollfds[i].fd == _listenFd)
        continue; // Listener is read-only

      if (_clients.count(_pollfds[i].fd)) {
        Client *c = _clients[_pollfds[i].fd];
        if (c->hasPendingSend())
          _pollfds[i].events = POLLIN | POLLOUT; // Needs to write
        else
          _pollfds[i].events = POLLIN; // Only reading
      }
    }

    // === PHASE 2: WAIT ===
    if (poll(_pollfds.data(), _pollfds.size(), -1) < 0) {
      if (_signal)
        break;
      throw std::runtime_error("poll() failed");
    }

    // === PHASE 3: PROCESS ===
    for (size_t i = 0; i < _pollfds.size(); i++) {
      // 1. Listener
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
            std::string msg = client->peekOutputBuffer();
            ssize_t sent = send(fd, msg.c_str(), msg.size(), 0);
            if (sent > 0)
              client->consumeBytes(sent);
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
  else if (name == "WHOIS")
    CommandHandler::handleWHOIS(this, client, cmd);
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
