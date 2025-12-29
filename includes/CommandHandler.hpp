/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 02:36:25 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/05 07:09:58 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "Client.hpp"
#include "Parser.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include <sys/socket.h>

class Server;
class Channel;

/**
 * @brief Module containing static handlers for IRC commands.
 *
 * Steps:
 *  - Process each IRC command independently from Server.cpp
 *  - Keep Server.cpp focused on networking logic
 *  - Use Server pointer to modify server state safely
 */
class CommandHandler {
public:
  static void handlePASS(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleNICK(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleUSER(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleQUIT(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleINVITE(Server *server, Client *client,
                           const ParsedCommand &cmd);
  static void handleJOIN(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handlePART(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handlePRIVMSG(Server *server, Client *client,
                            const ParsedCommand &cmd);
  static void handleNOTICE(Server *server, Client *client,
                           const ParsedCommand &cmd);
  static void handlePING(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handlePONG(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleKICK(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleMODE(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleWHOIS(Server *server, Client *client,
                          const ParsedCommand &cmd);
  static void handleWHO(Server *server, Client *client,
                        const ParsedCommand &cmd);
  static void handleTOPIC(Server *server, Client *client,
                          const ParsedCommand &cmd);
  // internal helpers for command handlers
  private:
  struct ModeContext {
    Server *server;
    Client *client;
    Channel *channel;
    std::string chanName;

    std::vector<std::string> args;
    size_t argIndex;

    std::string outModes;
    std::vector<std::string> outArgs;
    char lastOutSign;

    ModeContext(Server *srv, Client *cli, Channel *chan, const std::string &name)
        : server(srv), client(cli), channel(chan), chanName(name), argIndex(0),
          lastOutSign(0) {}
  };

  static bool modeTakeArg(ModeContext &ctx, std::string &out);
  static bool modeTakePositiveInt(ModeContext &ctx, int &out, std::string *rawOut);
  static void modeAppendApplied(ModeContext &ctx, char sign, char mode,
                                const std::string *arg);
  static bool modeApplyLetter(ModeContext &ctx, char sign, char mode);
  static std::string modeBuildBroadcast(const ModeContext &ctx);

  static bool requireParams(Server *server, Client *client, const ParsedCommand &cmd,
                   size_t expectedCount, const std::string &cmdName);
  static Channel *expectChannel(Server *server, Client *client,
                       const std::string &rawName,
                       const std::string &cmdName, bool mustExist = true,
                       bool requireMember = false, bool requireOperator = false);
  static bool ensureModeTargetProvided(Server *server, Client *client, const ParsedCommand &cmd);
  static Client *resolveClientOrReply(Server *server, Client *client,
                             const std::string &nick);
  static bool ensureValidLimit(Server *server, Client *client,
                               const std::string &arg, int &outLimit);
  static void replyActiveModes(Server *server, const Channel &in, const Client &client);
};

#endif
