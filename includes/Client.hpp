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
  std::string getSendQueue() const;

  // Setters
  void setNickname(const std::string &nick);
  void setUsername(const std::string &user);
  void setRealname(const std::string &real);
  void setAuthenticated(bool status);
  void setValidPass(bool status);
  void setSendQueue(const std::string &data);

  // Buffer handling
  void appendToBuffer(const std::string &data);
  void clearBuffer();
  void queueMessage(const std::string &data);
  bool hasPendingSend() const;

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
  std::string _sendQueue;         // stores outgoing messages
  std::vector<Channel *> _joined; // channels the client is in
};

#endif
