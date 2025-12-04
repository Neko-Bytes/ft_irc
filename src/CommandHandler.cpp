/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 02:37:22 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 03:28:33 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/CommandHandler.hpp"
#include "../includes/Channel.hpp"
#include "../includes/Server.hpp"

#include <algorithm>
#include <sys/socket.h>

/* ============================= */
/*       PASS COMMAND LOGIC      */
/* ============================= */

void CommandHandler::handlePASS(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->removeClient(client->getFd());
    return;
  }

  const std::string &pass = cmd.params[0];

  if (pass != server->getPassword()) {
    server->removeClient(client->getFd());
    return;
  }

  client->setValidPass(true);
  server->tryRegister(client);
}

/* ============================= */
/*       NICK COMMAND LOGIC      */
/* ============================= */

void CommandHandler::handleNICK(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->removeClient(client->getFd());
    return;
  }

  const std::string &nick = cmd.params[0];

  if (server->nicknameInUse(nick)) {
    server->removeClient(client->getFd());
    return;
  }

  client->setNickname(nick);
  server->tryRegister(client);
}

/* ============================= */
/*       USER COMMAND LOGIC      */
/* ============================= */

void CommandHandler::handleUSER(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.size() < 3 || cmd.trailing.empty()) {
    server->removeClient(client->getFd());
    return;
  }

  client->setUsername(cmd.params[0]);
  client->setRealname(cmd.trailing);

  server->tryRegister(client);
}

/* ============================= */
/*        QUIT COMMAND LOGIC     */
/* ============================= */

/**
 * @brief Handles QUIT, removes client from all channels,
 * broadcasts QUIT message, then disconnects.
 *
 * Steps:
 *  - Build QUIT message
 *  - Broadcast to all joined channels
 *  - Remove client from every channel
 *  - Trigger server->removeClient()
 */
void CommandHandler::handleQUIT(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  (void)cmd;

  std::string quitMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@localhost QUIT :Quit\r\n";

  const std::vector<Channel *> &joined = client->getJoinedChannels();

  for (size_t i = 0; i < joined.size(); i++) {
    Channel *ch = joined[i];
    ch->broadcast(quitMsg, client);
    ch->removeClient(client);
    server->cleanupChannel(ch->getName());
  }

  server->removeClient(client->getFd());
}

/* ============================= */
/*        JOIN COMMAND LOGIC     */
/* ============================= */

/**
 * @brief Processes the JOIN command.
 *
 * Steps:
 *  - Ensure a channel name parameter exists
 *  - Retrieve or create the channel
 *  - Add the client to the channel
 *  - Broadcast JOIN to other members
 *  - Send NAMES list (353)
 *  - Send end of NAMES (366)
 */
void CommandHandler::handleJOIN(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->removeClient(client->getFd());
    return;
  }

  std::string chanName = cmd.params[0];

  // IRC channels must begin with '#'
  if (chanName[0] != '#')
    chanName = "#" + chanName;

  Channel *channel = server->getOrCreateChannel(chanName);

  // Add user to channel
  channel->addClient(client);

  // Broadcast JOIN message to all channel members
  std::string joinMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@localhost JOIN " + chanName +
                        "\r\n";

  channel->broadcast(joinMsg, NULL);

  // Send NAMES list numeric (353)
  std::string names =
      ":ircserver 353 " + client->getNickname() + " = " + chanName + " :";

  const std::vector<Client *> &members = channel->getClients();
  for (size_t i = 0; i < members.size(); i++) {
    names += members[i]->getNickname();
    if (i + 1 < members.size())
      names += " ";
  }
  names += "\r\n";

  send(client->getFd(), names.c_str(), names.size(), 0);

  // End of NAMES (366)
  std::string endMsg = ":ircserver 366 " + client->getNickname() + " " +
                       chanName + " :End of NAMES list\r\n";

  send(client->getFd(), endMsg.c_str(), endMsg.size(), 0);
}

/* ============================= */
/*        PART COMMAND LOGIC     */
/* ============================= */

/**
 * @brief Processes the PART command.
 *
 * Steps:
 *  - Ensure a channel name parameter exists
 *  - Check if the channel exists in the server
 *  - Remove client from the channel
 *  - Broadcast PART message to other users
 *  - Delete channel if empty
 */
void CommandHandler::handlePART(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->removeClient(client->getFd());
    return;
  }

  std::string chanName = cmd.params[0];
  if (chanName[0] != '#')
    chanName = "#" + chanName;

  // Channel must exist
  if (!server->_channels.count(chanName)) {
    server->removeClient(client->getFd());
    return;
  }

  Channel *channel = server->_channels[chanName];

  // Remove from channel
  channel->removeClient(client);

  // Broadcast PART
  std::string partMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@localhost PART " + chanName +
                        "\r\n";

  channel->broadcast(partMsg, NULL);

  // Delete empty channels
  server->cleanupChannel(chanName);
}

/* ============================= */
/*      PRIVMSG COMMAND LOGIC    */
/* ============================= */

/**
 * @brief Processes the PRIVMSG command.
 *
 * Steps:
 *  - Ensure target and message are provided
 *  - If target is channel (#), send to all members except sender
 *  - Otherwise, treat as nickname and send directly to user
 */
void CommandHandler::handlePRIVMSG(Server *server, Client *client,
                                   const ParsedCommand &cmd) {
  if (cmd.params.empty() || cmd.trailing.empty()) {
    server->removeClient(client->getFd());
    return;
  }

  std::string target = cmd.params[0];
  std::string message = cmd.trailing;

  /* ========== CHANNEL MESSAGE ========== */
  if (target[0] == '#') {
    if (!server->_channels.count(target)) {
      server->removeClient(client->getFd());
      return;
    }

    Channel *channel = server->_channels[target];

    // Must be a member of the channel
    const std::vector<Client *> &members = channel->getClients();
    if (std::find(members.begin(), members.end(), client) == members.end()) {
      server->removeClient(client->getFd());
      return;
    }

    std::string msg = ":" + client->getNickname() + "!" +
                      client->getUsername() + "@localhost PRIVMSG " + target +
                      " :" + message + "\r\n";

    channel->broadcast(msg, client);
    return;
  }

  /* ========== USER-TO-USER MESSAGE ========== */
  Client *receiver = server->getClientByNick(target);
  if (!receiver) {
    server->removeClient(client->getFd());
    return;
  }

  std::string msg = ":" + client->getNickname() + "!" + client->getUsername() +
                    "@localhost PRIVMSG " + target + " :" + message + "\r\n";

  send(receiver->getFd(), msg.c_str(), msg.size(), 0);
}

/* ============================= */
/*         PING / PONG           */
/* ============================= */

void CommandHandler::handlePING(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->removeClient(client->getFd());
    return;
  }

  std::string pong = "PONG :" + cmd.params[0] + "\r\n";
  server->sendReply(client->getFd(), pong);
}

void CommandHandler::handlePONG(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  (void)server;
  (void)client;
  (void)cmd;
}

/* ============================= */
/*          KICK LOGIC           */
/* ============================= */

/**
 * @brief Removes a user from a channel.
 *
 * Steps:
 *  - Validate channel + target nickname
 *  - Verify channel exists
 *  - Verify target is in channel
 *  - Broadcast KICK to channel
 *  - Remove user
 *  - Cleanup empty channel
 */
void CommandHandler::handleKICK(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.size() < 2) {
    server->removeClient(client->getFd());
    return;
  }

  std::string chanName = cmd.params[0];
  std::string targetNick = cmd.params[1];

  if (chanName[0] != '#')
    chanName = "#" + chanName;

  if (!server->_channels.count(chanName)) {
    server->removeClient(client->getFd());
    return;
  }

  Channel *channel = server->_channels[chanName];
  Client *target = server->getClientByNick(targetNick);

  if (!target) {
    server->removeClient(client->getFd());
    return;
  }

  const std::vector<Client *> &members = channel->getClients();
  if (std::find(members.begin(), members.end(), target) == members.end()) {
    server->removeClient(client->getFd());
    return;
  }

  std::string kickMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@localhost KICK " + chanName +
                        " " + targetNick + "\r\n";

  channel->broadcast(kickMsg, NULL);

  channel->removeClient(target);
  server->cleanupChannel(chanName);
}
