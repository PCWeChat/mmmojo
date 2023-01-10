

#ifndef MMMOJO_COMMON_MMMOJO_ENVIRONMENT_H_
#define MMMOJO_COMMON_MMMOJO_ENVIRONMENT_H_

#include "base/memory/raw_ptr.h"

#include "base/command_line.h"
#include "base/run_loop.h"

#include "content/browser/child_process_launcher.h"
#include "content/browser/child_process_launcher_helper.h"
#include "content/public/browser/child_process_launcher_utils.h"
#include "content/public/common/content_descriptors.h"

#include "mmmojo/common/mmmojo_environment_callbacks.h"
#include "mmmojo/common/mmmojo_stream.h"

namespace mmmojo {
namespace common {

struct MMMojoEnvironmentInitParams {
  bool host_process = false;
  bool loop_start_thread = false;

#if BUILDFLAG(IS_WIN)
  const wchar_t* exe_path = nullptr;
#else
  const char* exe_path = nullptr;
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_WIN)
  const wchar_t* log_path = nullptr;
#else
  const char* log_path = nullptr;
#endif  // BUILDFLAG(IS_WIN)
  bool log_to_stderr = false;

#if BUILDFLAG(IS_WIN)
  bool defaultpolicy = false;
  bool elevated = false;
  bool compatible = true;
#endif  // BUILDFLAG(IS_WIN)

  int add_num_messagepipe = 0;
  bool set_disconnect_handlers = false;
};

class MMMojoEnvironment : public content::ChildProcessLauncher::Client {
 public:
  MMMojoEnvironment() = default;

  MMMojoEnvironment(const MMMojoEnvironment&) = delete;
  MMMojoEnvironment& operator=(const MMMojoEnvironment&) = delete;

  ~MMMojoEnvironment() override;

  void SetMMMojoCallbacks(int type, va_list* param);
  void SetMMMojoInitParams(int type, va_list* param);
  void AppendSubProcessSwitchNative(base::StringPiece switch_string,
                                    base::CommandLine::StringPieceType value);
  void StartEnvironment();
  void StartEnvironmentOnClientThread();
  void StopEnvironment();
  void StopEnvironmentOnClientThread(base::WaitableEvent* event);

  bool SendMMMojoWriteInfo(std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo);

 private:
  // https://source.chromium.org/chromium/chromium/src/+/main:content/browser/browser_child_process_host_impl.cc
  // ChildProcessLauncher::Client implementation.
  void OnProcessLaunched() override;
  void OnProcessLaunchFailed(int error_code) override;
  // TODO
#if BUILDFLAG(IS_ANDROID)
  bool CanUseWarmUpConnection() override;
#endif
  void OnMojoError(const std::string& error);

  void AttachAndLaunchSubProcess();
  void ExtractSubProcess();

  void LoopCurrentThreadIfNecessary();

  void CreateDefaultRemoteAndReceiverStream(
      mojo::ScopedMessagePipeHandle remote,
      mojo::ScopedMessagePipeHandle receiver,
      mojo::ScopedMessagePipeHandle remote_sync,
      mojo::ScopedMessagePipeHandle receiver_sync);
  void CreateAppendRemoteAndReceiverStream(
      absl::optional<std::vector<std::pair<mojo::ScopedMessagePipeHandle,
                                           mojo::ScopedMessagePipeHandle>>>&
          append_message_pipes);
  void CreateChildProcessRemoteAndReceiverStream(
      mojo::ScopedMessagePipeHandle remote,
      mojo::ScopedMessagePipeHandle receiver);

  void RemoveDefaultRemoteAndReceiverStream();
  void RemoveAppendRemoteAndReceiverStream();
  void RemoveChildProcessRemoteAndReceiverStream();

  raw_ptr<void> user_data_ = nullptr;
  MMMojoEnvironmentCallbacks delegate_;
  MMMojoEnvironmentInitParams init_params_;

  // host
  absl::optional<mojo::OutgoingInvitation> mojo_invitation_;

  // host
  absl::optional<std::unique_ptr<base::CommandLine>> command_line_;
  absl::optional<std::unique_ptr<content::ChildProcessLauncher>> child_process_;

  std::unique_ptr<MMMojoStream> child_process_stream_;
  std::vector<std::unique_ptr<MMMojoStream>> default_streams_;

  // append
  absl::optional<std::vector<std::unique_ptr<MMMojoStream>>> append_streams_;

  // should use on test
  absl::optional<std::unique_ptr<base::SingleThreadTaskExecutor>> executor_;
  absl::optional<std::unique_ptr<base::RunLoop>> run_loop_;
};

}  // namespace common
}  // namespace mmmojo

#endif  // MMMOJO_COMMON_MMMOJO_ENVIRONMENT_H_
