/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientHandling.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 18:43:23 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/12 07:44:15 by kmummadi         ###   ########.fr       */
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
  char buffer[1024];

  int bytes = recv(fd, buffer, sizeof(buffer), 0);
  if (bytes <= 0) {
    removeClient(fd);
    return (false);
  }

  Client *c = _clients[fd];
  c->appendToBuffer(std::string(buffer, bytes));

  std::vector<std::string> msgs = extractMessages(c);
  for (size_t i = 0; i < msgs.size(); i++) {
    handleCommand(c, msgs[i]);
  }

  return (true);
}

/**
 * @brief Removes a client from the server.
 */
void Server::removeClient(int fd) {
  // Remove from all channels first
  disconnectClientFromChannels(fd);

  // Remove from poll
  removePollFd(fd);

  // LOG DISCONNECTION
  Logger::logDisconnect(fd, "Connection closed by peer or quit");

  if (_clients.count(fd)) {
    std::string nick = _clients[fd]->getNickname();
    disconnectClientFromChannels(fd);
    if (!nick.empty())
      removeInvitesForNick(nick);
    delete _clients[fd];
    _clients.erase(fd);
  }
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
void Server::disconnectClientFromChannels(int fd) {
  if (!_clients.count(fd))
    return;

  Client *client = _clients[fd];

  // Iterate thru channels
  std::map<std::string, Channel *>::iterator it = _channels.begin();
  while (it != _channels.end()) {
    Channel *channel = it->second;

    if (channel->hasClient(client)) {
      // Client found in the channel
      // Remove the client from the channel
      channel->removeClient(client);

      // If channel is empty, delete it
      if (channel->getClients().empty()) {
        delete channel;
        // get the next valid iterator first and then erase prev
        std::map<std::string, Channel *>::iterator to_erase = it;
        ++it;
        _channels.erase(to_erase);
      } else {
        // Just move to next channel
        ++it;
      }
    } else {
      // Client wasn't found in the channel
      // move to next one
      ++it;
    }
  }
}
