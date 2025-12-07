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
#include <sstream>

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

// prefix check and creation

std::string ensureChannelPrefix(const std::string &name) {
  if (name.empty())
    return name;
  if (name[0] != '#')
    return "#" + name;
  return name;
}

std::string makePrefix(Client *client) {
  return ":" + client->getNickname() + "!" + client->getUsername() +
         "@localhost";
}

// invite command needed for invite only channels

void CommandHandler::handleINVITE(Server *server, Client *client,
                                  const ParsedCommand &cmd) {
  if (cmd.params.size() < 2) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("INVITE"));
    return;
  }

  std::string targetNick = cmd.params[0];
  std::string chanName = ensureChannelPrefix(cmd.params[1]);

  if (chanName.empty() || !server->_channels.count(chanName)) {
    server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(chanName));
    return;
  }

  Channel *channel = server->_channels[chanName];

  if (!channel->hasClient(client)) {
    server->sendReply(client->getFd(), ERR_NOTONCHANNEL(chanName));
    return;
  }

  Client *target = server->getClientByNick(targetNick);
  if (!target) {
    server->sendReply(client->getFd(), ERR_NOSUCHNICK(targetNick));
    return;
  }
  if (!channel->isOperator(client)) {
    server->sendReply(client->getFd(), ERR_CHANOPRIVSNEEDED(chanName));
    return;
  }
  channel->inviteNickname(targetNick);


  std::string inviteMsg = makePrefix(client) + " INVITE " + targetNick +
                          " " + chanName + "\r\n";
  server->sendReply(target->getFd(), inviteMsg);
  server->sendReply(client->getFd(), RPL_INVITING(targetNick, chanName));
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

  std::string channelstr = cmd.params[0];
  std::string key;
  if (cmd.params.size() > 1)
    key = cmd.params[1];
  if (channelstr.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("JOIN"));
    return;
  }

  std::string prefix = makePrefix(client);

  //rewrite for single channel join with checks
    std::string chanName = ensureChannelPrefix(channelstr);
    if (chanName.empty())
      return;

    Channel *channel = server->getOrCreateChannel(chanName);
    if (channel->hasKey() && key != channel->getKey()) {
      server->sendReply(client->getFd(), ERR_BADCHANNELKEY(chanName));
      return;
    }
    if (channel->isInviteOnly() && !channel->isInvited(client->getNickname()) &&
        !channel->isOperator(client)) {
      server->sendReply(client->getFd(), ERR_INVITEONLYCHAN(chanName));
      return;
    }
    if (channel->hasLimit() && channel->isFull() &&
        !channel->isOperator(client)) {
      server->sendReply(client->getFd(), ERR_CHANNELISFULL(chanName));
      return ;
    }
    if (channel->hasClient(client)) {
      return;
    }

    channel->addClient(client);
    client->joinChannel(channel);
    channel->removeInvited(client->getNickname());

    if (channel->getClients().size() == 1) {
      channel->addOperator(client);
    }

    std::string joinMsg = prefix + " JOIN " + chanName + "\r\n";
    channel->broadcast(joinMsg, NULL);

    const std::vector<Client *> &members = channel->getClients();
    std::string names = ":ircserver 353 " + client->getNickname() + " = " +
                        chanName + " :";
    for (size_t i = 0; i < members.size(); ++i) {
      names += members[i]->getNickname();
      if (i + 1 < members.size())
        names += " ";
    }
    names += "\r\n";
    server->sendReply(client->getFd(), names);

    std::string endMsg = ":ircserver 366 " +
                         client->getNickname() + " " + chanName +
                         " :End of NAMES list\r\n";
    server->sendReply(client->getFd(), endMsg);
}

// handle mode
void CommandHandler::handleMODE(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("MODE"));
    return;
  }

  std::string chanName = ensureChannelPrefix(cmd.params[0]);
  std::string mode = cmd.params.size() >= 2 ? cmd.params[1] : "";

  if (chanName.empty() || !server->_channels.count(chanName)) {
    server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(chanName));
    return;
  }

  Channel *channel = server->_channels[chanName];

  if (!channel->hasClient(client)) {
    server->sendReply(client->getFd(), ERR_NOTONCHANNEL(chanName));
    return;
  }
  if (mode.empty()) {
    std::string modes = "+";
    if (channel->isInviteOnly())
      modes += "i";
    if (channel->isTopicProtected())
      modes += "t";
    if (channel->hasKey())
      modes += "k";
    if (channel->hasLimit())
      modes += "l";

    std::string args;
    if (channel->hasKey())
      args += " " + channel->getKey();
    if (channel->hasLimit()) {
      std::stringstream ss;
      ss << channel->getLimit();
      args += " " + ss.str();
    }

    server->sendReply(client->getFd(),
                      RPL_CHANNELMODEIS(client->getNickname(), chanName,
                                        modes + args));
    return;
  }

  if (!channel->isOperator(client)) {
    server->sendReply(client->getFd(), ERR_CHANOPRIVSNEEDED(chanName));
    return;
  }

  std::string target;
  if (cmd.params.size() >= 3)
    target = cmd.params[2];
  std::string prefix = makePrefix(client);
  std::string modeMsg;

  if (mode == "+o") {
    if (target.empty()) {
      server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("MODE"));
      return;
    }
    Client *targetClient = server->getClientByNick(target);
    if (!targetClient) {
      server->sendReply(client->getFd(), ERR_NOSUCHNICK(target));
      return;
    }
    channel->addOperator(targetClient);
    modeMsg = prefix + " MODE " + chanName + " +o " + target + "\r\n";
  } else if (mode == "-o") {
    if (target.empty()) {
      server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("MODE"));
      return;
    }
    Client *targetClient = server->getClientByNick(target);
    if (!targetClient) {
      server->sendReply(client->getFd(), ERR_NOSUCHNICK(target));
      return;
    }
    channel->removeOperator(targetClient);
    modeMsg = prefix + " MODE " + chanName + " -o " + target + "\r\n";
  } else if (mode == "+k") {
    if (target.empty()) {
      server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("MODE"));
      return;
    }
    channel->setKey(target);
    modeMsg = prefix + " MODE " + chanName + " +k " + target + "\r\n";
  } else if (mode == "-k") {
    channel->clearKey();
    modeMsg = prefix + " MODE " + chanName + " -k\r\n";
  } else if (mode == "+i") {
    channel->setInviteOnly(true);
    modeMsg = prefix + " MODE " + chanName + " +i\r\n";
  } else if (mode == "-i") {
    channel->setInviteOnly(false);
    modeMsg = prefix + " MODE " + chanName + " -i\r\n";
  } else if (mode == "+l") {
    if (target.empty()) {
      server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("MODE"));
      return;
    }
    int limit = std::atoi(target.c_str());
    if (limit <= 0) {
      server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("MODE"));
      return;
    }
    channel->setLimit(limit);
    modeMsg = prefix + " MODE " + chanName + " +l " + target + "\r\n";
  } else if (mode == "-l") {
    channel->clearLimit();
    modeMsg = prefix + " MODE " + chanName + " -l\r\n";
  } else {
    server->sendReply(client->getFd(),
                      prefix + " MODE " + chanName + " " + mode + "\r\n");
    return;
  }

  channel->broadcast(modeMsg, NULL);
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

  if (!channel->isOperator(client)) {
    server->sendReply(client->getFd(), ERR_CHANOPRIVSNEEDED(chanName));
    return;
  }

  if (!channel->hasClient(client)) {
    server->sendReply(client->getFd(), ERR_NOTONCHANNEL(chanName));
    return;
  }

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
