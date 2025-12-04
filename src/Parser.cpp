/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 02:23:09 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 03:00:57 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Parser.hpp"
#include <sstream>

/* ============================= */
/*         IRC CMD PARSER        */
/* ============================= */

ParsedCommand Parser::parse(const std::string &line) {
  ParsedCommand result;
  std::istringstream iss(line);
  std::string token;
  bool trailingFound = false;

  while (iss >> token) {
    if (!trailingFound && token.size() > 0 && token[0] == ':') {
      trailingFound = true;
      result.trailing = token.substr(1);

      std::string rest;
      std::getline(iss, rest);
      if (!rest.empty() && rest[0] == ' ')
        rest.erase(0, 1);
      result.trailing += (rest.empty() ? "" : " " + rest);
    } else if (result.command.empty()) {
      result.command = token;
    } else if (!trailingFound) {
      result.params.push_back(token);
    }
  }
  return result;
}
