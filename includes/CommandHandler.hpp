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
#include <string>
#include <sys/socket.h>

class Server;

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
  static void handlePING(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handlePONG(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleKICK(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleMODE(Server *server, Client *client,
                         const ParsedCommand &cmd);
  static void handleTOPIC(Server *server, Client *client,
                          const ParsedCommand &cmd);
};

#endif
