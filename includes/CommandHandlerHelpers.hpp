# ifndef COMMAND_HANDLER_HELPERS_HPP
# define COMMAND_HANDLER_HELPERS_HPP

#include "Client.hpp"
#include "Channel.hpp"
#include "Server.hpp"
#include "Parser.hpp"
#include "Replies.hpp"

#include <string>
#include <vector>
#include <sstream>
# include <cstdlib>

std::string ensureChannelPrefix(const std::string &name);
std::string makePrefix(Client *client);
std::vector<std::string> splitCommaList(const std::string &list);
# endif