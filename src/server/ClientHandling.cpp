/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientHandling.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 18:43:23 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 18:44:39 by kmummadi         ###   ########.fr       */
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
  removePollFd(fd);

  if (_clients.count(fd)) {
    delete _clients[fd];
    _clients.erase(fd);
  }

  close(fd);

  std::cout << "Client disconnected: fd " << fd << std::endl;
}
