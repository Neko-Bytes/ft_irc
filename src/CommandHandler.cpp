/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 02:37:22 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/05 07:09:53 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * @file CommandHandler.cpp
 * @brief High-level IRC command handling (PASS, NICK, USER, JOIN, PART,
 * PRIVMSG, PING, PONG, KICK, QUIT).
 */

#include "../includes/CommandHandler.hpp"
#include "../includes/Channel.hpp"
#include "../includes/Replies.hpp"
#include "../includes/Server.hpp"

#include <sys/socket.h>

/* ============================= */
/*       PASS COMMAND LOGIC      */
/* ============================= */

void CommandHandler::handlePASS(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  // No password argument
  if (cmd.params.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("PASS"));
    return;
  }

  const std::string &pass = cmd.params[0];

  // Wrong password
  if (pass != server->getPassword()) {
    server->sendReply(client->getFd(),
                      ":ircserver 464 * :Password incorrect\r\n");
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
    server->sendReply(client->getFd(), ERR_NONICKNAMEGIVEN);
    return;
  }

  const std::string &nick = cmd.params[0];

  if (server->nicknameInUse(nick)) {
    server->sendReply(client->getFd(), ERR_NICKNAMEINUSE(nick));
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
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("USER"));
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
 */
void CommandHandler::handleQUIT(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  (void)cmd;

  std::string quitMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@localhost QUIT :Quit\r\n";

  const std::vector<Channel *> &joined = client->getJoinedChannels();

  // Broadcast QUIT and remove client from all channels
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
 *  - Add the client to the channel (if not already in)
 *  - Broadcast JOIN to other members
 *  - Send NAMES list (353)
 *  - Send end of NAMES (366)
 */
void CommandHandler::handleJOIN(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("JOIN"));
    return;
  }

  std::string chanName = cmd.params[0];
  if (chanName[0] != '#')
    chanName = "#" + chanName;

  Channel *channel = server->getOrCreateChannel(chanName);

  // If already in channel, nothing to do
  if (channel->hasClient(client))
    return;

  channel->addClient(client);
  client->joinChannel(channel);

  // Broadcast JOIN to channel
  std::string joinMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@localhost JOIN " + chanName +
                        "\r\n";
  channel->broadcast(joinMsg, NULL);

  // Build NAMES list
  const std::vector<Client *> &members = channel->getClients();

  std::string names =
      ":ircserver 353 " + client->getNickname() + " = " + chanName + " :";

  for (size_t i = 0; i < members.size(); i++) {
    names += members[i]->getNickname();
    if (i + 1 < members.size())
      names += " ";
  }
  names += "\r\n";

  server->sendReply(client->getFd(), names);

  std::string endMsg = ":ircserver 366 " + client->getNickname() + " " +
                       chanName + " :End of NAMES list\r\n";

  server->sendReply(client->getFd(), endMsg);
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
 *  - Check if the client is on that channel
 *  - Remove client from the channel
 *  - Broadcast PART message
 *  - Delete channel if empty
 */
void CommandHandler::handlePART(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("PART"));
    return;
  }

  std::string chanName = cmd.params[0];
  if (chanName[0] != '#')
    chanName = "#" + chanName;

  if (!server->_channels.count(chanName)) {
    server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(chanName));
    return;
  }

  Channel *channel = server->_channels[chanName];

  if (!channel->hasClient(client)) {
    server->sendReply(client->getFd(), ERR_NOTONCHANNEL(chanName));
    return;
  }

  channel->removeClient(client);
  client->leaveChannel(channel);

  std::string partMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@localhost PART " + chanName +
                        "\r\n";

  channel->broadcast(partMsg, NULL);

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
 *  - Use numeric replies instead of disconnecting on error
 */
void CommandHandler::handlePRIVMSG(Server *server, Client *client,
                                   const ParsedCommand &cmd) {
  // No target given
  if (cmd.params.empty()) {
    server->sendReply(client->getFd(),
                      ":ircserver 411 :No recipient given (PRIVMSG)\r\n");
    return;
  }

  // No text to send
  if (cmd.trailing.empty()) {
    server->sendReply(client->getFd(), ":ircserver 412 :No text to send\r\n");
    return;
  }

  std::string target = cmd.params[0];
  std::string text = cmd.trailing;

  /* ===== CHANNEL MESSAGE ===== */
  if (!target.empty() && target[0] == '#') {
    if (!server->_channels.count(target)) {
      server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(target));
      return;
    }

    Channel *channel = server->_channels[target];

    if (!channel->hasClient(client)) {
      server->sendReply(client->getFd(), ERR_CANNOTSENDTOCHAN(target));
      return;
    }

    std::string msg = ":" + client->getNickname() + "!" +
                      client->getUsername() + "@localhost PRIVMSG " + target +
                      " :" + text + "\r\n";

    channel->broadcast(msg, client);
    return;
  }

  /* ===== DIRECT MESSAGE ===== */
  Client *receiver = server->getClientByNick(target);
  if (!receiver) {
    server->sendReply(client->getFd(), ERR_NOSUCHNICK(target));
    return;
  }

  std::string msg = ":" + client->getNickname() + "!" + client->getUsername() +
                    "@localhost PRIVMSG " + target + " :" + text + "\r\n";

  server->sendReply(receiver->getFd(), msg);
}

/* ============================= */
/*         PING / PONG           */
/* ============================= */

void CommandHandler::handlePING(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("PING"));
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
  (void)client; // no operator checks for now (simple version)

  if (cmd.params.size() < 2) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("KICK"));
    return;
  }

  std::string chanName = cmd.params[0];
  std::string targetNick = cmd.params[1];

  if (chanName[0] != '#')
    chanName = "#" + chanName;

  if (!server->_channels.count(chanName)) {
    server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(chanName));
    return;
  }

  Channel *channel = server->_channels[chanName];
  Client *target = server->getClientByNick(targetNick);

  if (!target) {
    server->sendReply(client->getFd(), ERR_NOSUCHNICK(targetNick));
    return;
  }

  if (!channel->hasClient(target)) {
    server->sendReply(client->getFd(), ERR_NOTONCHANNEL(chanName));
    return;
  }

  std::string kickMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@localhost KICK " + chanName +
                        " " + targetNick + "\r\n";

  channel->broadcast(kickMsg, NULL);

  channel->removeClient(target);
  target->leaveChannel(channel);

  server->cleanupChannel(chanName);
}
