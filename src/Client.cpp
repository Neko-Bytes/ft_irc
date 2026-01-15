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
#include "../includes/Constants.hpp"
#include <algorithm>

/**
 * @brief Constructs a Client instance for the given socket fd.
 * Initializes nickname, username, buffer, and authentication state.
 */

Client::Client(int fd)
  : _fd(fd), _nickname(""), _username(""), _realname(""), _authenticated(false), _hasValidPass(false), _buffer(""), _inputOverflow(false), _outputBufferSize(0), _outputBuffer(), _outputOffset(0) {}
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
const std::deque<std::string> &Client::getoutputBuffer() const { return _outputBuffer; }
size_t Client::getOutputBufferSize() const { return _outputBufferSize; }
bool Client::hasInputOverflow() const { return _inputOverflow; }
size_t Client::getOutputOffset() const { return _outputOffset; }

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
void Client::appendToBuffer(const std::string &data) {
  if (data.empty() || _inputOverflow)
    return;
  if (_buffer.size() >= IRC::MaxInputBufferBytes) {
    _inputOverflow = true;
    return;
  }
  const size_t spaceLeft = IRC::MaxInputBufferBytes - _buffer.size();
  if (data.size() > spaceLeft) {
    _buffer.append(data, 0, spaceLeft);
    _inputOverflow = true;
    return;
  }
  _buffer += data;
}

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

  // cut off at 512 bytes and add \r\n
  std::string line = data;
  size_t cut = line.find_first_of("\r\n");
  if (cut != std::string::npos)
    line.erase(cut);
  if (line.size() > IRC::MaxIrcPayloadBytes)
    line.erase(IRC::MaxIrcPayloadBytes);
  line += "\r\n";
  if (line.size() > IRC::MaxIrcLineBytes)
    line.erase(IRC::MaxIrcLineBytes);
  if (_outputBufferSize + line.size() > IRC::MaxOutputBufferBytes)
    return;

  _outputBuffer.push_back(line);
  _outputBufferSize += line.size();
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
  _outputOffset = 0;
}

/**
 * @brief Peeks at the next message to be sent without removing it.
 * @return The next message in the output buffer, or an empty string if none.
 */
std::string Client::peekOutputBuffer() const {
  if (_outputBuffer.empty())
    return "";
  const std::string &front = _outputBuffer.front();
  if (_outputOffset >= front.size())
    return "";
  return front.substr(_outputOffset);
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
    if (_outputOffset >= front.size()) {
      _outputOffset = 0;
      _outputBuffer.pop_front();
      continue;
    }

    const size_t remaining = front.size() - _outputOffset;
    if (remaining <= localBytes) {
      localBytes -= remaining;
      _outputBufferSize -= remaining;
      _outputBuffer.pop_front();
      _outputOffset = 0;
    } else {
      _outputOffset += localBytes;
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
