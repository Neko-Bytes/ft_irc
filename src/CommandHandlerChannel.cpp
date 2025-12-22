#include "../includes/CommandHandler.hpp"

#include "../includes/Channel.hpp"
#include "../includes/Replies.hpp"
#include "../includes/Server.hpp"
#include "../includes/CommandHandlerHelpers.hpp"
#include <sys/socket.h>
#include <sstream>

/**
 * @brief Processes the INVITE command.
 * Steps:
 * - Validate parameters
 * - Check channel existence and membership
 * - Check operator privileges
 * - Invite target user and send notifications
 */
void CommandHandler::handleINVITE(Server *server, Client *client,
                                  const ParsedCommand &cmd) {
  if (!requireParams(server, client, cmd, 2, "INVITE"))
    return;

  std::string targetNick = cmd.params[0];
  Channel *channel =
      expectChannel(server, client, cmd.params[1], "INVITE", true, true, true);
  if (!channel)
    return;

  Client *target = resolveClientOrReply(server, client, targetNick);
  if (!target)
    return;
  
  std::string nick = client->getNickname();
  if (!channel->hasClient(client)) {
    server->sendReply(client->getFd(), ERR_NOTONCHANNEL(nick, channel->getName()));
    return;
  }
  if (channel->isInviteOnly() && !channel->isOperator(client)) {
    server->sendReply(client->getFd(), ERR_CHANOPRIVSNEEDED(nick, channel->getName()));
    return;
  }
  if (channel->hasClient(target)) {
    server->sendReply(client->getFd(), ERR_USERONCHANNEL(nick, targetNick, channel->getName()));
    return;
  }
  channel->inviteNickname(targetNick);


  std::string inviteMsg = makePrefix(client) + " INVITE " + targetNick +
                          " " + channel->getName() + "\r\n";
  server->sendReply(target->getFd(), inviteMsg);
  server->sendReply(client->getFd(),
                    RPL_INVITING(nick, targetNick, channel->getName()));
}

/* ============================= */
/*        JOIN COMMAND LOGIC     */
/* ============================= */

/**
 * @brief Processes the JOIN command.
 *
 * Steps:
 *  - Validate parameters
 *  - Parse channel names and keys
 * - For each channel:
 *   - Check for key, invite-only, and limit restrictions
 *   - Add client to channel
 *   - Broadcast JOIN message
 *   - Send NAMES list and topic information
 */
void CommandHandler::handleJOIN(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (!requireParams(server, client, cmd, 1, "JOIN"))
    return;

  std::vector<std::string> channels = splitCommaList(cmd.params[0]);
  std::vector<std::string> keys;
  if (cmd.params.size() > 1)
    keys = splitCommaList(cmd.params[1]);
  
  std::string nick = client->getNickname();
  if (channels.empty()) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS(nick, "JOIN"));
    return;
  }

  std::string prefix = makePrefix(client);

  for (size_t idx = 0; idx < channels.size(); ++idx) {
    std::string chanName = ensureChannelPrefix(channels[idx]);
    if (chanName.empty())
      continue;

    Channel *channel = server->getOrCreateChannel(chanName);
    std::string providedKey = idx < keys.size() ? keys[idx] : std::string();
    
    if (channel->hasKey() && providedKey != channel->getKey()) {
      server->sendReply(client->getFd(), ERR_BADCHANNELKEY(nick, chanName));
      continue;
    }
    if (channel->isInviteOnly() && !channel->isInvited(client->getNickname()) &&
        !channel->isOperator(client)) {
      server->sendReply(client->getFd(), ERR_INVITEONLYCHAN(nick, chanName));
      continue;
    }
    if (channel->hasLimit() && channel->isFull() &&
        !channel->isOperator(client)) {
      server->sendReply(client->getFd(), ERR_CHANNELISFULL(nick, chanName));
      continue;
    }

    bool alreadyOnChannel = channel->hasClient(client);
    
    if (!alreadyOnChannel) {
      channel->addClient(client);
      client->joinChannel(channel);
      channel->removeInvited(client->getNickname());
      
      if (channel->getClients().size() == 1) {
        channel->addOperator(client);
      }
      std::string joinMsg = prefix + " JOIN " + chanName + "\r\n";
      channel->broadcast(joinMsg, NULL);
    }
    const std::vector<Client *> &members = channel->getClients();
    std::string names;
    for (size_t i = 0; i < members.size(); ++i) {
      if (channel->isOperator(members[i]))
        names += "@";
      names += members[i]->getNickname();
      if (i + 1 < members.size())
        names += " ";
    }
    server->sendReply(client->getFd(), RPL_NAMREPLY(client->getNickname(), chanName, names));
    server->sendReply(client->getFd(), RPL_ENDOFNAMES(client->getNickname(), chanName));

    const std::string &topic = channel->getTopic();
    if (!topic.empty()) {
      server->sendReply(client->getFd(), RPL_TOPIC(client->getNickname(), chanName, topic));
    } else {
      server->sendReply(client->getFd(), RPL_NOTOPIC(client->getNickname(), chanName));
    }
    replyActiveModes(server, *channel, *client);
  }
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
  if (!requireParams(server, client, cmd, 1, "PART"))
    return;

  Channel *channel =
      expectChannel(server, client, cmd.params[0], "PART", true, true);
  if (!channel)
    return;

  std::string reason = cmd.hasTrailing ? (" :" + cmd.trailing) : "";
  std::string partMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@ircserv PART " +
                        channel->getName() + reason + "\r\n";
  channel->broadcast(partMsg, NULL);

  channel->removeClient(client);
  client->leaveChannel(channel);

  server->cleanupChannel(channel->getName());
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
 *  - Verify client is operator
 *  - Verify target is in channel
 *  - Broadcast KICK to channel
 *  - Remove user
 *  - Cleanup empty channel
 */
void CommandHandler::handleKICK(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (!requireParams(server, client, cmd, 2, "KICK"))
    return;

  std::string targetNick = cmd.params[1];
  Channel *channel =
      expectChannel(server, client, cmd.params[0], "KICK", true, true, true);
  if (!channel)
    return;

  Client *target = resolveClientOrReply(server, client, targetNick);
  if (!target)
    return;
  
  std::string nick = client->getNickname();
  if (!channel->hasClient(client)) {
     server->sendReply(client->getFd(), ERR_NOTONCHANNEL(nick, channel->getName()));
    return;
  }
  if (!channel->hasClient(target)) {
    server->sendReply(client->getFd(), ERR_USERNOTINCHANNEL(nick, targetNick, channel->getName()));
    return;
  }

  std::string kickMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@ircserv KICK " +
                        channel->getName() + " " + targetNick + "\r\n";

  channel->broadcast(kickMsg, NULL);

  channel->removeClient(target);
  target->leaveChannel(channel);

  server->cleanupChannel(channel->getName());
}
