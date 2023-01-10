

#ifndef MMMOJO_FLMOJO_FLMOJO_LOG_H_
#define MMMOJO_FLMOJO_FLMOJO_LOG_H_

#include "flmojo/flmojo_export.h"

#ifdef __cplusplus
extern "C" {
#endif

// [27984:34588:0516/214547.973:INFO:main.cc(31)]
// [process_id:thread_id:mouth&day/hour&min&sec.usec:severity:filename(line)]
FLMOJO_EXPORT void FLMojoLog(const char* file,
                             int line,
                             int severity,
                             const char* message);

FLMOJO_EXPORT void FLMojoLogArg(const char* file,
                                int line,
                                int severity,
                                const char* format,
                                ...);

#define FLMOJOLOGI(...) FLMojoLog(__FILE__, __LINE__, 0, ##__VA_ARGS__);
#define FLMOJOLOGW(...) FLMojoLog(__FILE__, __LINE__, 1, ##__VA_ARGS__);
#define FLMOJOLOGE(...) FLMojoLog(__FILE__, __LINE__, 2, ##__VA_ARGS__);
#define FLMOJOLOGF(...) FLMojoLog(__FILE__, __LINE__, 3, ##__VA_ARGS__);

#define FLMOJOLOGI_ARG(...) FLMojoLogArg(__FILE__, __LINE__, 0, ##__VA_ARGS__);
#define FLMOJOLOGW_ARG(...) FLMojoLogArg(__FILE__, __LINE__, 1, ##__VA_ARGS__);
#define FLMOJOLOGE_ARG(...) FLMojoLogArg(__FILE__, __LINE__, 2, ##__VA_ARGS__);
#define FLMOJOLOGF_ARG(...) FLMojoLogArg(__FILE__, __LINE__, 3, ##__VA_ARGS__);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MMMOJO_FLMOJO_FLMOJO_LOG_H_
