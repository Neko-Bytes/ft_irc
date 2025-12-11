/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 03:45:09 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/04 03:45:26 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>

/**
 * @brief Entry point for IRC server.
 *
 * Steps:
 *  - Validate argument count
 *  - Extract port and password
 *  - Create Server object
 *  - Run the server loop
 */
int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
    return 1;
  }

  std::string port = argv[1];
  std::string password = argv[2];

  try {
    // 1. Handle Shutdown Signals
    signal(SIGINT, Server::signalHandler);  // Ctrl+C
    signal(SIGQUIT, Server::signalHandler); // Ctrl+\ (Quit)
    signal(SIGTERM, Server::signalHandler); // Kill command

    // 2. Handle SIGPIPE (The "Crash on Disconnect" Edge Case)
    // If we write to a closed socket, we want 'send' to fail,
    // NOT the server to crash.
    signal(SIGPIPE, SIG_IGN);

    Server server(port, password);
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "Server error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
