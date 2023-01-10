

#ifndef MMMOJO_MMMOJO_MMMOJO_LOG_H_
#define MMMOJO_MMMOJO_MMMOJO_LOG_H_

#include "mmmojo/mmmojo_export.h"

#ifdef __cplusplus
extern "C" {
#endif

// [27984:34588:0516/214547.973:INFO:main.cc(31)]
// [process_id:thread_id:mouth&day/hour&min&sec.usec:severity:filename(line)]
MMMOJO_EXPORT void MMMojoLog(const char* file,
                             int line,
                             int severity,
                             const char* message);

MMMOJO_EXPORT void MMMojoLogArg(const char* file,
                                int line,
                                int severity,
                                const char* format,
                                ...);

#define MMMOJOLOGI(...) MMMojoLog(__FILE__, __LINE__, 0, ##__VA_ARGS__);
#define MMMOJOLOGW(...) MMMojoLog(__FILE__, __LINE__, 1, ##__VA_ARGS__);
#define MMMOJOLOGE(...) MMMojoLog(__FILE__, __LINE__, 2, ##__VA_ARGS__);
#define MMMOJOLOGF(...) MMMojoLog(__FILE__, __LINE__, 3, ##__VA_ARGS__);

#define MMMOJOLOGI_ARG(...) MMMojoLogArg(__FILE__, __LINE__, 0, ##__VA_ARGS__);
#define MMMOJOLOGW_ARG(...) MMMojoLogArg(__FILE__, __LINE__, 1, ##__VA_ARGS__);
#define MMMOJOLOGE_ARG(...) MMMojoLogArg(__FILE__, __LINE__, 2, ##__VA_ARGS__);
#define MMMOJOLOGF_ARG(...) MMMojoLogArg(__FILE__, __LINE__, 3, ##__VA_ARGS__);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MMMOJO_MMMOJO_MMMOJO_LOG_H_
