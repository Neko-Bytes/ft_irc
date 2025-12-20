#include "../includes/CommandHandler.hpp"

#include "../includes/Channel.hpp"
#include "../includes/CommandHandlerHelpers.hpp"
#include "../includes/Replies.hpp"
#include "../includes/Server.hpp"
#include <cctype>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <sys/socket.h>

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

  const std::string target = cmd.params[0];
  const std::string modeStr = cmd.params.size() >= 2 ? cmd.params[1] : "";

  if (target.empty() || (target[0] != '#')) {
    server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(target));
    return;
  }

  // MODE #chan [<modestring> [<args>...]]
  const std::string chanName = ensureChannelPrefix(target);
  Channel *channel = expectChannel(server, client, chanName, "MODE", true, true);
  if (!channel)
    return;
  if (modeStr.empty())
    return replyActiveModes(server, *channel, *client);
  if (modeStr[0] != '+' && modeStr[0] != '-') {
      server->sendReply(client->getFd(), ERR_UMODEUNKNOWNFLAG(client->getNickname()));
    return;
  }
  if (!channel->isOperator(client)) {
    server->sendReply(client->getFd(), ERR_CHANOPRIVSNEEDED(chanName));
    return;
  }

  ModeContext ctx(server, client, channel, chanName);
  for (size_t i = 2; i < cmd.params.size(); ++i)
    ctx.args.push_back(cmd.params[i]);

  char currentSign = 0;
  for (size_t i = 0; i < modeStr.size(); ++i) {
    const char ch = modeStr[i];
    if (ch == '+' || ch == '-') {
      currentSign = ch;
      continue;
    }
    if (!std::isalpha(static_cast<unsigned char>(ch)) || currentSign == 0)
      continue;

    (void)modeApplyLetter(ctx, currentSign, ch);
  }

  if (ctx.outModes.empty())
    return;

  channel->broadcast(modeBuildBroadcast(ctx), NULL);
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

  bool isSetting = cmd.hasTrailing || cmd.params.size() > 1;

  if (!isSetting) {
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
  std::string newTopic = cmd.hasTrailing ? cmd.trailing : cmd.params[1];
  if (newTopic.length() > 300) {
    newTopic = newTopic.substr(0, 300);
  }
  channel->setTopic(newTopic);
  std::string topicLine =
      makePrefix(client) + " TOPIC " + chanName + " :" + newTopic + "\r\n";
  
  channel->broadcast(topicLine, NULL);
}
