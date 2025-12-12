/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/12 07:26:24 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/12 07:28:51 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iomanip>
#include <iostream>
#include <string>

// === ANSI COLORS ===
#define RESET "\033[0m"
#define GREY "\033[90m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

class Logger {
public:
  // Generic Loggers
  static void info(const std::string &source, const std::string &message);
  static void error(const std::string &source, const std::string &message);
  static void debug(const std::string &source, const std::string &message);

  // Specialized Loggers (Automates formatting)
  static void logConnect(int fd, const std::string &ip, int port);
  static void logDisconnect(int fd, const std::string &reason);
  static void logCommand(const std::string &user, const std::string &cmd,
                         const std::string &params);

private:
  static std::string getCurrentTime();
  static void printLayout(std::ostream &out, const std::string &color,
                          const std::string &level, const std::string &source,
                          const std::string &message);
};
