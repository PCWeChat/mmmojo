

#include "mmmojo/common/mmmojo_environment.h"
#include "mmmojo/common/mmmojo_io_thread.h"

#include "base/rand_util.h"
#include "base/strings/stringprintf.h"

#include "content/public/common/sandboxed_process_launcher_delegate.h"

#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"

#include "sandbox/policy/mojom/sandbox.mojom.h"

namespace mmmojo {
namespace common {

const int kMMChildProcessReceiverProcessName = 0;
const int kMMChildProcessHostRemoteProcessName = 1;

const int kMMChildProcessReceiverName = 2;
const int kMMChildProcessHostRemoteName = 3;

const int kMMChildProcessReceiverSyncName = 4;
const int kMMChildProcessHostRemoteSyncName = 5;

const int kMMChildProcessReceiverAndHostRemoteSetSyncName = 1000;

const int kDefaultMessagePipe = 0;
const int kDefaultSyncMessagePipe = 1;
const int kNumDefaultMessagePipes = 2;

constexpr char kEnableLogging[] = "enable-logging";
constexpr char kLogFile[] = "log-file";

// log
bool InitLoggingFromCommandLine(const base::CommandLine& command_line) {
  base::CommandLine::StringType log_file_path;
  if (command_line.HasSwitch(kEnableLogging) ||
      command_line.HasSwitch(kLogFile)) {
    logging::LoggingSettings settings;
    if (command_line.GetSwitchValueASCII(kEnableLogging) == "stderr") {
      settings.logging_dest = logging::LOG_TO_STDERR;
    } else {
      settings.logging_dest |= logging::LOG_TO_FILE;
      log_file_path = command_line.GetSwitchValueNative(kLogFile);
      settings.log_file_path = log_file_path.c_str();
    }
    logging::SetLogItems(true /* Process ID */, true /* Thread ID */,
                         true /* Timestamp */, false /* Tick count */);
    return logging::InitLogging(settings);
  }
  return false;
}

// https://source.chromium.org/chromium/chromium/src/+/main:content/child/child_thread_impl.cc
mojo::IncomingInvitation InitializeMojoIPCChannel() {
  mojo::PlatformChannelEndpoint endpoint;
#if BUILDFLAG(IS_WIN)
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          mojo::PlatformChannel::kHandleSwitch)) {
    endpoint = mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
        *base::CommandLine::ForCurrentProcess());
  } else {
    // If this process is elevated, it will have a pipe path passed on the
    // command line.
    endpoint = mojo::NamedPlatformChannel::ConnectToServer(
        *base::CommandLine::ForCurrentProcess());
  }
#elif BUILDFLAG(IS_FUCHSIA)
  endpoint = mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
      *base::CommandLine::ForCurrentProcess());
#elif BUILDFLAG(IS_MAC)
  auto* client = base::MachPortRendezvousClient::GetInstance();
  if (!client) {
    LOG(ERROR) << "Mach rendezvous failed, terminating process (parent died?)";
    base::Process::TerminateCurrentProcessImmediately(0);
  }
  auto receive = client->TakeReceiveRight('mojo');
  if (!receive.is_valid()) {
    LOG(ERROR) << "Invalid PlatformChannel receive right";
    base::Process::TerminateCurrentProcessImmediately(0);
  }
  endpoint =
      mojo::PlatformChannelEndpoint(mojo::PlatformHandle(std::move(receive)));
#elif BUILDFLAG(IS_POSIX)
  endpoint = mojo::PlatformChannelEndpoint(mojo::PlatformHandle(base::ScopedFD(
      base::GlobalDescriptors::GetInstance()->Get(kMojoIPCChannel))));
#endif

  // diff chromium here we kill process immediately
  // and not using MOJO_ACCEPT_INVITATION_FLAG_LEAK_TRANSPORT_ENDPOINT

  //   return mojo::IncomingInvitation::Accept(
  //       std::move(endpoint),
  //       MOJO_ACCEPT_INVITATION_FLAG_LEAK_TRANSPORT_ENDPOINT);

  if (!endpoint.is_valid())
    base::Process::TerminateCurrentProcessImmediately(0);

  return mojo::IncomingInvitation::Accept(std::move(endpoint));
}

namespace mmmojo_util {

static void SendPushMMMojoWriteInfo(
    MMMojoStream* mojo_stream,
    MMMojoStream* mojo_stream_sync,
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
    bool* result) {
  if (mmmojo_writeinfo->sync) {
    if (mojo_stream_sync) {
      scoped_refptr<base::SingleThreadTaskRunner> mojo_stream_sync_thread =
          mojo_stream_sync->GetIOThreadTaskRunner();
      if (mojo_stream_sync_thread) {
        base::WaitableEvent event(
            base::WaitableEvent::ResetPolicy::AUTOMATIC,
            base::WaitableEvent::InitialState::NOT_SIGNALED);
        mojo_stream_sync_thread->PostTask(
            FROM_HERE,
            base::BindOnce(&MMMojoStream::PushMMMojoWriteInfoSync,
                           base::Unretained(mojo_stream_sync),
                           std::move(mmmojo_writeinfo), result, &event));
        event.Wait();
      }
    }
  } else {
    if (mojo_stream) {
      scoped_refptr<base::SingleThreadTaskRunner> mojo_stream_thread =
          mojo_stream->GetIOThreadTaskRunner();
      if (mojo_stream_thread) {
        mojo_stream_thread->PostTask(
            FROM_HERE, base::BindOnce(&MMMojoStream::PushMMMojoWriteInfo,
                                      base::Unretained(mojo_stream),
                                      std::move(mmmojo_writeinfo)));
        *result = true;
      }
    }
  }
}

static void SendPullReqMMMojoWriteInfo(
    MMMojoStream* mojo_stream,
    MMMojoStream* mojo_stream_sync,
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
    bool* result) {
  if (mmmojo_writeinfo->sync) {
    if (mojo_stream_sync) {
      scoped_refptr<base::SingleThreadTaskRunner> mojo_stream_sync_thread =
          mojo_stream_sync->GetIOThreadTaskRunner();
      if (mojo_stream_sync_thread) {
        base::WaitableEvent event(
            base::WaitableEvent::ResetPolicy::AUTOMATIC,
            base::WaitableEvent::InitialState::NOT_SIGNALED);
        mojo_stream_sync_thread->PostTask(
            FROM_HERE,
            base::BindOnce(&MMMojoStream::PullReqMMMojoWriteInfoSync,
                           base::Unretained(mojo_stream_sync),
                           std::move(mmmojo_writeinfo), result, &event));

        event.Wait();
      }
    }
  } else {
    if (mojo_stream) {
      scoped_refptr<base::SingleThreadTaskRunner> mojo_stream_thread =
          mojo_stream->GetIOThreadTaskRunner();
      if (mojo_stream_thread) {
        mojo_stream_thread->PostTask(
            FROM_HERE, base::BindOnce(&MMMojoStream::PullReqMMMojoWriteInfo,
                                      base::Unretained(mojo_stream),
                                      std::move(mmmojo_writeinfo)));
        *result = true;
      }
    }
  }
}

static void SendPullRespMMMojoWriteInfo(
    MMMojoStream* mojo_stream,
    MMMojoStream* mojo_stream_sync,
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
    bool* result) {
  if (mmmojo_writeinfo->sync) {
    if (mojo_stream_sync) {
      scoped_refptr<base::SingleThreadTaskRunner> mojo_stream_sync_thread =
          mojo_stream_sync->GetIOThreadTaskRunner();
      if (mojo_stream_sync_thread) {
        mojo_stream_sync_thread->PostTask(
            FROM_HERE,
            base::BindOnce(&MMMojoStream::PullRespMMMojoWriteInfoSync,
                           base::Unretained(mojo_stream_sync),
                           std::move(mmmojo_writeinfo)));
        *result = true;
      }
    }
  } else {
    if (mojo_stream) {
      scoped_refptr<base::SingleThreadTaskRunner> mojo_stream_thread =
          mojo_stream->GetIOThreadTaskRunner();
      if (mojo_stream_thread) {
        mojo_stream_thread->PostTask(
            FROM_HERE, base::BindOnce(&MMMojoStream::PullRespMMMojoWriteInfo,
                                      base::Unretained(mojo_stream),
                                      std::move(mmmojo_writeinfo)));
        *result = true;
      }
    }
  }
}

static void SendSharedMMMojoWriteInfo(
    MMMojoStream* mojo_stream,
    MMMojoStream* mojo_stream_sync,
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
    bool* result) {
  if (mmmojo_writeinfo->sync) {
    if (mojo_stream_sync) {
      scoped_refptr<base::SingleThreadTaskRunner> mojo_stream_sync_thread =
          mojo_stream_sync->GetIOThreadTaskRunner();
      if (mojo_stream_sync_thread) {
        base::WaitableEvent event(
            base::WaitableEvent::ResetPolicy::AUTOMATIC,
            base::WaitableEvent::InitialState::NOT_SIGNALED);
        mojo_stream_sync_thread->PostTask(
            FROM_HERE,
            base::BindOnce(&MMMojoStream::SharedMMMojoWriteInfoSync,
                           base::Unretained(mojo_stream_sync),
                           std::move(mmmojo_writeinfo), result, &event));
        event.Wait();
      }
    }
  } else {
    if (mojo_stream) {
      scoped_refptr<base::SingleThreadTaskRunner> mojo_stream_thread =
          mojo_stream->GetIOThreadTaskRunner();
      if (mojo_stream_thread) {
        mojo_stream_thread->PostTask(
            FROM_HERE, base::BindOnce(&MMMojoStream::SharedMMMojoWriteInfo,
                                      base::Unretained(mojo_stream),
                                      std::move(mmmojo_writeinfo)));
        *result = true;
      }
    }
  }
}

}  // namespace mmmojo_util

// delegate
class MMMojoSandboxedProcessLauncherDelegate
    : public content::SandboxedProcessLauncherDelegate {
 public:
#if BUILDFLAG(IS_WIN)
  explicit MMMojoSandboxedProcessLauncherDelegate(bool defaultpolicy,
                                                  bool elevated,
                                                  bool compatible);
#else
  MMMojoSandboxedProcessLauncherDelegate() = default;
#endif  // BUILDFLAG(IS_WIN)

  MMMojoSandboxedProcessLauncherDelegate(
      const MMMojoSandboxedProcessLauncherDelegate&) = delete;
  MMMojoSandboxedProcessLauncherDelegate& operator=(
      const MMMojoSandboxedProcessLauncherDelegate&) = delete;

  ~MMMojoSandboxedProcessLauncherDelegate() override = default;

  sandbox::mojom::Sandbox GetSandboxType() override {
    return sandbox::mojom::Sandbox::kNoSandbox;
  }

#if BUILDFLAG(IS_WIN)
  bool DisableDefaultPolicy() override { return defaultpolicy_; }
  bool ShouldLaunchElevated() override { return elevated_; }
  bool CetCompatible() override { return compatible_; }
#endif  // BUILDFLAG(IS_WIN)

 private:
  bool defaultpolicy_ = false;
  bool elevated_ = false;
  bool compatible_ = true;
};

MMMojoSandboxedProcessLauncherDelegate::MMMojoSandboxedProcessLauncherDelegate(
    bool defaultpolicy,
    bool elevated,
    bool compatible)
    : defaultpolicy_(defaultpolicy),
      elevated_(elevated),
      compatible_(compatible) {}

MMMojoEnvironment::~MMMojoEnvironment() = default;

void MMMojoEnvironment::OnProcessLaunched() {
  if (delegate_.remote_on_processlaunched)
    delegate_.remote_on_processlaunched(user_data_);
}

void MMMojoEnvironment::OnProcessLaunchFailed(int error_code) {
  if (delegate_.remote_on_processlaunchfailed) {
#if BUILDFLAG(IS_WIN)
    // last_error is DWORD on windows so safely cast int
    if (child_process_) {
      content::ChildProcessTerminationInfo info =
          child_process_.value()->GetChildTerminationInfo(true);
      error_code = static_cast<int>(info.last_error);
    }
    delegate_.remote_on_processlaunchfailed(error_code, user_data_);
#else
    delegate_.remote_on_processlaunchfailed(error_code, user_data_);
#endif
  }
}

// TODO
#if BUILDFLAG(IS_ANDROID)
bool MMMojoEnvironment::CanUseWarmUpConnection() {
  return false;
}
#endif

void MMMojoEnvironment::OnMojoError(const std::string& error) {
  if (delegate_.remote_on_mojoerror)
    delegate_.remote_on_mojoerror(error.data(), error.size(), user_data_);
}

void MMMojoEnvironment::SetMMMojoCallbacks(int type, va_list* param) {
  switch (type) {
    case MMMojoEnvironmentCallbackType::kMMUserData: {
      user_data_ = va_arg(*param, void*);
    } break;
    case MMMojoEnvironmentCallbackType::kMMReadPush: {
      delegate_.read_on_push = va_arg(*param, MMMojoReadOnPush*);
    } break;
    case MMMojoEnvironmentCallbackType::kMMReadPull: {
      delegate_.read_on_pull = va_arg(*param, MMMojoReadOnPull*);
    } break;
    case MMMojoEnvironmentCallbackType::kMMReadShared: {
      delegate_.read_on_shared = va_arg(*param, MMMojoReadOnShared*);
    } break;
    case MMMojoEnvironmentCallbackType::kMMRemoteConnect: {
      delegate_.remote_on_connect = va_arg(*param, MMMojoRemoteOnConnect*);
    } break;
    case MMMojoEnvironmentCallbackType::kMMRemoteDisconnect: {
      delegate_.remote_on_disconnect =
          va_arg(*param, MMMojoRemoteOnDisConnect*);
    } break;
    case MMMojoEnvironmentCallbackType::kMMRemoteProcessLaunched: {
      delegate_.remote_on_processlaunched =
          va_arg(*param, MMMojoRemoteOnProcessLaunched*);
    } break;
    case MMMojoEnvironmentCallbackType::kMMRemoteProcessLaunchFailed: {
      delegate_.remote_on_processlaunchfailed =
          va_arg(*param, MMMojoRemoteOnProcessLaunchFailed*);
    } break;
    case MMMojoEnvironmentCallbackType::kMMRemoteMojoError: {
      delegate_.remote_on_mojoerror = va_arg(*param, MMMojoRemoteOnMojoError*);
    } break;
    default:
      break;
  }
}

void MMMojoEnvironment::SetMMMojoInitParams(int type, va_list* param) {
  switch (type) {
    case MMMojoEnvironmentInitParamType::kMMHostProcess: {
      init_params_.host_process = va_arg(*param, int) ? true : false;
    } break;
    case MMMojoEnvironmentInitParamType::kMMLoopStartThread: {
      init_params_.loop_start_thread = va_arg(*param, int) ? true : false;
    } break;
    case MMMojoEnvironmentInitParamType::kMMExePath: {
#if BUILDFLAG(IS_WIN)
      init_params_.exe_path = va_arg(*param, wchar_t*);
#else
      init_params_.exe_path = va_arg(*param, char*);
#endif  // BUILDFLAG(IS_WIN)

      command_line_ = std::make_unique<base::CommandLine>(base::FilePath(
          base::FilePath::StringPieceType(init_params_.exe_path)));
    } break;
    case MMMojoEnvironmentInitParamType::kMMLogPath: {
#if BUILDFLAG(IS_WIN)
      init_params_.log_path = va_arg(*param, wchar_t*);
#else
      init_params_.log_path = va_arg(*param, char*);
#endif  // BUILDFLAG(IS_WIN)

      command_line_.value()->AppendSwitchNative(
          kLogFile, base::FilePath::StringPieceType(init_params_.log_path));
    } break;
    case MMMojoEnvironmentInitParamType::kMMLogToStderr: {
      init_params_.log_to_stderr = va_arg(*param, int) ? true : false;

      if (init_params_.log_to_stderr) {
        command_line_.value()->AppendSwitchASCII(kEnableLogging, "stderr");
      }
    } break;
    case MMMojoEnvironmentInitParamType::kMMAddNumMessagepipe: {
      init_params_.add_num_messagepipe = va_arg(*param, int);
    } break;
    case MMMojoEnvironmentInitParamType::kMMSetDisconnectHandlers: {
      init_params_.set_disconnect_handlers = va_arg(*param, int) ? true : false;
    } break;
#if BUILDFLAG(IS_WIN)
    case MMMojoEnvironmentInitParamType::kMMDisableDefaultPolicy: {
      init_params_.defaultpolicy = va_arg(*param, int) ? true : false;
    } break;
    case MMMojoEnvironmentInitParamType::kMMElevated: {
      init_params_.elevated = va_arg(*param, int) ? true : false;
    } break;
    case MMMojoEnvironmentInitParamType::kMMCompatible: {
      init_params_.compatible = va_arg(*param, int) ? true : false;
    } break;
#endif  // BUILDFLAG(IS_WIN)
    default:
      break;
  }
}

void MMMojoEnvironment::AppendSubProcessSwitchNative(
    base::StringPiece switch_string,
    base::CommandLine::StringPieceType value) {
  command_line_.value()->AppendSwitchNative(switch_string, value);
}

void MMMojoEnvironment::LoopCurrentThreadIfNecessary() {
  if (init_params_.loop_start_thread) {
    executor_ = std::make_unique<base::SingleThreadTaskExecutor>();
    run_loop_ = std::make_unique<base::RunLoop>();
    (*run_loop_)->Run();
    run_loop_.reset();
    executor_.reset();
  }
}

void MMMojoEnvironment::StartEnvironment() {
  GetMMMojoIOThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&MMMojoEnvironment::StartEnvironmentOnClientThread,
                     base::Unretained(this)));
  LoopCurrentThreadIfNecessary();
}

void MMMojoEnvironment::StartEnvironmentOnClientThread() {
  auto rand_thread_id = base::RandUint64();
  child_process_stream_ =
      std::make_unique<MMMojoStream>("", user_data_, &delegate_);

  default_streams_.resize(kNumDefaultMessagePipes);
  default_streams_[kDefaultMessagePipe] = std::make_unique<MMMojoStream>(
      base::StringPrintf("MMMojo RW Thread %I64u", rand_thread_id), user_data_,
      &delegate_);
  default_streams_[kDefaultSyncMessagePipe] = std::make_unique<MMMojoStream>(
      base::StringPrintf("MMMojo RW Sync Thread %I64u", rand_thread_id),
      user_data_, &delegate_);

  if (init_params_.add_num_messagepipe) {
    append_streams_.emplace();
    append_streams_->resize(init_params_.add_num_messagepipe);

    uint32_t stream_thread = 0;
    for (auto& stream : append_streams_.value()) {
      stream = std::make_unique<MMMojoStream>(
          base::StringPrintf("MMMojo RW Set Thread %u %I64u", stream_thread++,
                             rand_thread_id),
          user_data_, &delegate_);
    }
  }

  // if has cmdline we launch subprocess
  if (command_line_) {
    // host process
    AttachAndLaunchSubProcess();
  } else {
    // child process
    ExtractSubProcess();
  }
}

void MMMojoEnvironment::AttachAndLaunchSubProcess() {
  mojo_invitation_.emplace();

  mojo::ScopedMessagePipeHandle pipe_for_remote_process =
      mojo_invitation_->AttachMessagePipe(kMMChildProcessReceiverProcessName);
  mojo::ScopedMessagePipeHandle pipe_for_receiver_process =
      mojo_invitation_->AttachMessagePipe(kMMChildProcessHostRemoteProcessName);
  mojo::ScopedMessagePipeHandle pipe_for_remote =
      mojo_invitation_->AttachMessagePipe(kMMChildProcessReceiverName);
  mojo::ScopedMessagePipeHandle pipe_for_receiver =
      mojo_invitation_->AttachMessagePipe(kMMChildProcessHostRemoteName);
  mojo::ScopedMessagePipeHandle pipe_for_remote_sync =
      mojo_invitation_->AttachMessagePipe(kMMChildProcessReceiverSyncName);
  mojo::ScopedMessagePipeHandle pipe_for_receiver_sync =
      mojo_invitation_->AttachMessagePipe(kMMChildProcessHostRemoteSyncName);

  absl::optional<std::vector<
      std::pair<mojo::ScopedMessagePipeHandle, mojo::ScopedMessagePipeHandle>>>
      append_message_pipes;

  if (append_streams_) {
    uint32_t attachment_name = kMMChildProcessReceiverAndHostRemoteSetSyncName;
    append_message_pipes.emplace();
    append_message_pipes->resize(append_streams_->size());
    for (auto& message_pipe : append_message_pipes.value()) {
      message_pipe.first =
          mojo_invitation_->AttachMessagePipe(attachment_name++);
      message_pipe.second =
          mojo_invitation_->AttachMessagePipe(attachment_name++);
    }
  }

  CreateDefaultRemoteAndReceiverStream(
      std::move(pipe_for_remote), std::move(pipe_for_receiver),
      std::move(pipe_for_remote_sync), std::move(pipe_for_receiver_sync));

  CreateAppendRemoteAndReceiverStream(append_message_pipes);

  CreateChildProcessRemoteAndReceiverStream(
      std::move(pipe_for_remote_process), std::move(pipe_for_receiver_process));

  child_process_ =
#if BUILDFLAG(IS_WIN)
      std::make_unique<content::ChildProcessLauncher>(
          std::make_unique<MMMojoSandboxedProcessLauncherDelegate>(
              init_params_.defaultpolicy, init_params_.elevated,
              init_params_.compatible),
          std::move(command_line_.value()), 0, this,
          std::move(*mojo_invitation_),
          base::BindRepeating(&MMMojoEnvironment::OnMojoError,
                              base::Unretained(this)),
          nullptr);
#else
      std::make_unique<content::ChildProcessLauncher>(
          std::make_unique<MMMojoSandboxedProcessLauncherDelegate>(),
          std::move(command_line_.value()), 0, this,
          std::move(*mojo_invitation_),
          base::BindRepeating(&MMMojoEnvironment::OnMojoError,
                              base::Unretained(this)),
          nullptr);
#endif  // BUILDFLAG(IS_WIN)

  mojo_invitation_.reset();
}

void MMMojoEnvironment::ExtractSubProcess() {
  mojo::IncomingInvitation invitation = InitializeMojoIPCChannel();
  if (!invitation.is_valid())
    base::Process::TerminateCurrentProcessImmediately(0);

  InitLoggingFromCommandLine(*base::CommandLine::ForCurrentProcess());

  mojo::ScopedMessagePipeHandle pipe_for_receiver_process =
      invitation.ExtractMessagePipe(kMMChildProcessReceiverProcessName);
  mojo::ScopedMessagePipeHandle pipe_for_remote_process =
      invitation.ExtractMessagePipe(kMMChildProcessHostRemoteProcessName);
  mojo::ScopedMessagePipeHandle pipe_for_receiver =
      invitation.ExtractMessagePipe(kMMChildProcessReceiverName);
  mojo::ScopedMessagePipeHandle pipe_for_remote =
      invitation.ExtractMessagePipe(kMMChildProcessHostRemoteName);
  mojo::ScopedMessagePipeHandle pipe_for_receiver_sync =
      invitation.ExtractMessagePipe(kMMChildProcessReceiverSyncName);
  mojo::ScopedMessagePipeHandle pipe_for_remote_sync =
      invitation.ExtractMessagePipe(kMMChildProcessHostRemoteSyncName);

  absl::optional<std::vector<
      std::pair<mojo::ScopedMessagePipeHandle, mojo::ScopedMessagePipeHandle>>>
      append_message_pipes;

  if (append_streams_) {
    uint32_t attachment_name = kMMChildProcessReceiverAndHostRemoteSetSyncName;
    append_message_pipes.emplace();
    append_message_pipes->resize(append_streams_->size());
    for (auto& message_pipe : append_message_pipes.value()) {
      message_pipe.second = invitation.ExtractMessagePipe(attachment_name++);
      message_pipe.first = invitation.ExtractMessagePipe(attachment_name++);
    }
  }

  CreateDefaultRemoteAndReceiverStream(
      std::move(pipe_for_remote), std::move(pipe_for_receiver),
      std::move(pipe_for_remote_sync), std::move(pipe_for_receiver_sync));

  CreateAppendRemoteAndReceiverStream(append_message_pipes);

  CreateChildProcessRemoteAndReceiverStream(
      std::move(pipe_for_remote_process), std::move(pipe_for_receiver_process));
}

void MMMojoEnvironment::CreateDefaultRemoteAndReceiverStream(
    mojo::ScopedMessagePipeHandle remote,
    mojo::ScopedMessagePipeHandle receiver,
    mojo::ScopedMessagePipeHandle remote_sync,
    mojo::ScopedMessagePipeHandle receiver_sync) {
  default_streams_[kDefaultMessagePipe]->GetIOThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &MMMojoStream::CreateRemoteAndReceiverStream,
          base::Unretained(default_streams_[kDefaultMessagePipe].get()),
          std::move(remote), std::move(receiver),
          init_params_.set_disconnect_handlers, false));

  default_streams_[kDefaultSyncMessagePipe]->GetIOThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &MMMojoStream::CreateRemoteAndReceiverStream,
          base::Unretained(default_streams_[kDefaultSyncMessagePipe].get()),
          std::move(remote_sync), std::move(receiver_sync),
          init_params_.set_disconnect_handlers, false));
}

void MMMojoEnvironment::CreateAppendRemoteAndReceiverStream(
    absl::optional<std::vector<std::pair<mojo::ScopedMessagePipeHandle,
                                         mojo::ScopedMessagePipeHandle>>>&
        append_message_pipes) {
  if (append_message_pipes) {
    for (auto& stream : append_streams_.value()) {
      // here we using back pipe from end not front
      stream->GetIOThreadTaskRunner()->PostTask(
          FROM_HERE,
          base::BindOnce(&MMMojoStream::CreateRemoteAndReceiverStream,
                         base::Unretained(stream.get()),
                         std::move(append_message_pipes->back().first),
                         std::move(append_message_pipes->back().second),
                         init_params_.set_disconnect_handlers, false));
      append_message_pipes->pop_back();
    }
    append_message_pipes.reset();
  }
}

void MMMojoEnvironment::CreateChildProcessRemoteAndReceiverStream(
    mojo::ScopedMessagePipeHandle remote,
    mojo::ScopedMessagePipeHandle receiver) {
  child_process_stream_->CreateRemoteAndReceiverStream(
      std::move(remote), std::move(receiver), true, true);
}

void MMMojoEnvironment::StopEnvironment() {
  base::WaitableEvent event;
  GetMMMojoIOThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&MMMojoEnvironment::StopEnvironmentOnClientThread,
                     base::Unretained(this), &event));
  event.Wait();
}

void MMMojoEnvironment::StopEnvironmentOnClientThread(
    base::WaitableEvent* event) {
  RemoveDefaultRemoteAndReceiverStream();
  RemoveAppendRemoteAndReceiverStream();
  RemoveChildProcessRemoteAndReceiverStream();

  child_process_.reset();

  if (run_loop_)
    (*run_loop_)->Quit();

  event->Signal();
}

void MMMojoEnvironment::RemoveDefaultRemoteAndReceiverStream() {
  for (auto& stream : default_streams_) {
    if (stream) {
      scoped_refptr<base::SingleThreadTaskRunner> stream_thread =
          stream->GetIOThreadTaskRunner();
      if (stream_thread)
        stream_thread->PostTask(
            FROM_HERE,
            base::BindOnce(&MMMojoStream::RemoveRemoteAndReceiverStream,
                           base::Unretained(stream.get())));
      stream = nullptr;
    }
  }
  default_streams_.shrink_to_fit();
}

void MMMojoEnvironment::RemoveAppendRemoteAndReceiverStream() {
  if (append_streams_) {
    for (auto& stream : append_streams_.value()) {
      scoped_refptr<base::SingleThreadTaskRunner> stream_thread =
          stream->GetIOThreadTaskRunner();
      if (stream_thread)
        stream_thread->PostTask(
            FROM_HERE,
            base::BindOnce(&MMMojoStream::RemoveRemoteAndReceiverStream,
                           base::Unretained(stream.get())));
      stream = nullptr;
    }
    append_streams_.reset();
  }
}

void MMMojoEnvironment::RemoveChildProcessRemoteAndReceiverStream() {
  child_process_stream_ = nullptr;
}

bool MMMojoEnvironment::SendMMMojoWriteInfo(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo) {
  bool result = false;

  MMMojoStream* mojo_stream = nullptr;
  MMMojoStream* mojo_stream_sync = nullptr;

  if (mmmojo_writeinfo->message_pipe) {
    mojo_stream = mojo_stream_sync =
        append_streams_->at(mmmojo_writeinfo->message_pipe.value()).get();
  } else {
    mojo_stream = default_streams_[kDefaultMessagePipe].get();
    mojo_stream_sync = default_streams_[kDefaultSyncMessagePipe].get();
  }

  switch (mmmojo_writeinfo->method) {
    case MMMojoInfoMethod::kMMPush: {
      mmmojo_util::SendPushMMMojoWriteInfo(
          mojo_stream, mojo_stream_sync, std::move(mmmojo_writeinfo), &result);
    } break;
    case MMMojoInfoMethod::kMMPullReq: {
      mmmojo_util::SendPullReqMMMojoWriteInfo(
          mojo_stream, mojo_stream_sync, std::move(mmmojo_writeinfo), &result);
    } break;
    case MMMojoInfoMethod::kMMPullResp: {
      mmmojo_util::SendPullRespMMMojoWriteInfo(
          mojo_stream, mojo_stream_sync, std::move(mmmojo_writeinfo), &result);
    } break;
    case MMMojoInfoMethod::kMMShared: {
      mmmojo_util::SendSharedMMMojoWriteInfo(
          mojo_stream, mojo_stream_sync, std::move(mmmojo_writeinfo), &result);
    } break;
    default:
      break;
  }

  return result;
}

}  // namespace common
}  // namespace mmmojo
