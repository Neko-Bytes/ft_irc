/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 03:07:07 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/05 06:43:37 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
class Server;

class Client;

/**
 * @brief Represents an IRC channel and its member list.
 *
 * Steps:
 *  - Store channel name
 *  - Track clients inside the channel
 *  - Provide join/leave operations
 *  - Provide message broadcasting to channel users
 */
class Channel {
public:
  Channel(const std::string &name, Server *server);
  ~Channel();

  // getters
  const std::string &getName() const;
  const std::vector<Client *> &getClients() const;
  const std::vector<Client *> &getOperators() const;
  int getLimit() const;
  bool isTopicProtected() const;
  bool isInviteOnly() const;
  const std::string &getTopic() const;
  const std::string &getKey() const;
  bool hasKey() const;
  bool hasLimit() const;
  bool isFull() const;

  // setters
  void setLimit(int limit);
  void clearLimit();
  void setTopicProtected(bool value);
  void setInviteOnly(bool invite);
  void setTopic(const std::string &topic);
  void setKey(const std::string &key);
  void clearKey();


  /* ============================= */
  /*       MEMBER MANAGEMENT       */
  /* ============================= */

  void addClient(Client *client);
  bool hasClient(Client *client) const;
  void removeClient(Client *client);
  void inviteNickname(const std::string &nickname);
  bool isInvited(const std::string &nickname) const;
  void removeInvited(const std::string &nickname);
  void addOperator(Client *client);
  void removeOperator(Client *client);
  bool isOperator(Client *client) const;


  /* ============================= */
  /*          BROADCASTING         */
  /* ============================= */

  void broadcast(const std::string &msg, Client *exclude = NULL);

private:
  std::string _name;
  std::vector<Client *> _clients;
  std::vector<Client *> _operators;
  std::vector<std::string> _invited;
  Server *_server;
  bool _topicProtected;
  std::string _key;
  bool _inviteOnly;
  int _limit;
  std::string _topic;
};

#endif
