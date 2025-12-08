/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 01:47:17 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 02:58:12 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * @file Client.cpp
 * @brief Implementation of the Client class used to track per-user IRC state.
 */

#include "../includes/Client.hpp"
#include "../includes/Channel.hpp"
#include <algorithm>

/**
 * @brief Constructs a Client instance for the given socket fd.
 * Initializes nickname, username, buffer, and authentication state.
 */

Client::Client(int fd)
    : _fd(fd), _nickname(""), _username(""), _realname(""), _authenticated(false), _hasValidPass(false), _buffer(""), _sendQueue("") {}
/**
 * @brief Destructor. No special cleanup required here.
 * Channel removal and server-side cleanup is handled by Server.
 */
Client::~Client() {}

/* ============================= */
/*           GETTERS             */
/* ============================= */

int Client::getFd() const { return _fd; }
const std::string &Client::getNickname() const { return _nickname; }
const std::string &Client::getUsername() const { return _username; }
const std::string &Client::getRealname() const { return _realname; }
const std::string &Client::getBuffer() const { return _buffer; }
bool Client::isAuthenticated() const { return _authenticated; }
std::string &Client::getBufferRef() { return _buffer; }
bool Client::hasValidPass() const { return _hasValidPass; }
std::string Client::getSendQueue() const { return _sendQueue; }

/* ============================= */
/*           SETTERS             */
/* ============================= */

void Client::setNickname(const std::string &nick) { _nickname = nick; }
void Client::setUsername(const std::string &user) { _username = user; }
void Client::setRealname(const std::string &real) { _realname = real; }
void Client::setAuthenticated(bool status) { _authenticated = status; }
void Client::setValidPass(bool status) { _hasValidPass = status; }
void Client::setSendQueue(const std::string &data) { _sendQueue = data; }

/* ============================= */
/*         BUFFER HANDLING       */
/* ============================= */

/**
 * @brief Appends raw incoming data to the client's buffer.
 * Used to accumulate partial TCP fragments until a full IRC command is formed.
 */
void Client::appendToBuffer(const std::string &data) { _buffer += data; }

/**
 * @brief Clears the buffer once all complete IRC commands have been processed.
 */
void Client::clearBuffer() { _buffer.clear(); }

void Client::queueMessage(const std::string &data) { _sendQueue += data; }
bool Client::hasPendingSend() const { return !_sendQueue.empty(); }

/* ============================= */
/*       CHANNEL MANAGEMENT      */
/* ============================= */

/**
 * @brief Adds the client to a channel if not already present.
 */
void Client::joinChannel(Channel *channel) {
  if (std::find(_joined.begin(), _joined.end(), channel) == _joined.end())
    _joined.push_back(channel);
}

/**
 * @brief Removes the client from a channel if they are a member.
 */
void Client::leaveChannel(Channel *channel) {
  std::vector<Channel *>::iterator it =
      std::find(_joined.begin(), _joined.end(), channel);

  if (it != _joined.end())
    _joined.erase(it);
}

/**
 * @brief Returns a reference to the list of channels the client has joined.
 */
const std::vector<Channel *> &Client::getJoinedChannels() const {
  return _joined;
}
