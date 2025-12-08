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
    : _fd(fd), _nickname(""), _username(""), _realname(""), _authenticated(false), _hasValidPass(false), _buffer(""), _outputBufferSize(0), _outputBuffer() {}
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
std::deque<std::string> Client::getoutputBuffer() const { return _outputBuffer; }
int Client::getOutputBufferSize() const { return _outputBufferSize; }

/* ============================= */
/*           SETTERS             */
/* ============================= */

void Client::setNickname(const std::string &nick) { _nickname = nick; }
void Client::setUsername(const std::string &user) { _username = user; }
void Client::setRealname(const std::string &real) { _realname = real; }
void Client::setAuthenticated(bool status) { _authenticated = status; }
void Client::setValidPass(bool status) { _hasValidPass = status; }

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

/**
 * @brief Queues a message to be sent to the client.
 */
void Client::queueMessage(const std::string &data) {
  if (data.empty())
    return;
  _outputBuffer.push_back(data);
  _outputBufferSize += data.size();
}
/**
 * @brief Checks if there are pending messages to send.
 */
bool Client::hasPendingSend() const { return !_outputBuffer.empty(); }

/**
 * @brief Clears all queued messages in the output buffer.
 */
void Client::clearOutputBuffer() {
  _outputBuffer.clear();
  _outputBufferSize = 0;
}

/**
 * @brief Peeks at the next message to be sent without removing it.
 * @return The next message in the output buffer, or an empty string if none.
 */
std::string Client::peekOutputBuffer() const {
  if (_outputBuffer.empty())
    return "";
  return _outputBuffer.front();
}
/**
 * @brief Peeks at the message at a specific offset in the output buffer.
 * @param offset The offset index to peek at.
 */

/**
 * @brief updates the total size of the output buffer and removes bytes that have been sent.
 * @param bytes Number of bytes to consume from the output buffer.
 * -steps:
 * - Iterate through the output buffer deque
 * - Remove bytes from the front strings until the requested number is consumed
 * - Adjust the total output buffer size accordingly
 */
void Client::consumeBytes(size_t bytes) {
  size_t localBytes = bytes;

  while (localBytes > 0 && !_outputBuffer.empty()) {
    std::string &front = _outputBuffer.front();
    if (front.size() <= localBytes) {
      localBytes -= front.size();
      _outputBufferSize -= front.size();
      _outputBuffer.pop_front();
    } else {
      front.erase(0, localBytes);
      _outputBufferSize -= localBytes;
      localBytes = 0;
    }
  }
}

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
