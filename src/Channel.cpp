/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 03:07:30 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/05 06:43:57 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * @file Channel.cpp
 * @brief Implementation of the Channel class used to manage IRC channels.
 */

#include "../includes/Channel.hpp"
#include "../includes/Client.hpp"
#include "../includes/Server.hpp"

#include <algorithm>
#include <sys/socket.h>

/* ============================= */
/*          CONSTRUCTION         */
/* ============================= */

Channel::Channel(const std::string &name)
    : _name(name), _topicProtected(false),
      _key(), _inviteOnly(false), _limit(0) {}

Channel::~Channel() {}

/* ============================= */
/*           GETTERS             */
/* ============================= */

const std::string &Channel::getName() const { return _name; }

const std::vector<Client *> &Channel::getClients() const { return _clients; }

const std::vector<Client *> &Channel::getOperators() const { return _operators; }

int Channel::getLimit() const { return _limit; }

bool Channel::isTopicProtected() const { return _topicProtected; }

bool Channel::isInviteOnly() const { return _inviteOnly; }

const std::string &Channel::getTopic() const { return _topic; }

const std::string &Channel::getKey() const { return _key; }

bool Channel::hasKey() const { return !_key.empty(); }

bool Channel::hasLimit() const { return _limit > 0; }

bool Channel::isFull() const {
  return hasLimit() && _clients.size() >= static_cast<size_t>(_limit);
}

// setters

void Channel::setLimit(int limit) { _limit = limit > 0 ? limit : 0; }

void Channel::clearLimit() { _limit = 0; }

void Channel::setTopicProtected(bool value) { _topicProtected = value; }

void Channel::setInviteOnly(bool invite) { _inviteOnly = invite; }

void Channel::setTopic(const std::string &topic) { _topic = topic; }

void Channel::setKey(const std::string &key) { _key = key; }

void Channel::clearKey() { _key.clear(); }

/* ============================= */
/*       MEMBER MANAGEMENT       */
/* ============================= */

void Channel::addClient(Client *client) {
  if (std::find(_clients.begin(), _clients.end(), client) == _clients.end())
    _clients.push_back(client);
}

bool Channel::hasClient(Client *client) const {
  const std::vector<Client *> &members = _clients;
  return std::find(members.begin(), members.end(), client) != members.end();
}

void Channel::removeClient(Client *client) {
  std::vector<Client *>::iterator it =
      std::find(_clients.begin(), _clients.end(), client);

  if (it != _clients.end())
    _clients.erase(it);

  removeOperator(client);
  removeInvited(client->getNickname());
}

void Channel::inviteNickname(const std::string &nickname) {
  if (std::find(_invited.begin(), _invited.end(), nickname) == _invited.end())
    _invited.push_back(nickname);
}

bool Channel::isInvited(const std::string &nickname) const {
  return std::find(_invited.begin(), _invited.end(), nickname) != _invited.end();
}

void Channel::removeInvited(const std::string &nickname) {
  std::vector<std::string>::iterator it =
      std::find(_invited.begin(), _invited.end(), nickname);
  if (it != _invited.end())
    _invited.erase(it);
}

void Channel::addOperator(Client *client) {
  if (std::find(_operators.begin(), _operators.end(), client) ==
      _operators.end()) {
    _operators.push_back(client);
  }
}

void Channel::removeOperator(Client *client) {
  std::vector<Client *>::iterator it =
      std::find(_operators.begin(), _operators.end(), client);

  if (it != _operators.end())
    _operators.erase(it);
}

bool Channel::isOperator(Client *client) const {
  return std::find(_operators.begin(), _operators.end(), client) !=
         _operators.end();
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
