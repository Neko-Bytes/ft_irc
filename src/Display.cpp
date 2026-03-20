/**
 * @file Display.cpp
 * @brief Implementation of the Display class
 */

#include "../includes/Display.hpp"
#include <iomanip>
#include <sstream>

/* ============================= */
/*          CONSTRUCTION         */
/* ============================= */

/**
 * @brief Constructs the Display object and starts the uptime clock.
 */
Display::Display(const std::string &serverName)
    : _clientCount(0), _channelCount(0), _serverName(serverName) {
  _startTime = std::chrono::steady_clock::now();
}

Display::~Display() {}

/* ============================= */
/*            WRITERS            */
/* ============================= */

void Display::addClient() { _clientCount++; }

/**
 * @brief Safely decrements the client count using a lock-free
 * compare-and-swap loop to prevent race conditions.
 */
void Display::removeClient() {
  int current = _clientCount.load();
  while (current > 0) {
    if (_clientCount.compare_exchange_weak(current, current - 1)) {
      break;
    }
  }
}

void Display::addChannel() { _channelCount++; }

/**
 * @brief Safely decrements the channel count.
 */
void Display::removeChannel() {
  int current = _channelCount.load();
  while (current > 0) {
    if (_channelCount.compare_exchange_weak(current, current - 1)) {
      break;
    }
  }
}

/* ============================= */
/*            READERS            */
/* ============================= */

/**
 * @brief Grabs a snapshot of all current status.
 * * @param outClients Reference to store current client count.
 * @param outChannels Reference to store current channel count.
 * @param outUptimeSecs Reference to store total elapsed seconds.
 */
void Display::getSnapshot(int &outClients, int &outChannels,
                          long long &outUptimeSecs) const {
  outClients = _clientCount.load();
  outChannels = _channelCount.load();

  std::chrono::time_point<std::chrono::steady_clock> now =
      std::chrono::steady_clock::now();
  outUptimeSecs =
      std::chrono::duration_cast<std::chrono::seconds>(now - _startTime)
          .count();
}

/**
 * @brief Formats the current server state into a serialized string
 * designed to be parsed easily by a microcontroller.
 * * @return std::string formatted as:
 * STATS:name|Clients:x|Chans:y|Up:hh:mm:ss\n
 */
std::string Display::getSerialPayload() const {
  int clients = 0;
  int channels = 0;
  long long uptimeSecs = 0;

  getSnapshot(clients, channels, uptimeSecs);

  long long h = uptimeSecs / 3600;
  long long m = (uptimeSecs % 3600) / 60;
  long long s = uptimeSecs % 60;

  std::ostringstream oss;

  oss << "STATS:" << _serverName << "|"
      << "Clients:" << clients << "|"
      << "Chans:" << channels << "|"
      << "Up:" << std::setfill('0') << std::setw(2) << h << ":"
      << std::setfill('0') << std::setw(2) << m << ":" << std::setfill('0')
      << std::setw(2) << s << "\n";

  return oss.str();
}
