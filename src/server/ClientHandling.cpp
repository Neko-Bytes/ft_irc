/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientHandling.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qhahn <qhahn@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 18:43:23 by kmummadi          #+#    #+#             */
/*   Updated: 2026/01/23 18:43:52 by qhahn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ============================= */
/*        CLIENT HANDLING        */
/* ============================= */

#include "../../includes/Channel.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/CommandHandler.hpp"
#include "../../includes/Parser.hpp"
#include "../../includes/Server.hpp"
#include "../../includes/Constants.hpp"

#include <vector>

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

  // extract IP of the client
  char ipStr[INET_ADDRSTRLEN];
  // converts binary ip data into human readable string
  inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);

  // LOG CONNECTION
  Logger::logConnect(clientFd, ipStr, ntohs(clientAddr.sin_port));

  // Add client to list
  _clients[clientFd] = new Client(clientFd);

  addPollFd(clientFd);

  // std::cout << "Client connected: fd " << clientFd << std::endl;
}

/**
 * @brief Reads data from a client and dispatches commands.
 */
bool Server::handleClientRead(int index) {
  int fd = _pollfds[index].fd;
  char buffer[IRC::ReadBufferBytes];

  int bytes = recv(fd, buffer, sizeof(buffer), 0);
  if (bytes <= 0) {
    if (bytes < 0 &&
        (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
      return true;
    removeClient(fd, "Connection closed by peer");
    return (false);
  }

  Client *c = _clients[fd];
  c->appendToBuffer(std::string(buffer, bytes));
  if (c->hasInputOverflow()) {
    removeClient(fd, "Input overflow");
    return (false);
  }

  std::vector<std::string> msgs = extractMessages(c);
  for (size_t i = 0; i < msgs.size(); i++) {
    handleCommand(c, msgs[i]);

    // Say client uses QUIT and he doesn't exist
    if (_clients.find(fd) == _clients.end()) {
      return (false);
    }
  }

  return (true);
}

/**
 * @brief Removes a client from the server.
 */
void Server::removeClient(int fd, const std::string &reason) {
  // LOG DISCONNECTION
  Logger::logDisconnect(fd, reason);

  if (_clients.count(fd)) {
    std::string nick = _clients[fd]->getNickname();
    disconnectClientFromChannels(fd, reason);
    if (!nick.empty())
      removeInvitesForNick(nick);
    delete _clients[fd];
    _clients.erase(fd);
  }
  removePollFd(fd);
  close(fd);

  // std::cout << "Client disconnected: fd " << fd << std::endl;
}

/**
 * @brief Removes clients from all channels that they are in
 * and remove any empty channels.
 * Why?
 * Because the first person to join the channel becomes the
 * operator. When he exits, channel doesn't have an operator.
 * So even if another person joins in, he will not be the operator.
 * Also it is a wise method to save memory.
 */
void Server::disconnectClientFromChannels(int fd, const std::string &reason) {
  if (!_clients.count(fd))
    return;

  Client *client = _clients[fd];
  std::vector<Channel *> channels = client->getJoinedChannels();
  
  std::string quitMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@ircserv QUIT :" + reason + "\r\n";

  for (size_t i = 0; i < channels.size(); ++i) {
    Channel *ch = channels[i];
    if (ch) {
        ch->broadcast(quitMsg, client);
        ch->removeClient(client);
        cleanupChannel(ch->getName());
    }
  }
}
