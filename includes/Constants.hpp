#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cstddef>

namespace IRC {

static const size_t MaxIrcLineBytes = 512;      // with /r/n
static const size_t MaxIrcPayloadBytes = 510;   // without /r/n
static const size_t MaxInputBufferBytes = 64 * 1024; // max capacity of input buffer
static const size_t MaxOutputBufferBytes = 256 * 1024; // cap outbound queue to avoid unbounded growth

// socket read chunk size.
static const size_t ReadBufferBytes = 1024;

} // namespace IRC

#endif
