/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/12 07:29:18 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/12 07:29:33 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Logger.hpp"
#include <ctime>
#include <sstream>

std::string Logger::getCurrentTime() {
  std::time_t now = std::time(NULL);
  std::tm *now_tm = std::localtime(&now);
  char buf[80];
  std::strftime(buf, sizeof(buf), "%H:%M:%S", now_tm);
  return std::string(buf);
}

// Helper: Centers a string within a fixed width
std::string center(const std::string &str, int width) {
  int len = str.length();
  if (len >= width)
    return str.substr(0, width);

  int pad_left = (width - len) / 2;
  int pad_right = width - len - pad_left;

  return std::string(pad_left, ' ') + str + std::string(pad_right, ' ');
}

void Logger::printLayout(std::ostream &out, const std::string &color,
                         const std::string &level, const std::string &source,
                         const std::string &message) {
  // 1. Timestamp [14:20:00]
  out << GREY << "[" << getCurrentTime() << "] " << RESET;

  // 2. Level [COMMAND]
  out << color << "[" << center(level, 7) << "] " << RESET;

  // 3. Source [Server]
  std::string cleanSource = source;
  if (cleanSource.length() > 10)
    cleanSource = cleanSource.substr(0, 10);
  out << BLUE << "[" << std::setw(10) << std::left << center(cleanSource, 10)
      << "] " << RESET;

  // 4. Message
  out << message << std::endl;
}

// === GENERIC METHODS ===

void Logger::info(const std::string &source, const std::string &message) {
  printLayout(std::cout, GREEN, "INFO", source, message);
}

void Logger::error(const std::string &source, const std::string &message) {
  printLayout(std::cerr, RED, "ERROR", source, message);
}

void Logger::debug(const std::string &source, const std::string &message) {
  printLayout(std::cout, GREY, "DEBUG", source, message);
}

// === SPECIALIZED METHODS ===

void Logger::logConnect(int fd, const std::string &ip, int port) {
  std::stringstream ss;
  ss << "New connection from " << ip << ":" << port << " (FD: " << fd << ")";
  printLayout(std::cout, CYAN, "CONNECT", "Socket", ss.str());
}

void Logger::logDisconnect(int fd, const std::string &reason) {
  std::stringstream ss;
  ss << "Client (FD: " << fd << ") disconnected. Reason: " << reason;
  printLayout(std::cout, YELLOW, "DISCONN", "Socket", ss.str());
}

void Logger::logCommand(const std::string &user, const std::string &cmd,
                        const std::string &params) {
  std::string cleanUser = user.empty() ? "Unknown" : user;
  printLayout(std::cout, MAGENTA, "COMMAND", cleanUser, cmd + " " + params);
}
