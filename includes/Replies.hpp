/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Replies.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 06:24:48 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/05 07:27:28 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REPLIES_HPP
#define REPLIES_HPP

#include <string>

/*
 * In IRC protocol, the * symbol is used when the client does not yet have a
 * nickname, or when the numeric reply cannot logically address the user by
 * name.
 */

/* ============================= */
/*        ERROR NUMERICS         */
/* ============================= */

#define ERR_NEEDMOREPARAMS(nick, cmd)                                          \
  (std::string(":ircserv 461 ") + (nick) + " " + (cmd) +                     \
   " :Not enough parameters\r\n")

#define ERR_ALREADYREGISTRED(nick)                                             \
  (std::string(":ircserv 462 ") + (nick) + " :You may not reregister\r\n")

#define ERR_NICKNAMEINUSE(client, nick)                                        \
  (std::string(":ircserv 433 ") + (client) + " " + (nick) +                  \
   " :Nickname is already in use\r\n")

#define ERR_ERRONEUSNICKNAME(client, nick)                                     \
  (std::string(":ircserv 432 ") + (client) + " " + (nick) +                  \
   " :Erroneous nickname\r\n")

#define ERR_NONICKNAMEGIVEN(nick)                                              \
  (std::string(":ircserv 431 ") + (nick) + " :No nickname given\r\n")

#define ERR_NOSUCHNICK(client, nick)                                           \
  (std::string(":ircserv 401 ") + (client) + " " + (nick) +                  \
   " :No such nick\r\n")

#define ERR_NOTREGISTERED(nick)                                                \
  (std::string(":ircserv 451 ") + (nick) + " :You have not registered\r\n")

#define ERR_PASSWDMISMATCH(nick)                                               \
  (std::string(":ircserv 464 ") + (nick) + " :Password incorrect\r\n")

#define ERR_NOMOTD(nick)                                               \
  (std::string(":ircserv 422 ") + (nick) + " :MOTD File is missing\r\n")

/* ============================= */
/*    CHANNEL ERROR NUMERICS     */
/* ============================= */

#define ERR_NOSUCHCHANNEL(nick, chan)                                          \
  (std::string(":ircserv 403 ") + (nick) + " " + (chan) +                    \
   " :No such channel\r\n")

#define ERR_USERNOTINCHANNEL(client, nick, chan)                               \
  (std::string(":ircserv 441 ") + (client) + " " + (nick) + " " + (chan) +   \
   " :They aren't on that channel\r\n")

#define ERR_NOTONCHANNEL(nick, chan)                                           \
  (std::string(":ircserv 442 ") + (nick) + " " + (chan) +                    \
   " :You're not on that channel\r\n")

#define ERR_CANNOTSENDTOCHAN(nick, chan)                                       \
  (std::string(":ircserv 404 ") + (nick) + " " + (chan) +                    \
   " :Cannot send to channel\r\n")

#define ERR_CHANNELISFULL(nick, chan)                                          \
  (std::string(":ircserv 471 ") + (nick) + " " + (chan) +                    \
   " :Cannot join channel (+l)\r\n")

#define ERR_INVITEONLYCHAN(nick, chan)                                         \
  (std::string(":ircserv 473 ") + (nick) + " " + (chan) +                    \
   " :Cannot join channel (+i)\r\n")

#define ERR_BADCHANNELKEY(nick, chan)                                          \
  (std::string(":ircserv 475 ") + (nick) + " " + (chan) +                    \
   " :Cannot join channel (+k)\r\n")

#define ERR_CHANOPRIVSNEEDED(nick, chan)                                       \
  (std::string(":ircserv 482 ") + (nick) + " " + (chan) +                    \
   " :You're not channel operator\r\n")

#define ERR_USERONCHANNEL(client, nick, chan)                                  \
  (std::string(":ircserv 443 ") + (client) + " " + (nick) + " " + (chan) +   \
   " :is already on channel\r\n")

#define ERR_USERSDONTMATCH(nick)                                               \
  (std::string(":ircserv 502 ") + (nick) +                                   \
   " :Cannot change mode for other users\r\n")

#define ERR_UMODEUNKNOWNFLAG(nick)                                             \
  (std::string(":ircserv 501 ") + (nick) + " :Unknown MODE flag\r\n")

#define ERR_INVALIDMODEPARAM(nick, chan)                                       \
  (std::string(":ircserv 696 ") + (nick) + " " + (chan) +                    \
   " :Invalid MODE parameter\r\n")
#define ERR_NORECIPIENT(nick, cmd)                                             \
  (std::string(":ircserv 411 ") + (nick) + " :No recipient given (" +        \
   (cmd) + ")\r\n")

#define ERR_NOTEXTTOSEND(nick)                                                 \
  (std::string(":ircserv 412 ") + (nick) + " :No text to send\r\n")

#define ERR_UNKNOWNCOMMAND(nick, cmd)                                          \
  (std::string(":ircserv 421 ") + (nick) + " " + (cmd) + " :Unknown command\r\n")
#define ERR_KEYSET(nick, chan)                                                 \
  (std::string(":ircserv 467 ") + (nick) + " " + (chan) +                    \
   " :Channel key already set\r\n")
/* ============================= */
/*      REGISTRATION NUMERICS    */
/* ============================= */

#define RPL_WELCOME(nick)                                                      \
  (std::string(":ircserv 001 ") + (nick) + " :Welcome to the IRC server!\r\n")

#define RPL_YOURHOST(nick)                                                      \
  (std::string(":ircserv 002 ") + (nick) +                                      \
   " :Your host is ircserv, running version 0.42\r\n")

#define RPL_CREATED(nick, datetime)                                              \
  (std::string(":ircserv 003 ") + (nick) + " :this server was created " +        \
   datetime + "\r\n")

#define RPL_MYINFO(nick)                                                      \
  (std::string(":ircserv 004 ") + (nick) + " ircserv 0.42 - kilot kol\r\n")

#define RPL_ISUPPORT(nick)                                                    \
  (std::string(":ircserv 005 ") + (nick) +                                  \
   " CHANTYPES=# CHANMODES=itkol :are supported by this server\r\n")

#define RPL_NAMREPLY(nick, chan, names)                                        \
  (std::string(":ircserv 353 ") + (nick) + " = " + (chan) + " :" + (names) + \
   "\r\n")

#define RPL_ENDOFNAMES(nick, chan)                                             \
  (std::string(":ircserv 366 ") + (nick) + " " + (chan) +                    \
   " :End of NAMES list\r\n")

/* ============================= */
/*      CHANNEL NUMERICS         */
/* ============================= */

#define RPL_INVITING(client, nick, chan)                                             \
  (std::string(":ircserv 341 ") + (client) + " " + (nick) + " " + (chan) + "\r\n")
#define RPL_NOTOPIC(nick, chan)                                                \
  (std::string(":ircserv 331 ") + (nick) + " " + (chan) +                    \
   " :No topic is set\r\n")
#define RPL_TOPIC(nick, chan, topic)                                           \
  (std::string(":ircserv 332 ") + (nick) + " " + (chan) + " :" + (topic) +   \
   "\r\n")

/* ============================= */
/*      QUERY NUMERICS           */
/* ============================= */

#define RPL_CHANNELMODEIS(nick, chan, modes)                                   \
  (std::string(":ircserv 324 ") + (nick) + " " + (chan) + " " + (modes) +    \
   "\r\n")

#define RPL_UMODEIS(nick, modes)                                             \
  (std::string(":ircserv 221 ") + (nick) + " " + (modes) + "\r\n")

#define RPL_WHOISUSER(client, nick, user, host, real)                                  \
  (std::string(":ircserv 311 ") + (client) + " " + (nick) + " " + (user) + " " + (host) +     \
   " * :" + (real) + "\r\n")
#define RPL_WHOISCHANNELS(client, nick, chanList)                                      \
  (std::string(":ircserv 319 ") + (client) + " " + (nick) + " :" + (chanList) + "\r\n")
#define RPL_ENDOFWHOIS(client, nick)                                                   \
  (std::string(":ircserv 318 ") + (client) + " " + (nick) + " :End of WHOIS list\r\n")
#define RPL_WHOREPLY(requester, channel, user, host, server, nick, status, real) \
  (std::string(":ircserv 352 ") + (requester) + " " + (channel) + " " +        \
   (user) + " " + (host) + " " + (server) + " " + (nick) + " " + (status) + \
   " :" + (real) + "\r\n")
#define RPL_ENDOFWHO(requester, name)                                          \
  (std::string(":ircserv 315 ") + (requester) + " " + (name) +              \
   " :End of WHO list\r\n")
#endif
