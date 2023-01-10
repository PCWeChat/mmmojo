

#ifndef MMMOJO_FLMOJO_FLMOJO_EXPORT_H_
#define MMMOJO_FLMOJO_FLMOJO_EXPORT_H_

#if defined(FLMOJO_SHARED_LIBRARY)
#if defined(WIN32)

#if defined(FLMOJO_IMPLEMENTATION)
#define FLMOJO_EXPORT __declspec(dllexport)
#else
#define FLMOJO_EXPORT __declspec(dllimport)
#endif  // defined(FLMOJO_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(FLMOJO_IMPLEMENTATION)
#define FLMOJO_EXPORT __attribute__((visibility("default")))
#else
#define FLMOJO_EXPORT
#endif  // defined(FLMOJO_IMPLEMENTATION)
#endif

#else  // defined(FLMOJO_SHARED_LIBRARY)
#define FLMOJO_EXPORT
#endif

#endif  // MMMOJO_FLMOJO_FLMOJO_EXPORT_H_
