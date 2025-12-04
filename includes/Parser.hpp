/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 02:22:31 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 02:47:31 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

/**
 * @brief Represents a parsed IRC command.
 *
 * Steps:
 *  - Store the command name
 *  - Store all parameters before the trailing part
 *  - Store the trailing message (if present, begins with ':')
 */
struct ParsedCommand {
  std::string command;
  std::vector<std::string> params;
  std::string trailing;
};

/**
 * @brief Provides parsing utilities for IRC command strings.
 *
 * Steps:
 *  - Take a raw IRC line (without CRLF)
 *  - Split the first token as the command
 *  - Collect parameters until a token begins with ':'
 *  - Store the rest as trailing text
 */
class Parser {
public:
  /**
   * @brief Parses a raw IRC command into components.
   *
   * Steps:
   *  - Split message into tokens by spaces
   *  - First token -> command
   *  - Tokens before ':' -> params
   *  - Everything after ':' -> trailing
   *
   * @param line Raw IRC message without "\r\n".
   * @return ParsedCommand Structure containing the parsed result.
   */
  static ParsedCommand parse(const std::string &line);
};

#endif
