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

#define ERR_NEEDMOREPARAMS(cmd)                                                \
  (std::string(":ircserver 461 ") + (cmd) + " :Not enough parameters\r\n")

#define ERR_ALREADYREGISTRED(nick)                                             \
  (std::string(":ircserver 462 ") + (nick) + " :You may not reregister\r\n")

#define ERR_NICKNAMEINUSE(nick)                                                \
  (std::string(":ircserver 433 * ") + (nick) +                                 \
   " :Nickname is already in use\r\n")

#define ERR_NONICKNAMEGIVEN                                                    \
  (std::string(":ircserver 431 * :No nickname given\r\n"))

#define ERR_NOSUCHNICK(nick)                                                   \
  (std::string(":ircserver 401 * ") + (nick) + " :No such nick\r\n")

  
  #define ERR_NOTONCHANNEL(chan)                                                 \
  (std::string(":ircserver 442 * ") + (chan) +                                 \
  " :You're not on that channel\r\n")
  
  
  #define ERR_NOTREGISTERED                                                      \
  (std::string(":ircserver 451 * :You have not registered\r\n"))
  
  /* ============================= */
  /*    CHANNEL ERROR NUMERICS     */
  /* ============================= */
  
  #define ERR_NOSUCHCHANNEL(chan)                                                \
    (std::string(":ircserver 403 * ") + (chan) + " :No such channel\r\n")
  
  #define ERR_CANNOTSENDTOCHAN(chan)                                             \
    (std::string(":ircserver 404 * ") + (chan) + " :Cannot send to channel\r\n")
  
#define ERR_CHANNELISFULL(chan)                                                \
  (std::string(":ircserver 471 * ") + (chan) + " :Cannot join channel (+l)\r\n")

#define ERR_INVITEONLYCHAN(chan)                                              \
  (std::string(":ircserver 473 * ") + (chan) + " :Cannot join channel (+i)\r\n")

#define ERR_BADCHANNELKEY(chan)                                               \
  (std::string(":ircserver 475 * ") + (chan) + " :Cannot join channel (+k)\r\n")
#define ERR_CHANOPRIVSNEEDED(chan)                                            \
  (std::string(":ircserver 482 * ") + (chan) + " :You're not channel operator\r\n")

/* ============================= */
/*      REGISTRATION NUMERICS    */
/* ============================= */

#define RPL_WELCOME(nick)                                                      \
  (std::string(":ircserver 001 ") + (nick) + " :Welcome to the IRC server!\r\n")

#define RPL_NAMREPLY(nick, chan, names)                                        \
  (std::string(":ircserver 353 ") + (nick) + " = " + (chan) + " :" + (names) + \
   "\r\n")

#define RPL_ENDOFNAMES(nick, chan)                                             \
  (std::string(":ircserver 366 ") + (nick) + " " + (chan) +                    \
   " :End of NAMES list\r\n")

/* ============================= */
/*      CHANNEL NUMERICS         */
/* ============================= */

#define RPL_INVITING(nick, chan)                                              \
  (std::string(":ircserver 341 * ") + (nick) + " " + (chan) +                 \
   " :You have been invited\r\n")
#define RPL_NOTOPIC(nick, chan)                                               \
  (std::string(":ircserver 331 ") + (nick) + " " + (chan) +                   \
   " :No topic is set\r\n")
#define RPL_TOPIC(nick, chan, topic)                                         \
  (std::string(":ircserver 332 ") + (nick) + " " + (chan) + " :" + (topic) + "\r\n")

/* ============================= */
/*      MODE QUERY NUMERICS      */
/* ============================= */

#define RPL_CHANNELMODEIS(nick, chan, modes)                                  \
  (std::string(":ircserver 324 ") + (nick) + " " + (chan) + " " + (modes) +  \
   "\r\n")
#endif
