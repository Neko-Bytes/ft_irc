/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 01:44:45 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 02:25:53 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <deque>

class Channel; // forward declaration

class Client {
public:
  // Construct a client using its socket file descriptor
  Client(int fd);
  ~Client();

  // Getters
  int getFd() const;
  const std::string &getNickname() const;
  const std::string &getUsername() const;
  const std::string &getRealname() const;
  const std::string &getBuffer() const;
  std::string &getBufferRef();
  bool isAuthenticated() const;
  bool hasValidPass() const;
  std::deque<std::string> getoutputBuffer() const;
  int getOutputBufferSize() const;

  // Setters
  void setNickname(const std::string &nick);
  void setUsername(const std::string &user);
  void setRealname(const std::string &real);
  void setAuthenticated(bool status);
  void setValidPass(bool status);
  
  // Buffer handling
  void appendToBuffer(const std::string &data);
  void clearBuffer();
  
  // outputBuffer handling
  /**
   * @brief Manages the output buffer for sending data to the client.
   * - queueMessage(data): adds data to output buffer and updates size
   * - hasPendingSend(): checks if there is data to send
   * 
   * - peekOutputBuffer(): peeks at next message without removing
   * - consumeBytes(n): removes n bytes from front of buffer and updates size
   * - getOutputBufferSize(): gets total size of output buffer
   * 
   * - clearOutputBuffer(): clears all queued messages
   * 
   *  how to use in server:
   *  - while client->hasPendingSend():
   *   - send(client->peekOutputBuffer())
   *   - client->consumeBytes(bytes_sent)
   */

  void queueMessage(const std::string &data);
  bool hasPendingSend() const;
  void clearOutputBuffer();
  void consumeBytes(size_t bytes);
  std::string peekOutputBuffer() const;

  // Channel tracking (used later)
  void joinChannel(Channel *channel);
  void leaveChannel(Channel *channel);
  const std::vector<Channel *> &getJoinedChannels() const;

private:
  int _fd; // socket fd for this client
  std::string _nickname;
  std::string _username;
  std::string _realname;
  bool _authenticated; // true after PASS+NICK+USER
  bool _hasValidPass;

  std::string _buffer;            // stores partial packets
  std::deque<std::string> _outputBuffer;         // stores outgoing messages
  int _outputBufferSize; // total size of _outputBuffer
  std::vector<Channel *> _joined; // channels the client is in
};

#endif
