/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelHelpers.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 06:20:43 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/05 07:44:56 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Channel.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Server.hpp"

/* ============================= */
/*        CHANNEL HELPERS        */
/* ============================= */

/**
 * @brief Retrieves an existing channel or creates a new one.
 *
 * Steps:
 *  - Look for channel name in the _channels map
 *  - If not found, create a new Channel object
 *  - Return the channel pointer
 */
Channel *Server::getOrCreateChannel(const std::string &name) {
  if (_channels.count(name))
    return _channels[name];

  Channel *ch = new Channel(name);
  _channels[name] = ch;
  return ch;
}

/**
 * @brief Deletes a channel if it becomes empty.
 *
 * Steps:
 *  - Check if the channel exists
 *  - If it has zero members, delete it and remove it from the map
 */
void Server::cleanupChannel(std::string name) {
  if (!_channels.count(name))
    return;

  Channel *ch = _channels[name];
  if (!ch)
    return;

  if (ch->getClients().empty()) {
    ch->clearInvites();
    delete ch;
    _channels.erase(name);
  }
}

/**
 * @brief Finds a client by their nickname.
 *
 * Steps:
 *  - Iterate through all connected clients
 *  - Compare nickname with the given nick
 *
 * @param nick Nickname to search for.
 * @return Client* Pointer if found, NULL otherwise.
 */
Client *Server::getClientByNick(const std::string &nick) const {
  for (std::map<int, Client *>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second->getNickname() == nick)
      return it->second;
  }
  return NULL;
}

void Server::removeInvitesForNick(const std::string &nick) {
  for (std::map<std::string, Channel *>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    Channel *channel = it->second;
    if (channel)
      channel->removeInvited(nick);
  }
}
