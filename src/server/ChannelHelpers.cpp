/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelHelpers.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qhahn <qhahn@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 06:20:43 by kmummadi          #+#    #+#             */
/*   Updated: 2026/01/23 18:42:37 by qhahn            ###   ########.fr       */
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
  std::string lowerName = toLowerCase(name);
  if (_channels.count(lowerName))
    return _channels[lowerName];

  Channel *ch = new Channel(name);
  _channels[lowerName] = ch;
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
  std::string lowerName = toLowerCase(name);
  if (!_channels.count(lowerName))
    return;

  Channel *ch = _channels[lowerName];
  if (!ch)
    return;

  if (ch->getClients().empty()) {
    ch->clearInvites();
    delete ch;
    _channels.erase(lowerName);
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
  std::string lowerNick = toLowerCase(nick);
  for (std::map<int, Client *>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (toLowerCase(it->second->getNickname()) == lowerNick)
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
