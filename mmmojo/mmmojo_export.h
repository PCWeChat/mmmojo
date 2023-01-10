

#ifndef MMMOJO_MMMOJO_MMMOJO_EXPORT_H_
#define MMMOJO_MMMOJO_MMMOJO_EXPORT_H_

#if defined(MMMOJO_SHARED_LIBRARY)
#if defined(WIN32)

#if defined(MMMOJO_IMPLEMENTATION)
#define MMMOJO_EXPORT __declspec(dllexport)
#else
#define MMMOJO_EXPORT __declspec(dllimport)
#endif  // defined(MMMOJO_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(MMMOJO_IMPLEMENTATION)
#define MMMOJO_EXPORT __attribute__((visibility("default")))
#else
#define MMMOJO_EXPORT
#endif  // defined(MMMOJO_IMPLEMENTATION)
#endif

#else  // defined(MMMOJO_SHARED_LIBRARY)
#define MMMOJO_EXPORT
#endif

#endif  // MMMOJO_MMMOJO_MMMOJO_EXPORT_H_
