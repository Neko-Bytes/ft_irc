/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 16:13:57 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 18:44:05 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class Client;
class Channel;
struct ParsedCommand;
class CommandHandler;

/**
 * @brief The central server class handling socket setup, polling,
 *        client connections, and dispatching IRC commands.
 *
 * Steps:
 *  - Initialize and configure the listening socket
 *  - Use poll() to monitor all file descriptors
 *  - Accept new clients and manage their lifetime
 *  - Read incoming data and extract IRC messages
 *  - Dispatch parsed IRC commands to CommandHandler
 */
class Server {
public:
  Server(const std::string &port, const std::string &password);
  ~Server();

  void run();

  /**
   * @brief Returns the configured server password.
   *
   * Steps:
   *  - Provide read-only access for authentication checks.
   */
  const std::string &getPassword() const;

private:
  friend class CommandHandler; // allow CommandHandler to access private
                               // internals

  /* =============================
   *        DATA MEMBERS
   * ============================= */
  std::string _port;
  std::string _password;
  int _listenFd;

  std::vector<pollfd> _pollfds;
  std::map<int, Client *> _clients;
  std::map<std::string, Channel *> _channels;

  /* =============================
   *      CORE SERVER LOGIC
   * ============================= */
  void initSocket();
  void mainLoop();

  /* =============================
   *       POLL MANAGEMENT
   * ============================= */
  void addPollFd(int fd);
  void removePollFd(int fd);

  /* =============================
   *     CLIENT CONNECTION OPS
   * ============================= */
  void acceptNewClient();
  bool handleClientRead(int index);
  void removeClient(int fd);

  /* =============================
   *       MESSAGE PROCESSING
   * ============================= */
  std::vector<std::string> extractMessages(Client *client);
  void handleCommand(Client *client, const std::string &msg);

  /* =============================
   *     REGISTRATION HELPERS
   * ============================= */
  bool nicknameInUse(const std::string &nick) const;
  bool isClientFullyRegistered(Client *client) const;
  void sendWelcome(Client *client);
  void tryRegister(Client *client);

  /* ============================= */
  /*        CHANNEL HELPERS        */
  /* ============================= */

  Channel *getOrCreateChannel(const std::string &name);
  void cleanupChannel(std::string name);
  Client *getClientByNick(const std::string &nick) const;
  void removeInvitesForNick(const std::string &nick);
  void sendReply(int fd, const std::string &msg);
  void queueMessage(Client *client, const std::string &msg);
  void disconnectClientFromChannels(int fd);
};

#endif
