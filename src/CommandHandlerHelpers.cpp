#include "../includes/CommandHandlerHelpers.hpp"
#include "../includes/CommandHandler.hpp"

#include "../includes/Channel.hpp"
#include "../includes/Replies.hpp"
#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include <cctype>
#include <cstdlib>
#include <climits>

bool CommandHandler::requireParams(Server *server, Client *client, const ParsedCommand &cmd,
                   size_t expectedCount, const std::string &cmdName) {
  if (cmd.params.size() < expectedCount) {
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS(nick, cmdName));
    return false;
  }
  return true;
}

Channel *CommandHandler::expectChannel(Server *server, Client *client,
                       const std::string &rawName,
                       const std::string &cmdName, bool mustExist,
                       bool requireMember, bool requireOperator) {
  std::string chanName = ensureChannelPrefix(rawName);
  std::string nick = client->getNickname().empty() ? "*" : client->getNickname();

  if (mustExist && !server->_channels.count(chanName)) {
    server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(nick, chanName));
    return NULL;
  }

  Channel *channel = NULL;
  if (server->_channels.count(chanName))
    channel = server->_channels[chanName];

  if (requireMember && channel && !channel->hasClient(client)) {
    server->sendReply(client->getFd(), ERR_NOTONCHANNEL(nick, chanName));
    return NULL;
  }

  if (requireOperator && channel && !channel->isOperator(client)) {
    server->sendReply(client->getFd(), ERR_CHANOPRIVSNEEDED(nick, chanName));
    return NULL;
  }
  (void)cmdName;
  return channel;
}

bool CommandHandler::ensureModeTargetProvided(Server *server, Client *client, const ParsedCommand &cmd) {
  if (cmd.params.empty()) {
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS(nick, "MODE"));
    return false;
  }
  return true;
}

Client *CommandHandler::resolveClientOrReply(Server *server, Client *client,
                             const std::string &nick) {
  Client *target = server->getClientByNick(nick);
  if (!target) {
    std::string clientNick = client->getNickname().empty() ? "*" : client->getNickname();
    server->sendReply(client->getFd(), ERR_NOSUCHNICK(clientNick, nick));
  }
  return target;
}

bool CommandHandler::ensureValidLimit(Server *server, Client *client, const std::string &arg,
                      int &outLimit) {
  outLimit = std::atoi(arg.c_str());
  if (outLimit <= 0) {
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS(nick, "MODE"));
    return false;
  }
  return true;
}

std::string ensureChannelPrefix(const std::string &name) {
  if (name.empty())
    return name;
  if (name[0] != '#')
    return "#" + name;
  return name;
}

std::string makePrefix(Client *client) {
  return ":" + client->getNickname() + "!" + client->getUsername() + "@ircserv";
}

std::vector<std::string> splitCommaList(const std::string &list) {
  std::vector<std::string> result;
  std::istringstream iss(list);
  std::string item;

  while (std::getline(iss, item, ',')) {
    result.push_back(item);
  }

  return result;
}

bool CommandHandler::modeTakeArg(ModeContext &ctx, std::string &out) {
  if (ctx.argIndex >= ctx.args.size())
    return false;
  out = ctx.args[ctx.argIndex++];
  return true;
}

bool CommandHandler::modeTakePositiveInt(ModeContext &ctx, int &out,
                                        std::string *rawOut) {
  std::string raw;
  if (!modeTakeArg(ctx, raw))
    return false;

  char *end = NULL;
  long parsed = std::strtol(raw.c_str(), &end, 10);
  if (!end || *end != '\0' || parsed <= 0 || parsed > INT_MAX)
    return false;

  out = static_cast<int>(parsed);
  if (rawOut)
    *rawOut = raw;
  return true;
}

void CommandHandler::modeAppendApplied(ModeContext &ctx, char sign, char mode,
                                      const std::string *arg) {
  if (ctx.lastOutSign != sign) {
    ctx.outModes += sign;
    ctx.lastOutSign = sign;
  }
  ctx.outModes += mode;
  if (arg)
    ctx.outArgs.push_back(*arg);
}

bool CommandHandler::modeApplyLetter(ModeContext &ctx, char sign, char mode) {
  switch (mode) {
  case 'i': {
    ctx.channel->setInviteOnly(sign == '+');
    modeAppendApplied(ctx, sign, 'i', NULL);
    return true;
  }
  case 't': {
    ctx.channel->setTopicProtected(sign == '+');
    modeAppendApplied(ctx, sign, 't', NULL);
    return true;
  }
  case 'k': {
    if (sign == '+') {
      std::string key;
      if (!modeTakeArg(ctx, key))
        return false;
      // Type-B mode: if parameter is missing/empty, ignore this mode.
      // (Servers may validate and error, but clients must also handle silent ignore.)
      if (key.empty())
        return false;
      if (ctx.channel->hasKey()) {
        ctx.server->sendReply(ctx.client->getFd(), ERR_KEYSET(ctx.client->getNickname(), ctx.chanName));
        return false;
      }
      ctx.channel->setKey(key);
      // Hide sensitive information in broadcast.
      const std::string maskedKey = "*";
      modeAppendApplied(ctx, sign, 'k', &maskedKey);
      return true;
    }
    if (!ctx.channel->hasKey())
      return false;
    ctx.channel->clearKey();
    modeAppendApplied(ctx, sign, 'k', NULL);
    return true;
  }
  case 'l': {
    if (sign == '+') {
      int limit = 0;
      std::string raw;
      // For MODE parsing, ignore invalid limits without emitting errors.
      if (!modeTakePositiveInt(ctx, limit, &raw))
        return false;
      ctx.channel->setLimit(limit);
      modeAppendApplied(ctx, sign, 'l', &raw);
      return true;
    }
    if (!ctx.channel->hasLimit())
      return false;
    ctx.channel->clearLimit();
    modeAppendApplied(ctx, sign, 'l', NULL);
    return true;
  }
  case 'o': {
    std::string nick;
    if (!modeTakeArg(ctx, nick))
      return false;
    Client *targetClient = resolveClientOrReply(ctx.server, ctx.client, nick);
    if (!targetClient)
      return false;
    if (!ctx.channel->hasClient(targetClient)) {
      ctx.server->sendReply(ctx.client->getFd(),
                            ERR_USERNOTINCHANNEL(ctx.client->getNickname(), nick, ctx.chanName));
      return false;
    }
    if (sign == '+')
      ctx.channel->addOperator(targetClient);
    else
      ctx.channel->removeOperator(targetClient);
    modeAppendApplied(ctx, sign, 'o', &nick);
    return true;
  }
  default:
    return false;
  }
}

std::string CommandHandler::modeBuildBroadcast(const ModeContext &ctx) {
  std::string modeMsg = makePrefix(ctx.client) + " MODE " + ctx.chanName + " " + ctx.outModes;
  for (size_t i = 0; i < ctx.outArgs.size(); ++i)
    modeMsg += " " + ctx.outArgs[i];
  modeMsg += "\r\n";
  return modeMsg;
}

void CommandHandler::replyActiveModes(Server *server, const Channel &channel,
                                      const Client &client) {
  std::string modes = "+";
  if (channel.isInviteOnly())
    modes += "i";
  if (channel.isTopicProtected())
    modes += "t";
  if (channel.hasKey())
    modes += "k";
  if (channel.hasLimit())
    modes += "l";

  std::string chanName = channel.getName();
  std::string args;
  if (channel.hasKey())
    args += " *";
  if (channel.hasLimit()) {
    std::stringstream ss;
    ss << channel.getLimit();
    args += " " + ss.str();
  }
  server->sendReply(client.getFd(), RPL_CHANNELMODEIS(client.getNickname(), chanName, modes + args));
  return;
}