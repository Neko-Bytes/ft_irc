# include "../includes/CommandHandler.hpp"

#include "../includes/Channel.hpp"
#include "../includes/Replies.hpp"
#include "../includes/Server.hpp"
#include "../includes/CommandHandlerHelpers.hpp"
#include <sys/socket.h>
#include <sstream>
#include <cstdlib>

/**
 * @brief Processes the MODE command.
 * Steps:
 * - Validate parameters
 * - Check channel existence and membership
 * - If no mode specified, return current modes
 * - For mode changes, verify operator privileges
 * - Apply mode changes and broadcast to channel members
*/
void CommandHandler::handleMODE(Server *server, Client *client,
                                const ParsedCommand &cmd) {
  if (!requireParams(server, client, cmd, 1, "MODE"))
    return;

  std::string chanName = ensureChannelPrefix(cmd.params[0]);
  std::string mode = cmd.params.size() >= 2 ? cmd.params[1] : "";

  Channel *channel =
      expectChannel(server, client, chanName, "MODE", true, true);
  if (!channel)
    return;
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

  const bool addFlag = !mode.empty() && mode[0] == '+';
  const char flag = mode.size() > 1 ? mode[1] : '\0';

  switch (flag) {
  case 'o': {
    if (target.empty() && !ensureModeTargetProvided(server, client))
      return;
    Client *targetClient = resolveClientOrReply(server, client, target);
    if (!targetClient)
      return;
    if (addFlag)
      channel->addOperator(targetClient);
    else
      channel->removeOperator(targetClient);
    modeMsg = prefix + " MODE " + chanName + (addFlag ? " +o " : " -o ") +
              target + "\r\n";
    break;
  }
  case 'k': {
    if (addFlag) {
      if (target.empty() && !ensureModeTargetProvided(server, client))
        return;
      channel->setKey(target);
      modeMsg = prefix + " MODE " + chanName + " +k " + target + "\r\n";
    } else {
      channel->clearKey();
      modeMsg = prefix + " MODE " + chanName + " -k\r\n";
    }
    break;
  }
  case 'i': {
    channel->setInviteOnly(addFlag);
    modeMsg = prefix + " MODE " + chanName + (addFlag ? " +i\r\n" : " -i\r\n");
    break;
  }
  case 'l': {
    if (addFlag) {
      int limit;
      if (target.empty() && !ensureModeTargetProvided(server, client))
        return;
      if (!ensureValidLimit(server, client, target, limit))
        return;
      channel->setLimit(limit);
      modeMsg = prefix + " MODE " + chanName + " +l " + target + "\r\n";
    } else {
      channel->clearLimit();
      modeMsg = prefix + " MODE " + chanName + " -l\r\n";
    }
    break;
  }
  case 't': {
    channel->setTopicProtected(addFlag);
    modeMsg = prefix + " MODE " + chanName + (addFlag ? " +t\r\n" : " -t\r\n");
    break;
  }
  default:
    server->sendReply(client->getFd(),
                      prefix + " MODE " + chanName + " " + mode + "\r\n");
    return;
  }

  channel->broadcast(modeMsg, NULL);
}

/**
 * @brief Processes the TOPIC command.
 *
 * Steps:
 * - Validate parameters
 * - Check channel existence and membership
 * - If no topic provided, return current topic
 * - If topic protected, verify operator privileges
 * - Set new topic and broadcast to channel members
 */
void CommandHandler::handleTOPIC(Server *server, Client *client,
                                 const ParsedCommand &cmd) {
  if (!requireParams(server, client, cmd, 1, "TOPIC"))
    return;

  std::string chanName = ensureChannelPrefix(cmd.params[0]);
  Channel *channel =
      expectChannel(server, client, chanName, "TOPIC", true, true);
  if (!channel)
    return;

  if (cmd.trailing.empty()) {
    const std::string &topic = channel->getTopic();
    if (topic.empty()) {
      server->sendReply(client->getFd(),
                        RPL_NOTOPIC(client->getNickname(), chanName));
    } else {
      server->sendReply(client->getFd(),
                        RPL_TOPIC(client->getNickname(), chanName, topic));
    }
    return;
  }

  if (channel->isTopicProtected() && !channel->isOperator(client)) {
    server->sendReply(client->getFd(), ERR_CHANOPRIVSNEEDED(chanName));
    return;
  }
  if (cmd.trailing.length() > 300) {
    server->sendReply(client->getFd(),
                      ":ircserver 422 " + client->getNickname() +
                          " " + chanName +
                          " :Topic is too long (maximum 300 characters)\r\n");
    return;
  }
  
  channel->setTopic(cmd.trailing);
  std::string topicLine = makePrefix(client) + " TOPIC " + chanName +
                          " :" + cmd.trailing + "\r\n";
  channel->broadcast(topicLine, NULL);
  server->sendReply(client->getFd(),
                    RPL_TOPIC(client->getNickname(), chanName, cmd.trailing));
}