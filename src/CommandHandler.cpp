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
#include "../includes/CommandHandlerHelpers.hpp"
#include "../includes/Replies.hpp"
#include "../includes/Server.hpp"

#include <set>
#include <sstream>
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

  if (client->isAuthenticated()) {
    server->sendReply(client->getFd(),
                      ERR_ALREADYREGISTRED(client->getNickname()));
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
  // Check if pass has been entered first
  if (!client->hasValidPass()) {
    server->sendReply(client->getFd(),
                      ERR_PASSWDMISMATCH(client->getNickname().empty()
                                             ? "*"
                                             : client->getNickname()));
    return;
  }

  if (cmd.params.empty()) {
    server->sendReply(client->getFd(), ERR_NONICKNAMEGIVEN);
    return;
  }

  const std::string &nick = cmd.params[0];

  const std::string oldNick = client->getNickname();
  if (!oldNick.empty() && nick == oldNick)
    return;

  if (server->nicknameInUse(nick)) {
    server->sendReply(client->getFd(), ERR_NICKNAMEINUSE(nick));
    return;
  }

  if (client->isAuthenticated() && !oldNick.empty()) {
    const std::string nickMsg = ":" + oldNick + "!" + client->getUsername() +
                               "@localhost NICK " + nick + "\r\n";

    std::set<int> sentFds;
    const std::vector<Channel *> &joined = client->getJoinedChannels();
    for (size_t i = 0; i < joined.size(); ++i) {
      Channel *ch = joined[i];
      const std::vector<Client *> &members = ch->getClients();
      for (size_t j = 0; j < members.size(); ++j) {
        Client *dst = members[j];
        if (dst == client)
          continue;
        if (sentFds.insert(dst->getFd()).second)
          server->sendReply(dst->getFd(), nickMsg);
      }
    }

    server->sendReply(client->getFd(), nickMsg);

    for (std::map<std::string, Channel *>::iterator it = server->_channels.begin();
         it != server->_channels.end(); ++it) {
      Channel *channel = it->second;
      if (!channel)
        continue;
      if (channel->isInvited(oldNick)) {
        channel->removeInvited(oldNick);
        channel->inviteNickname(nick);
      }
    }
  }

  client->setNickname(nick);
  server->tryRegister(client);
}

/* ============================= */
/*       USER COMMAND LOGIC      */
/* ============================= */

void CommandHandler::handleUSER(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  // Check if pass has been entered first
  if (!client->hasValidPass()) {
    server->sendReply(client->getFd(),
                      ERR_PASSWDMISMATCH(client->getNickname().empty()
                                             ? "*"
                                             : client->getNickname()));
    return;
  }

  if (cmd.params.size() < 3 || cmd.trailing.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("USER"));
    return;
  }

  if (client->isAuthenticated()) {
    server->sendReply(client->getFd(),
                      ERR_ALREADYREGISTRED(client->getNickname()));
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

  // Broadcast QUIT
  for (size_t i = 0; i < joined.size(); i++) {
    Channel *ch = joined[i];
    ch->broadcast(quitMsg, client);
  }
  server->removeClient(client->getFd());
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

  std::string target = cmd.params[0];
  std::string text = cmd.trailing;

  // Allows msgs without colon. Ex: "PRIVMSG user hello world"
  // i = 1 to skip "user"
  // if (text.empty() && cmd.params.size() > 1) {
  //   for (size_t i = 1; i < cmd.params.size(); ++i) {
  //     if (i > 1)
  //       text += " ";
  //     text += cmd.params[i];
  //   }
  // }

  // No text to send
  if (text.empty()) {
    server->sendReply(client->getFd(), ":ircserver 412 :No text to send\r\n");
    return;
  }

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
/*            NOTICE             */
/* ============================= */

void CommandHandler::handleNOTICE(Server *server, Client *client,
                                  const ParsedCommand &cmd) {
  if (cmd.params.empty())
    return;

  std::string target = cmd.params[0];
  std::string text = cmd.trailing;
  if (text.empty())
    return;
  std::string msg = ":" + client->getNickname() + "!" + client->getUsername() + 
                    "@localhost NOTICE " + target + " :" + text + "\r\n";
  if (!target.empty() && target[0] == '#') {
    if (!server->_channels.count(target))
      return;
    Channel *channel = server->_channels[target];
    if (!channel->hasClient(client))
      return;
    channel->broadcast(msg, client);
    return;
  }
  Client *receiver = server->getClientByNick(target);
  if (!receiver)
    return;
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
/*         WHOIS LOGIC           */
/* ============================= */

void CommandHandler::handleWHOIS(Server *server, Client *client,
                                 const ParsedCommand &cmd) {
  if (!requireParams(server, client, cmd, 1, "WHOIS"))
    return;

  std::string targetNick = cmd.params[0];
  Client *target = resolveClientOrReply(server, client, targetNick);
  if (!target)
    return;

  server->sendReply(client->getFd(),
                    RPL_WHOISUSER(target->getNickname(), target->getUsername(),
                                  "localhost", target->getRealname()));

  std::string chanList;
  const std::vector<Channel *> &joinedChannels = target->getJoinedChannels();
  for (size_t i = 0; i < joinedChannels.size(); ++i) {
    if (i > 0)
      chanList += " ";
    chanList += joinedChannels[i]->getName();
  }

  // Reply with WHOISCHANNELS
  server->sendReply(client->getFd(),
                    RPL_WHOISCHANNELS(target->getNickname(), chanList));

  // End of WHOIS
  server->sendReply(client->getFd(), RPL_ENDOFWHOIS(target->getNickname()));
}

/* ============================= */
/*             WHO               */
/* ============================= */

void CommandHandler::handleWHO(Server *server, Client *client,
                               const ParsedCommand &cmd) {
  if (!requireParams(server, client, cmd, 1, "WHO"))
    return;

  const std::string &mask = cmd.params[0];
  if (!mask.empty() && mask[0] == '#') {
    if (!server->_channels.count(mask)) {
      server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(mask));
      return;
    }
    Channel *channel = server->_channels[mask];
    const std::vector<Client *> &members = channel->getClients();
    for (size_t i = 0; i < members.size(); ++i) {
      Client *entry = members[i];
      server->sendReply(client->getFd(), RPL_WHOREPLY(client->getNickname(), channel->getName(),
                        entry->getUsername(), "localhost", "ircserver", entry->getNickname(), "H", entry->getRealname()));
    }
  } else {
    Client *target = server->getClientByNick(mask);
    if (!target) {
      server->sendReply(client->getFd(), ERR_NOSUCHNICK(mask));
      return;
    }
    std::string chanName = "*";
    const std::vector<Channel *> &joined = target->getJoinedChannels();
    if (!joined.empty())
      chanName = joined[0]->getName();
    server->sendReply(
        client->getFd(),
        RPL_WHOREPLY(client->getNickname(), chanName, target->getUsername(),
                      "localhost", "ircserver", target->getNickname(), "H", target->getRealname()));
  }
  server->sendReply(client->getFd(), RPL_ENDOFWHO(client->getNickname(), mask));
}
