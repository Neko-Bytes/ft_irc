/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 03:07:07 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 03:07:08 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>

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
  Channel(const std::string &name);
  ~Channel();

  const std::string &getName() const;
  const std::vector<Client *> &getClients() const;

  /* ============================= */
  /*       MEMBER MANAGEMENT       */
  /* ============================= */

  void addClient(Client *client);
  void removeClient(Client *client);

  /* ============================= */
  /*          BROADCASTING         */
  /* ============================= */

  void broadcast(const std::string &msg, Client *exclude = NULL);

private:
  std::string _name;
  std::vector<Client *> _clients;
};

#endif
