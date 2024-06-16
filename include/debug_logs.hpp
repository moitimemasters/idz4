#ifndef DEBUG_LOGS_HPP
#define DEBUG_LOGS_HPP

#ifdef DEBUG_LOGS
#define DEBUG_LOG_BLOCK(block) \
    do {                       \
        block                  \
    } while (0)
#else
#define DEBUG_LOG_BLOCK(block)
#endif

#endif  // DEBUG_LOGS_HPP
