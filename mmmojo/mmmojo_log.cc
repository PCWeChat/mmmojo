

#include "base/logging.h"
#include "base/strings/stringprintf.h"

#include "mmmojo/mmmojo_log.h"

// log interface
void MMMojoLog(const char* file, int line, int severity, const char* message) {
  if (::logging::ShouldCreateLogMessage(severity))
    ::logging::LogMessage(file, line, severity).stream() << message;
}

void MMMojoLogArg(const char* file,
                  int line,
                  int severity,
                  const char* format,
                  ...) {
  if (::logging::ShouldCreateLogMessage(severity)) {
    va_list args;
    va_start(args, format);

    std::string msg;
    base::StringAppendV(&msg, format, args);
    va_end(args);

    ::logging::LogMessage(file, line, severity).stream() << msg;
  }
}
