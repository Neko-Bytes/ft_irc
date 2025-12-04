/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 03:07:30 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 03:07:31 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * @file Channel.cpp
 * @brief Implementation of the Channel class used to manage IRC channels.
 */

#include "../includes/Channel.hpp"
#include "../includes/Client.hpp"

#include <algorithm>
#include <sys/socket.h>

/* ============================= */
/*          CONSTRUCTION         */
/* ============================= */

Channel::Channel(const std::string &name) : _name(name) {}

Channel::~Channel() {}

/* ============================= */
/*           GETTERS             */
/* ============================= */

const std::string &Channel::getName() const { return _name; }

const std::vector<Client *> &Channel::getClients() const { return _clients; }

/* ============================= */
/*       MEMBER MANAGEMENT       */
/* ============================= */

void Channel::addClient(Client *client) {
  if (std::find(_clients.begin(), _clients.end(), client) == _clients.end())
    _clients.push_back(client);
}

void Channel::removeClient(Client *client) {
  std::vector<Client *>::iterator it =
      std::find(_clients.begin(), _clients.end(), client);

  if (it != _clients.end())
    _clients.erase(it);
}

/* ============================= */
/*          BROADCASTING         */
/* ============================= */

void Channel::broadcast(const std::string &msg, Client *exclude) {
  for (size_t i = 0; i < _clients.size(); i++) {
    if (_clients[i] == exclude)
      continue;

    int fd = _clients[i]->getFd();
    send(fd, msg.c_str(), msg.size(), 0);
  }
}
