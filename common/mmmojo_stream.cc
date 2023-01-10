

#include "mmmojo/common/mmmojo_stream.h"

namespace mmmojo {
namespace common {

// readinfo
MMMojoReadInfo::MMMojoReadInfo(
    MMMojoInfoMethod _method,
    bool _sync,
    const absl::optional<std::vector<uint8_t>>& _request_data,
    const absl::optional<std::vector<uint8_t>>& _attach_data)
    : method(_method), sync(_sync) {
  request_data = std::move(
      const_cast<absl::optional<std::vector<uint8_t>>&>(_request_data));
  attach_data = std::move(
      const_cast<absl::optional<std::vector<uint8_t>>&>(_attach_data));
}

MMMojoReadInfo::MMMojoReadInfo(
    MMMojoInfoMethod _method,
    bool _sync,
    const absl::optional<std::vector<uint8_t>>& _request_data,
    mmmojo::mojom::MMMojoService::HiCallback _callback,
    const absl::optional<std::vector<uint8_t>>& _attach_data)
    : method(_method), sync(_sync), callback(std::move(_callback)) {
  request_data = std::move(
      const_cast<absl::optional<std::vector<uint8_t>>&>(_request_data));
  attach_data = std::move(
      const_cast<absl::optional<std::vector<uint8_t>>&>(_attach_data));
}

MMMojoReadInfo::MMMojoReadInfo(
    MMMojoInfoMethod _method,
    bool _sync,
    const absl::optional<std::vector<uint8_t>>& _request_data,
    mmmojo::mojom::MMMojoService::ShakeHandHiCallback _callback,
    const absl::optional<std::vector<uint8_t>>& _attach_data)
    : method(_method), sync(_sync), callbacksync(std::move(_callback)) {
  request_data = std::move(
      const_cast<absl::optional<std::vector<uint8_t>>&>(_request_data));
  attach_data = std::move(
      const_cast<absl::optional<std::vector<uint8_t>>&>(_attach_data));
}

MMMojoReadInfo::MMMojoReadInfo(
    MMMojoInfoMethod _method,
    bool _sync,
    ::mojo::ScopedSharedBufferHandle _shared_memory,
    uint64_t _shared_memory_size,
    const absl::optional<std::vector<uint8_t>>& _attach_data)
    : method(_method),
      sync(_sync),
      shared_memory(std::move(_shared_memory)),
      shared_memory_size(_shared_memory_size) {
  attach_data = std::move(
      const_cast<absl::optional<std::vector<uint8_t>>&>(_attach_data));
}

// writeinfo
MMMojoWriteInfo::MMMojoWriteInfo(MMMojoInfoMethod _method,
                                 bool _sync,
                                 uint32_t _request_id)
    : method(_method), sync(_sync), request_id(_request_id) {}

// stream
MMMojoStream::MMMojoStream(const std::string& name,
                           void* user_data,
                           MMMojoEnvironmentCallbacks* delegate) {
  user_data_ = user_data;
  delegate_ = delegate;
  if (!name.empty()) {
    thread_ = std::make_unique<base::Thread>(name);
    thread_->StartWithOptions(
        base::Thread::Options(base::MessagePumpType::IO, 0));
  }
}

scoped_refptr<base::SingleThreadTaskRunner>
MMMojoStream::GetIOThreadTaskRunner() {
  if (thread_)
    return thread_->IsRunning() ? thread_->task_runner() : nullptr;
  return nullptr;
}

void MMMojoStream::CreateRemoteAndReceiverStream(
    mojo::ScopedMessagePipeHandle remote,
    mojo::ScopedMessagePipeHandle receiver,
    bool set_disconnect_handler,
    bool connected) {
  impl_ = std::make_unique<MMMojoServiceImpl>(
      std::move(remote),
      mojo::PendingReceiver<mmmojo::mojom::MMMojoService>(std::move(receiver)),
      user_data_, delegate_, set_disconnect_handler, connected);
}

void MMMojoStream::RemoveRemoteAndReceiverStream() {
  impl_ = nullptr;
}

void MMMojoStream::PushMMMojoWriteInfo(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo) {
  if (impl_ && impl_->remote_)
    impl_->remote_->Hello(mmmojo_writeinfo->request_id,
                          mmmojo_writeinfo->request_data,
                          mmmojo_writeinfo->attach_data);
}

void MMMojoStream::PushMMMojoWriteInfoSync(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
    bool* success,
    base::WaitableEvent* event) {
  if (impl_ && impl_->remote_)
    impl_->remote_->ShakeHandHello(mmmojo_writeinfo->request_id,
                                   mmmojo_writeinfo->request_data,
                                   mmmojo_writeinfo->attach_data, success);
  event->Signal();
}

void MMMojoStream::PullReqMMMojoWriteInfo(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo) {
  if (impl_ && impl_->remote_)
    impl_->remote_->Hi(mmmojo_writeinfo->request_id,
                       mmmojo_writeinfo->request_data,
                       mmmojo_writeinfo->attach_data,
                       base::BindOnce(&MMMojoStream::OnPushReqMMMojoReadInfo,
                                      base::Unretained(this)));
}

void MMMojoStream::PullReqMMMojoWriteInfoSync(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
    bool* success,
    base::WaitableEvent* event) {
  if (impl_ && impl_->remote_) {
    if (mmmojo_writeinfo->response_info) {
      uint32_t request_id;
      absl::optional<std::vector<uint8_t>> request_data;
      absl::optional<std::vector<uint8_t>> attach_data;
      impl_->remote_->ShakeHandHi(mmmojo_writeinfo->request_id,
                                  mmmojo_writeinfo->request_data,
                                  mmmojo_writeinfo->attach_data, &request_id,
                                  &request_data, &attach_data, success);
      if (success) {
        MMMojoReadInfo* mmmojo_readinfo = new MMMojoReadInfo(
            MMMojoInfoMethod::kMMPullResp, true, request_data, attach_data);
        **mmmojo_writeinfo->response_info =
            reinterpret_cast<void*>(mmmojo_readinfo);
      }
    } else {
      // pass callback is not sync
      impl_->remote_->ShakeHandHi(
          mmmojo_writeinfo->request_id, mmmojo_writeinfo->request_data,
          mmmojo_writeinfo->attach_data,
          base::BindOnce(&MMMojoStream::OnPushReqMMMojoReadInfoSync,
                         base::Unretained(this)));
      *success = true;
    }
  }
  event->Signal();
}

void MMMojoStream::PullRespMMMojoWriteInfo(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo) {
  if (impl_ && mmmojo_writeinfo->callback)
    std::move(mmmojo_writeinfo->callback.value())
        .Run(mmmojo_writeinfo->request_id, mmmojo_writeinfo->request_data,
             mmmojo_writeinfo->attach_data);
}

void MMMojoStream::PullRespMMMojoWriteInfoSync(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo) {
  if (impl_ && mmmojo_writeinfo->callbacksync)
    std::move(mmmojo_writeinfo->callbacksync.value())
        .Run(mmmojo_writeinfo->request_id, mmmojo_writeinfo->request_data,
             mmmojo_writeinfo->attach_data, true);
}

void MMMojoStream::SharedMMMojoWriteInfo(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo) {
  if (impl_ && impl_->remote_)
    impl_->remote_->Hey(mmmojo_writeinfo->request_id,
                        std::move(mmmojo_writeinfo->shared_memory.value()),
                        mmmojo_writeinfo->shared_memory_size.value(),
                        mmmojo_writeinfo->attach_data);
}

void MMMojoStream::SharedMMMojoWriteInfoSync(
    std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
    bool* success,
    base::WaitableEvent* event) {
  if (impl_ && impl_->remote_)
    impl_->remote_->ShakeHandHey(
        mmmojo_writeinfo->request_id,
        std::move(mmmojo_writeinfo->shared_memory.value()),
        mmmojo_writeinfo->shared_memory_size.value(),
        mmmojo_writeinfo->attach_data, success);
  event->Signal();
}

void MMMojoStream::OnPushReqMMMojoReadInfo(
    uint32_t request_id,
    const absl::optional<std::vector<uint8_t>>& request_data,
    const absl::optional<std::vector<uint8_t>>& attach_data) {
  if (delegate_ && delegate_->read_on_pull) {
    MMMojoReadInfo* mmmojo_readinfo = new MMMojoReadInfo(
        MMMojoInfoMethod::kMMPullResp, false, request_data, attach_data);
    delegate_->read_on_pull(request_id, mmmojo_readinfo, user_data_);
  }
}

void MMMojoStream::OnPushReqMMMojoReadInfoSync(
    uint32_t request_id,
    const absl::optional<std::vector<uint8_t>>& request_data,
    const absl::optional<std::vector<uint8_t>>& attach_data,
    bool success) {
  if (success && delegate_ && delegate_->read_on_pull) {
    MMMojoReadInfo* mmmojo_readinfo = new MMMojoReadInfo(
        MMMojoInfoMethod::kMMPullResp, true, request_data, attach_data);
    delegate_->read_on_pull(request_id, mmmojo_readinfo, user_data_);
  }
}

}  // namespace common
}  // namespace mmmojo
