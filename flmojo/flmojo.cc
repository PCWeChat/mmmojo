

#include <memory>
#include <string>
#include <vector>

#include "base/command_line.h"

#include "base/debug/leak_annotations.h"
#include "base/metrics/field_trial.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/initialization_util.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#if BUILDFLAG(IS_WIN)
#include "base/win/win_util.h"
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#endif  // BUILDFLAG(IS_WIN)
#include "sandbox/policy/sandbox.h"
#include "sandbox/policy/sandbox_type.h"
#include "sandbox/policy/switches.h"

#include "flmojo/flmojo.h"

#include "mmmojo/common/mmmojo_environment.h"
#include "mmmojo/common/mmmojo_environment_callbacks.h"
#include "mmmojo/common/mmmojo_io_thread.h"
#include "mmmojo/common/mmmojo_service_impl.h"
#include "mmmojo/common/mmmojo_stream.h"
#include "mmmojo/common/mojo_ipc_initializer.h"

// log assert
// std::unique_ptr<logging::ScopedLogAssertHandler> g_assert_handler;
// void SilentRuntimeAssertHandler(const char* file,
//                                 int line,
//                                 const base::StringPiece message,
//                                 const base::StringPiece stack_trace) {}
// register it
// if (!g_assert_handler) {
//   g_assert_handler = std::make_unique<logging::ScopedLogAssertHandler>(
//       base::BindRepeating(SilentRuntimeAssertHandler));
// }

// global interface
void InitializeFLMojo(int argc, const char* const* argv) {
  // command
  base::CommandLine::Init(argc, argv);

  // mojo
  mmmojo::common::InitializeMojo();
  mmmojo::common::InitializeMMMojoIOThread();

  // sandbox
  // https://source.chromium.org/chromium/chromium/src/+/main:content/app/content_main_runner_impl.cc
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  // https://source.chromium.org/chromium/chromium/src/+/main:chrome/app/main_dll_loader_win.cc
  const bool is_sandboxed =
      !command_line.HasSwitch(sandbox::policy::switches::kNoSandbox);
  if (is_sandboxed) {
    // For child processes that are running as --no-sandbox, don't initialize
    // the sandbox info, otherwise they'll be treated as brokers (as if they
    // were the browser).
    sandbox::SandboxInterfaceInfo sandbox_info = {nullptr};
    content::InitializeSandboxInfo(&sandbox_info);
#if BUILDFLAG(IS_WIN)
    sandbox::policy::Sandbox::Initialize(
        sandbox::policy::SandboxTypeFromCommandLine(command_line),
        &sandbox_info);
#endif
  }

  // featurelist
  // https://source.chromium.org/chromium/chromium/src/+/main:content/child/field_trial.cc
  if (!base::FeatureList::GetInstance()) {
    //     base::FieldTrialList* leaked_field_trial_list =
    //         new base::FieldTrialList(nullptr);
    //     ANNOTATE_LEAKING_OBJECT_PTR(leaked_field_trial_list);
    //     std::ignore = leaked_field_trial_list;

    // Ensure any field trials in browser are reflected into the child
    // process.
    //     base::FieldTrialList::CreateTrialsFromCommandLine(command_line,
    //                                                       kMojoIPCChannel);
    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    //     base::FieldTrialList::CreateFeaturesFromCommandLine(command_line,
    //                                                         feature_list.get());
    // TODO(crbug.com/988603): This may be redundant. The way this is
    // supposed to work is that the parent process's state should be
    // passed via command-line to the child process, such that a feature
    // explicitly enabled or disabled in the parent process via this
    // mechanism (since the browser process also registers these
    // switch-dependent overrides), it will get passed via the command
    // line - so then no extra logic would be needed in the child.
    // TODO(chlily): Test this more thoroughly and understand the
    // behavior to see whether this is actually needed.
    base::FeatureList::SetInstance(std::move(feature_list));
  }

  // thread pool
  // https://source.chromium.org/chromium/chromium/src/+/main:content/browser/startup_helper.cc
  if (!base::ThreadPoolInstance::Get()) {
    constexpr size_t kThreadPoolDefaultMin = 0;
    constexpr size_t kThreadPoolMax = 1;
    constexpr double kThreadPoolCoresMultiplier = 0.6;
    constexpr size_t kThreadPoolOffset = 0;
    base::ThreadPoolInstance::Create("MMMojoThreadPool");
    base::ThreadPoolInstance::InitParams thread_pool_init_params = {
        base::RecommendedMaxNumberOfThreadsInThreadGroup(
            kThreadPoolDefaultMin, kThreadPoolMax, kThreadPoolCoresMultiplier,
            kThreadPoolOffset)};
#if BUILDFLAG(IS_WIN)
    thread_pool_init_params.common_thread_pool_environment = base::
        ThreadPoolInstance::InitParams::CommonThreadPoolEnvironment::COM_MTA;
#endif
    base::ThreadPoolInstance::Get()->Start(thread_pool_init_params);
  }

  // mojo ipc
  mmmojo::common::InitializeMojoIPC();
}

void ShutdownFLMojo() {
  // mojo ipc
  mmmojo::common::ShutdownMMMojoIOThread();
  mmmojo::common::ShutdownMojoIPC();

  // threadpool
  base::ThreadPoolInstance::Get()->Shutdown();
}

// env interface
void* CreateFLMojoEnvironment() {
  auto* mmmojo_env = new mmmojo::common::MMMojoEnvironment();
  return reinterpret_cast<void*>(mmmojo_env);
}

void SetFLMojoEnvironmentCallbacks(void* mmmojo_env, int type, ...) {
  if (!mmmojo_env)
    return;

  va_list arg;
  va_start(arg, type);

  auto* mmmojo_env_ptr =
      reinterpret_cast<mmmojo::common::MMMojoEnvironment*>(mmmojo_env);
  mmmojo_env_ptr->SetMMMojoCallbacks(type, &arg);

  va_end(arg);
}

void SetFLMojoEnvironmentInitParams(void* mmmojo_env, int type, ...) {
  if (!mmmojo_env)
    return;

  va_list arg;
  va_start(arg, type);

  auto* mmmojo_env_ptr =
      reinterpret_cast<mmmojo::common::MMMojoEnvironment*>(mmmojo_env);
  mmmojo_env_ptr->SetMMMojoInitParams(type, &arg);

  va_end(arg);
}

#if BUILDFLAG(IS_WIN)
void AppendFLSubProcessSwitchNative(void* mmmojo_env,
                                    const char* switch_string,
                                    const wchar_t* value)
#elif BUILDFLAG(IS_POSIX) || BUILDFLAG(IS_FUCHSIA)
void AppendFLSubProcessSwitchNative(void* mmmojo_env,
                                    const char* switch_string,
                                    const char* value)
#endif  // BUILDFLAG(IS_WIN)
{
  if (!mmmojo_env)
    return;

  auto* mmmojo_env_ptr =
      reinterpret_cast<mmmojo::common::MMMojoEnvironment*>(mmmojo_env);
  mmmojo_env_ptr->AppendSubProcessSwitchNative(switch_string, value);
}

void StartFLMojoEnvironment(void* mmmojo_env) {
  if (!mmmojo_env)
    return;

  auto* mmmojo_env_ptr =
      reinterpret_cast<mmmojo::common::MMMojoEnvironment*>(mmmojo_env);
  mmmojo_env_ptr->StartEnvironment();
}

void StopFLMojoEnvironment(void* mmmojo_env) {
  if (!mmmojo_env)
    return;

  auto* mmmojo_env_ptr =
      reinterpret_cast<mmmojo::common::MMMojoEnvironment*>(mmmojo_env);
  mmmojo_env_ptr->StopEnvironment();
}

void RemoveFLMojoEnvironment(void* mmmojo_env) {
  if (!mmmojo_env)
    return;

  auto* mmmojo_env_ptr =
      reinterpret_cast<mmmojo::common::MMMojoEnvironment*>(mmmojo_env);
  delete mmmojo_env_ptr;
}

// read interface
const void* GetFLMojoReadInfoRequest(const void* mmmojo_readinfo,
                                     uint32_t* request_data_size) {
  void* request_data = nullptr;

  if (!mmmojo_readinfo || !request_data_size)
    return request_data;

  auto* mmmojo_readinfo_ptr = reinterpret_cast<mmmojo::common::MMMojoReadInfo*>(
      const_cast<void*>(mmmojo_readinfo));
  switch (mmmojo_readinfo_ptr->method) {
    case MMMojoInfoMethod::kMMPush:
    case MMMojoInfoMethod::kMMPullReq:
    case MMMojoInfoMethod::kMMPullResp: {
      if (mmmojo_readinfo_ptr->request_data) {
        *request_data_size = mmmojo_readinfo_ptr->request_data->size();
        request_data = reinterpret_cast<void*>(
            const_cast<uint8_t*>(mmmojo_readinfo_ptr->request_data->data()));
      }
    } break;
    case MMMojoInfoMethod::kMMShared: {
      do {
        if (!mmmojo_readinfo_ptr->shared_memory ||
            !mmmojo_readinfo_ptr->shared_memory->is_valid() ||
            !mmmojo_readinfo_ptr->shared_memory_size) {
          break;
        }
        mojo::ScopedSharedBufferMapping mapping =
            mmmojo_readinfo_ptr->shared_memory.value()->Map(
                *mmmojo_readinfo_ptr->shared_memory_size);
        if (!mapping) {
          break;
        }
        mmmojo_readinfo_ptr->shared_buffer = std::move(mapping);
        *request_data_size = *mmmojo_readinfo_ptr->shared_memory_size;
        request_data = mmmojo_readinfo_ptr->shared_buffer->get();
      } while (0);
    } break;
    default:
      break;
  }
  return request_data;
}

const void* GetFLMojoReadInfoAttach(const void* mmmojo_readinfo,
                                    uint32_t* attach_data_size) {
  void* attach_data = nullptr;

  if (!mmmojo_readinfo || !attach_data_size)
    return attach_data;

  auto* mmmojo_readinfo_ptr =
      reinterpret_cast<const mmmojo::common::MMMojoReadInfo*>(mmmojo_readinfo);
  switch (mmmojo_readinfo_ptr->method) {
    case MMMojoInfoMethod::kMMPush:
    case MMMojoInfoMethod::kMMPullReq:
    case MMMojoInfoMethod::kMMPullResp:
    case MMMojoInfoMethod::kMMShared: {
      if (mmmojo_readinfo_ptr->attach_data) {
        *attach_data_size = mmmojo_readinfo_ptr->attach_data->size();
        attach_data = reinterpret_cast<void*>(
            const_cast<uint8_t*>(mmmojo_readinfo_ptr->attach_data->data()));
      }
    } break;
    default:
      break;
  }
  return attach_data;
}

int GetFLMojoReadInfoMethod(const void* mmmojo_readinfo) {
  if (!mmmojo_readinfo)
    return MMMojoInfoMethod::kMMNone;

  auto* mmmojo_readinfo_ptr =
      reinterpret_cast<const mmmojo::common::MMMojoReadInfo*>(mmmojo_readinfo);
  return mmmojo_readinfo_ptr->method;
}

bool GetFLMojoReadInfoSync(const void* mmmojo_readinfo) {
  if (!mmmojo_readinfo)
    return false;

  auto* mmmojo_readinfo_ptr =
      reinterpret_cast<const mmmojo::common::MMMojoReadInfo*>(mmmojo_readinfo);
  return mmmojo_readinfo_ptr->sync;
}

void RemoveFLMojoReadInfo(void* mmmojo_readinfo) {
  if (!mmmojo_readinfo)
    return;

  auto* mmmojo_readinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoReadInfo*>(mmmojo_readinfo);
  delete mmmojo_readinfo_ptr;
}

// write interface
void* CreateFLMojoWriteInfo(int method, bool sync, uint32_t request_id) {
  auto* mmmojo_writeinfo = new mmmojo::common::MMMojoWriteInfo(
      static_cast<MMMojoInfoMethod>(method), sync, request_id);
  return reinterpret_cast<void*>(mmmojo_writeinfo);
}

void SetFLMojoWriteInfoMessagePipe(void* mmmojo_writeinfo,
                                   int num_of_message_pipe) {
  if (!mmmojo_writeinfo)
    return;

  auto* mmmojo_writeinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoWriteInfo*>(mmmojo_writeinfo);
  mmmojo_writeinfo_ptr->message_pipe = num_of_message_pipe;
}

void SetFLMojoWriteInfoResponseSync(void* mmmojo_writeinfo,
                                    void** mmmojo_readinfo) {
  if (!mmmojo_writeinfo)
    return;

  auto* mmmojo_writeinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoWriteInfo*>(mmmojo_writeinfo);
  mmmojo_writeinfo_ptr->response_info = mmmojo_readinfo;
}

void* GetFLMojoWriteInfoRequest(void* mmmojo_writeinfo,
                                const uint32_t request_data_size) {
  void* request_data = nullptr;

  if (!mmmojo_writeinfo)
    return request_data;

  auto* mmmojo_writeinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoWriteInfo*>(mmmojo_writeinfo);
  switch (mmmojo_writeinfo_ptr->method) {
    case MMMojoInfoMethod::kMMPush:
    case MMMojoInfoMethod::kMMPullReq:
    case MMMojoInfoMethod::kMMPullResp: {
      mmmojo_writeinfo_ptr->request_data.emplace();
      mmmojo_writeinfo_ptr->request_data->resize(request_data_size);
      request_data = reinterpret_cast<void*>(
          const_cast<uint8_t*>(mmmojo_writeinfo_ptr->request_data->data()));
    } break;
    case MMMojoInfoMethod::kMMShared: {
      do {
        auto shmem_handle = mojo::WrapPlatformSharedMemoryRegion(
            base::UnsafeSharedMemoryRegion::TakeHandleForSerialization(
                base::UnsafeSharedMemoryRegion::Create(request_data_size)));
        if (!shmem_handle.is_valid()) {
          break;
        }
        mojo::ScopedSharedBufferMapping mapping =
            shmem_handle->Map(request_data_size);
        if (!mapping) {
          break;
        }
        mmmojo_writeinfo_ptr->shared_memory_size = request_data_size;
        mmmojo_writeinfo_ptr->shared_memory = std::move(shmem_handle);
        mmmojo_writeinfo_ptr->shared_buffer = std::move(mapping);
        request_data = mmmojo_writeinfo_ptr->shared_buffer->get();
      } while (0);
    } break;
    default:
      break;
  }
  return request_data;
}

void* GetFLMojoWriteInfoAttach(void* mmmojo_writeinfo,
                               uint32_t attach_data_size) {
  void* attach_data = nullptr;

  if (!mmmojo_writeinfo)
    return attach_data;

  auto* mmmojo_writeinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoWriteInfo*>(mmmojo_writeinfo);
  mmmojo_writeinfo_ptr->attach_data.emplace();
  mmmojo_writeinfo_ptr->attach_data->resize(attach_data_size);
  attach_data = reinterpret_cast<void*>(
      const_cast<uint8_t*>(mmmojo_writeinfo_ptr->attach_data->data()));
  return attach_data;
}

bool SwapFLMojoWriteInfoCallback(void* mmmojo_writeinfo,
                                 void* mmmojo_readinfo) {
  bool result = false;

  if (!mmmojo_writeinfo || !mmmojo_readinfo)
    return result;

  auto* mmmojo_writeinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoWriteInfo*>(mmmojo_writeinfo);
  auto* mmmojo_readinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoReadInfo*>(mmmojo_readinfo);
  mmmojo_writeinfo_ptr->callback = std::move(mmmojo_readinfo_ptr->callback);
  mmmojo_writeinfo_ptr->callbacksync =
      std::move(mmmojo_readinfo_ptr->callbacksync);
  result = true;
  return result;
}

bool SwapFLMojoWriteInfoMessage(void* mmmojo_writeinfo, void* mmmojo_readinfo) {
  bool result = false;

  if (!mmmojo_writeinfo || !mmmojo_readinfo)
    return result;

  auto* mmmojo_writeinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoWriteInfo*>(mmmojo_writeinfo);
  auto* mmmojo_readinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoReadInfo*>(mmmojo_readinfo);
  mmmojo_writeinfo_ptr->request_data =
      std::move(mmmojo_readinfo_ptr->request_data);
  mmmojo_writeinfo_ptr->attach_data =
      std::move(mmmojo_readinfo_ptr->attach_data);
  mmmojo_writeinfo_ptr->shared_memory =
      std::move(mmmojo_readinfo_ptr->shared_memory);
  mmmojo_writeinfo_ptr->shared_buffer =
      std::move(mmmojo_readinfo_ptr->shared_buffer);
  mmmojo_writeinfo_ptr->shared_memory_size =
      std::move(mmmojo_readinfo_ptr->shared_memory_size);
  result = true;
  return result;
}

bool SendFLMojoWriteInfo(void* mmmojo_env, void* mmmojo_writeinfo) {
  std::unique_ptr<mmmojo::common::MMMojoWriteInfo> mmmojo_writeinfo_ptr(
      reinterpret_cast<mmmojo::common::MMMojoWriteInfo*>(mmmojo_writeinfo));
  bool result = false;

  if (!mmmojo_env || !mmmojo_writeinfo)
    return result;

  auto* mmmojo_env_ptr =
      reinterpret_cast<mmmojo::common::MMMojoEnvironment*>(mmmojo_env);
  result = mmmojo_env_ptr->SendMMMojoWriteInfo(std::move(mmmojo_writeinfo_ptr));
  return result;
}

void RemoveFLMojoWriteInfo(void* mmmojo_writeinfo) {
  if (!mmmojo_writeinfo)
    return;

  auto* mmmojo_writeinfo_ptr =
      reinterpret_cast<mmmojo::common::MMMojoWriteInfo*>(mmmojo_writeinfo);
  delete mmmojo_writeinfo_ptr;
}
