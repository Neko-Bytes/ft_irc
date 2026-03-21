/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmummadi <kmummadi@student.42heilbronn.de  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/04 03:45:09 by kmummadi          #+#    #+#             */
/*   Updated: 2025/12/12 07:33:07 by kmummadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Display.hpp"
#include "../includes/Server.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

/**
 * @brief Background thread that prints the server payload every second.
 * You can pipe this output directly to your Arduino's serial port.
 */
void displayThreadFunc(Display &display) {
  while (!Server::isShuttingDown()) {
    // Generate the serialized string
    std::string payload = display.getSerialPayload();

    // For now, print to console to verify it works.
    // Later, you can send this directly to a serial port (e.g., /dev/ttyACM0)
    std::cout << payload << std::flush;

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

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

    Display display(" IRC Server ");

    std::thread lcdThread(displayThreadFunc, std::ref(display));

    Server server(port, password, display);
    server.run();

    if (lcdThread.joinable()) {
      lcdThread.join();
    }

  } catch (const std::exception &e) {
    Logger::error("Main", e.what());
    return 1;
  }

  return 0;
}
