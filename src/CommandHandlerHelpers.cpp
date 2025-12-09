#include "../includes/CommandHandlerHelpers.hpp"
#include "../includes/CommandHandler.hpp"

bool CommandHandler::requireParams(Server *server, Client *client, const ParsedCommand &cmd,
                   size_t expectedCount, const std::string &cmdName) {
  if (cmd.params.size() < expectedCount) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS(cmdName));
    return false;
  }
  return true;
}

Channel *CommandHandler::expectChannel(Server *server, Client *client,
                       const std::string &rawName,
                       const std::string &cmdName, bool mustExist,
                       bool requireMember, bool requireOperator) {
  std::string chanName = ensureChannelPrefix(rawName);

  if (mustExist && !server->_channels.count(chanName)) {
    server->sendReply(client->getFd(), ERR_NOSUCHCHANNEL(chanName));
    return NULL;
  }

  Channel *channel = NULL;
  if (server->_channels.count(chanName))
    channel = server->_channels[chanName];

  if (requireMember && channel && !channel->hasClient(client)) {
    server->sendReply(client->getFd(), ERR_NOTONCHANNEL(chanName));
    return NULL;
  }

  if (requireOperator && channel && !channel->isOperator(client)) {
    server->sendReply(client->getFd(), ERR_CHANOPRIVSNEEDED(chanName));
    return NULL;
  }
  (void)cmdName;
  return channel;
}


bool CommandHandler::ensureModeTargetProvided(Server *server, Client *client) {
  server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("MODE"));
  return false;
}

Client *CommandHandler::resolveClientOrReply(Server *server, Client *client,
                             const std::string &nick) {
  Client *target = server->getClientByNick(nick);
  if (!target)
    server->sendReply(client->getFd(), ERR_NOSUCHNICK(nick));
  return target;
}

bool CommandHandler::ensureValidLimit(Server *server, Client *client, const std::string &arg,
                      int &outLimit) {
  outLimit = std::atoi(arg.c_str());
  if (outLimit <= 0) {
    server->sendReply(client->getFd(), ERR_NEEDMOREPARAMS("MODE"));
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
  return ":" + client->getNickname() + "!" + client->getUsername() +
         "@localhost";
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