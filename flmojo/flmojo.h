

#ifndef MMMOJO_FLMOJO_FLMOJO_H_
#define MMMOJO_FLMOJO_FLMOJO_H_

#include <stdint.h>

#include "flmojo/flmojo_export.h"

#ifdef __cplusplus
extern "C" {
#endif

// method interface
typedef enum {
  kFLNone = 0,
  kFLPush,
  kFLPullReq,
  kFLPullResp,
  kFLShared,
} FLMojoInfoMethod;

// read interface
FLMOJO_EXPORT const void* GetFLMojoReadInfoRequest(const void* mmmojo_readinfo,
                                                   uint32_t* request_data_size);
FLMOJO_EXPORT const void* GetFLMojoReadInfoAttach(const void* mmmojo_readinfo,
                                                  uint32_t* attach_data_size);
FLMOJO_EXPORT int GetFLMojoReadInfoMethod(const void* mmmojo_readinfo);
FLMOJO_EXPORT bool GetFLMojoReadInfoSync(const void* mmmojo_readinfo);
FLMOJO_EXPORT void RemoveFLMojoReadInfo(void* mmmojo_readinfo);

// write interface
//
// MMMojoInfoMethod::kMMPush & kMMPullReq & kMMPullResp
// using default max num bytes
// mojo/core/embedder/configuration.h
// Maximum data size of messages sent over message pipes, in bytes.
// size_t max_message_num_bytes = 256 * 1024 * 1024;
//
// MMMojoInfoMethod:kMMShared
// Maximum size of a single shared memory segment, in bytes.
// size_t max_shared_memory_num_bytes = 1024 * 1024 * 1024;
FLMOJO_EXPORT void* CreateFLMojoWriteInfo(int method,
                                          bool sync,
                                          uint32_t request_id);
FLMOJO_EXPORT void SetFLMojoWriteInfoMessagePipe(void* mmmojo_writeinfo,
                                                 int num_of_message_pipe);
FLMOJO_EXPORT void SetFLMojoWriteInfoResponseSync(void* mmmojo_writeinfo,
                                                  void** mmmojo_readinfo);
FLMOJO_EXPORT void* GetFLMojoWriteInfoRequest(void* mmmojo_writeinfo,
                                              uint32_t request_data_size);
FLMOJO_EXPORT void* GetFLMojoWriteInfoAttach(void* mmmojo_writeinfo,
                                             uint32_t attach_data_size);
FLMOJO_EXPORT bool SwapFLMojoWriteInfoCallback(void* mmmojo_writeinfo,
                                               void* mmmojo_readinfo);
FLMOJO_EXPORT bool SwapFLMojoWriteInfoMessage(void* mmmojo_writeinfo,
                                              void* mmmojo_readinfo);
FLMOJO_EXPORT bool SendFLMojoWriteInfo(void* mmmojo_env,
                                       void* mmmojo_writeinfo);
FLMOJO_EXPORT void RemoveFLMojoWriteInfo(void* mmmojo_writeinfo);

// env interface
typedef enum {
  kFLUserData = 0,
  kFLReadPush,
  kFLReadPull,
  kFLReadShared,
  kFLRemoteConnect,
  kFLRemoteDisconnect,
  kFLRemoteProcessLaunched,
  kFLRemoteProcessLaunchFailed,
  kFLRemoteMojoError,
} FLMojoEnvironmentCallbackType;

typedef enum {
  kFLHostProcess = 0,
  kFLLoopStartThread,
  kFLExePath,
  kFLLogPath,
  kFLLogToStderr,
  kFLAddNumMessagepipe,
  kFLSetDisconnectHandlers,
#if defined(WIN32)
  kFLDisableDefaultPolicy = 1000,
  kFLElevated,
  kFLCompatible,
#endif  // defined(WIN32)
} FLMojoEnvironmentInitParamType;

FLMOJO_EXPORT void* CreateFLMojoEnvironment();
FLMOJO_EXPORT void SetFLMojoEnvironmentCallbacks(void* mmmojo_env,
                                                 int type,
                                                 ...);
FLMOJO_EXPORT void SetFLMojoEnvironmentInitParams(void* mmmojo_env,
                                                  int type,
                                                  ...);
#if defined(WIN32)
FLMOJO_EXPORT void AppendFLSubProcessSwitchNative(void* mmmojo_env,
                                                  const char* switch_string,
                                                  const wchar_t* value);
#else
FLMOJO_EXPORT void AppendFLSubProcessSwitchNative(void* mmmojo_env,
                                                  const char* switch_string,
                                                  const char* value);
#endif  // defined(WIN32)
FLMOJO_EXPORT void StartFLMojoEnvironment(void* mmmojo_env);
FLMOJO_EXPORT void StopFLMojoEnvironment(void* mmmojo_env);
FLMOJO_EXPORT void RemoveFLMojoEnvironment(void* mmmojo_env);

// global interface
FLMOJO_EXPORT void InitializeFLMojo(int argc, const char* const* argv);
FLMOJO_EXPORT void ShutdownFLMojo();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MMMOJO_FLMOJO_FLMOJO_H_
