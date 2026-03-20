#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <atomic>
#include <chrono>
#include <string>

/**
 * @brief Thread-safe telemetry container for the IRC server.
 * Tracks client counts, channel counts, and server uptime.
 * Designed to be read by a background thread for external displays.
 */
class Display {
public:
  Display(const std::string &serverName);
  ~Display();

  /* ============================= */
  /* WRITERS            */
  /* ============================= */
  void addClient();
  void removeClient();
  void addChannel();
  void removeChannel();

  /* ============================= */
  /* READERS            */
  /* ============================= */
  void getSnapshot(int &outClients, int &outChannels,
                   long long &outUptimeSecs) const;
  std::string getSerialPayload() const;

private:
  /* ============================= */
  /* DATA MEMBERS         */
  /* ============================= */
  std::atomic<int> _clientCount;
  std::atomic<int> _channelCount;
  std::string _serverName;
  std::chrono::time_point<std::chrono::steady_clock> _startTime;
};

#endif
