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

static bool isValidNickname(const std::string &nick) {
  if (nick.empty() || nick.length() > 32)
    return false;
  // special = ( [ \ ] ^ _ ` { | } )
  std::string special = "[]\\`_^{|}";
  if (!std::isalpha(nick[0]) && special.find(nick[0]) == std::string::npos)
    return false;
  for (size_t i = 1; i < nick.length(); ++i) {
    if (!std::isalnum(nick[i]) && special.find(nick[i]) == std::string::npos &&
        nick[i] != '-')
      return false;
  }
  return true;
}

/* ============================= */
/*       PASS COMMAND LOGIC      */
/* ============================= */

void CommandHandler::handlePASS(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  // No password argument
  if (cmd.params.empty()) {
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS(nick, "PASS"));
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
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    server->sendReply(client->getFd(), ERR_PASSWDMISMATCH(nick));
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
  std::string currentNick = client->getNickname().empty() ? "*" : client->getNickname();
  // Check if pass has been entered first
  if (!client->hasValidPass()) {
    server->sendReply(client->getFd(), ERR_PASSWDMISMATCH(currentNick));
    return;
  }

  if (cmd.params.empty()) {
    server->sendReply(client->getFd(), ERR_NONICKNAMEGIVEN(currentNick));
    return;
  }

  const std::string &nick = cmd.params[0];

  if (!isValidNickname(nick)) {
    server->sendReply(client->getFd(), ERR_ERRONEUSNICKNAME(currentNick, nick));
    return;
  }

  const std::string oldNick = client->getNickname();
  if (!oldNick.empty() && nick == oldNick)
    return;

  if (server->nicknameInUse(nick)) {
    server->sendReply(client->getFd(), ERR_NICKNAMEINUSE(currentNick, nick));
    return;
  }

  if (client->isAuthenticated() && !oldNick.empty()) {
    const std::string nickMsg = ":" + oldNick + "!" + client->getUsername() +
                               "@ircserv NICK " + nick + "\r\n";

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
  std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
  // Check if pass has been entered first
  if (!client->hasValidPass()) {
    server->sendReply(client->getFd(), ERR_PASSWDMISMATCH(nick));
    return;
  }

  if (cmd.params.size() < 3 || cmd.trailing.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS(nick, "USER"));
    return;
  }

  if (client->isAuthenticated()) {
    server->sendReply(client->getFd(), ERR_ALREADYREGISTRED(nick));
    return;
  }

  std::string username = cmd.params[0];
  if (username.length() > 16)
    username = username.substr(0, 16);

  client->setUsername(username);
  client->setRealname(cmd.trailing);

  server->tryRegister(client);
}

/* ============================= */
/*        CAP COMMAND LOGIC      */
/* ============================= */

void CommandHandler::handleCAP(Server *server, Client *client,
                               const ParsedCommand &cmd) {
  (void)server;
  // minimal CAP handling for clients

  if (cmd.params.empty())
    return;

  std::string sub = cmd.params[0];
  for (size_t i = 0; i < sub.size(); ++i)
    sub[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(sub[i])));

  if (sub == "LS") {
    std::string target = client->getNickname().empty() ? "*" : client->getNickname();
    // Empty caps list: trailing ':' with nothing following.
    server->sendReply(client->getFd(), std::string(":ircserv CAP ") + target + " LS :\r\n");
  }
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
  std::string reason = cmd.hasTrailing ? cmd.trailing : "Quit";
  std::string quitMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@ircserv QUIT :" + reason +
                        "\r\n";

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
  std::string nick = client->getNickname();
  // No target given
  if (cmd.params.empty()) {
    server->sendReply(client->getFd(), ERR_NORECIPIENT(nick, "PRIVMSG"));
    return;
  }

  std::string target = cmd.params[0];
  std::string text = cmd.trailing;

  // No text to send
  if (text.empty()) {
    server->sendReply(client->getFd(), ERR_NOTEXTTOSEND(nick));
    return;
  }

  /* ===== CHANNEL MESSAGE ===== */
  if (!target.empty() && target[0] == '#') {
    if (!server->_channels.count(target)) {
      server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(nick, target));
      return;
    }

    Channel *channel = server->_channels[target];

    if (!channel->hasClient(client)) {
      server->sendReply(client->getFd(), ERR_CANNOTSENDTOCHAN(nick, target));
      return;
    }

    std::string msg = ":" + client->getNickname() + "!" +
                      client->getUsername() + "@ircserv PRIVMSG " + target +
                      " :" + text + "\r\n";

    channel->broadcast(msg, client);
    return;
  }

  /* ===== DIRECT MESSAGE ===== */
  Client *receiver = server->getClientByNick(target);
  if (!receiver) {
    server->sendReply(client->getFd(), ERR_NOSUCHNICK(nick, target));
    return;
  }

  std::string msg = ":" + client->getNickname() + "!" + client->getUsername() +
                    "@ircserv PRIVMSG " + target + " :" + text + "\r\n";

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
                    "@ircserv NOTICE " + target + " :" + text + "\r\n";
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
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS(nick, "PING"));
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
                    RPL_WHOISUSER(client->getNickname(), target->getNickname(),
                                  target->getUsername(), "ircserv",
                                  target->getRealname()));

  std::string chanList;
  const std::vector<Channel *> &joinedChannels = target->getJoinedChannels();
  for (size_t i = 0; i < joinedChannels.size(); ++i) {
    if (i > 0)
      chanList += " ";
    chanList += joinedChannels[i]->getName();
  }

  // Reply with WHOISCHANNELS
  server->sendReply(client->getFd(),
                    RPL_WHOISCHANNELS(client->getNickname(),
                                      target->getNickname(), chanList));

  // End of WHOIS
  server->sendReply(client->getFd(),
                    RPL_ENDOFWHOIS(client->getNickname(),
                                   target->getNickname()));
}

/* ============================= */
/*             WHO               */
/* ============================= */

void CommandHandler::handleWHO(Server *server, Client *client,
                               const ParsedCommand &cmd) {
  if (!requireParams(server, client, cmd, 1, "WHO"))
    return;

  const std::string &mask = cmd.params[0];
  std::string nick = client->getNickname();
  if (!mask.empty() && mask[0] == '#') {
    if (!server->_channels.count(mask)) {
      server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(nick, mask));
      return;
    }
    Channel *channel = server->_channels[mask];
    const std::vector<Client *> &members = channel->getClients();
    for (size_t i = 0; i < members.size(); ++i) {
      Client *entry = members[i];
      server->sendReply(client->getFd(), RPL_WHOREPLY(nick, channel->getName(),
                        entry->getUsername(), "ircserv", "ircserv", entry->getNickname(), "H", entry->getRealname()));
    }
  } else {
    Client *target = server->getClientByNick(mask);
    if (!target) {
      server->sendReply(client->getFd(), ERR_NOSUCHNICK(nick, mask));
      return;
    }
    std::string chanName = "*";
    const std::vector<Channel *> &joined = target->getJoinedChannels();
    if (!joined.empty())
      chanName = joined[0]->getName();
    server->sendReply(
        client->getFd(),
        RPL_WHOREPLY(nick, chanName, target->getUsername(),
              "ircserv", "ircserv", target->getNickname(), "H", target->getRealname()));
  }
  server->sendReply(client->getFd(), RPL_ENDOFWHO(nick, mask));
}
