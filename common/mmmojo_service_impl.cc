

#include "mmmojo/common/mmmojo_service_impl.h"
#include "mmmojo/common/mmmojo_stream.h"

namespace mmmojo {
namespace common {

MMMojoServiceImpl::MMMojoServiceImpl(
    mojo::ScopedMessagePipeHandle remote,
    mojo::PendingReceiver<mmmojo::mojom::MMMojoService> receiver,
    void* user_data,
    MMMojoEnvironmentCallbacks* delegate,
    bool set_disconnect_handler,
    bool connected) {
  user_data_ = user_data;
  delegate_ = delegate;
  remote_.Bind(mojo::PendingRemote<mmmojo::mojom::MMMojoService>(
      std::move(remote), 0 /* version */));
  remote_.set_disconnect_handler(
      base::BindOnce(&MMMojoServiceImpl::OnMojoDisconnect,
                     base::Unretained(this), set_disconnect_handler));
  receiver_.Bind(std::move(receiver));
  if (connected) {
    delegate_->remote_on_connect(remote_.is_connected(), user_data_);
  }
}

MMMojoServiceImpl::~MMMojoServiceImpl() = default;

void MMMojoServiceImpl::Hello(
    uint32_t request_id,
    const absl::optional<std::vector<uint8_t>>& request_data,
    const absl::optional<std::vector<uint8_t>>& attach_data) {
  if (delegate_ && delegate_->read_on_push) {
    MMMojoReadInfo* mmmojo_readinfo = new MMMojoReadInfo(
        MMMojoInfoMethod::kMMPush, false, request_data, attach_data);
    delegate_->read_on_push(request_id, mmmojo_readinfo, user_data_);
  }
}

void MMMojoServiceImpl::Hi(
    uint32_t request_id,
    const absl::optional<std::vector<uint8_t>>& request_data,
    const absl::optional<std::vector<uint8_t>>& attach_data,
    mmmojo::mojom::MMMojoService::HiCallback callback) {
  if (delegate_ && delegate_->read_on_pull) {
    MMMojoReadInfo* mmmojo_readinfo =
        new MMMojoReadInfo(MMMojoInfoMethod::kMMPullReq, false, request_data,
                           std::move(callback), attach_data);
    delegate_->read_on_pull(request_id, mmmojo_readinfo, user_data_);
  } else {
    std::move(callback).Run(request_id, absl::nullopt, absl::nullopt);
  }
}

void MMMojoServiceImpl::Hey(
    uint32_t request_id,
    ::mojo::ScopedSharedBufferHandle shared_memory,
    uint64_t shared_memory_size,
    const absl::optional<std::vector<uint8_t>>& attach_data) {
  if (delegate_ && delegate_->read_on_shared) {
    MMMojoReadInfo* mmmojo_readinfo = new MMMojoReadInfo(
        MMMojoInfoMethod::kMMShared, false, std::move(shared_memory),
        shared_memory_size, attach_data);
    delegate_->read_on_shared(request_id, mmmojo_readinfo, user_data_);
  }
}

void MMMojoServiceImpl::ShakeHandHello(
    uint32_t request_id,
    const absl::optional<std::vector<uint8_t>>& request_data,
    const absl::optional<std::vector<uint8_t>>& attach_data,
    mmmojo::mojom::MMMojoService::ShakeHandHelloCallback callback) {
  bool success = false;
  if (delegate_ && delegate_->read_on_push) {
    MMMojoReadInfo* mmmojo_readinfo = new MMMojoReadInfo(
        MMMojoInfoMethod::kMMPush, true, std::move(request_data), attach_data);
    delegate_->read_on_push(request_id, mmmojo_readinfo, user_data_);
    success = true;
  }
  std::move(callback).Run(success);
}

void MMMojoServiceImpl::ShakeHandHi(
    uint32_t request_id,
    const absl::optional<std::vector<uint8_t>>& request_data,
    const absl::optional<std::vector<uint8_t>>& attach_data,
    mmmojo::mojom::MMMojoService::ShakeHandHiCallback callback) {
  if (delegate_ && delegate_->read_on_pull) {
    MMMojoReadInfo* mmmojo_readinfo = new MMMojoReadInfo(
        MMMojoInfoMethod::kMMPullReq, true, std::move(request_data),
        std::move(callback), attach_data);
    delegate_->read_on_pull(request_id, mmmojo_readinfo, user_data_);
  } else {
    std::move(callback).Run(request_id, absl::nullopt, absl::nullopt, false);
  }
}

void MMMojoServiceImpl::ShakeHandHey(
    uint32_t request_id,
    ::mojo::ScopedSharedBufferHandle shared_memory,
    uint64_t shared_memory_size,
    const absl::optional<std::vector<uint8_t>>& attach_data,
    mmmojo::mojom::MMMojoService::ShakeHandHeyCallback callback) {
  bool success = false;
  if (delegate_ && delegate_->read_on_shared) {
    MMMojoReadInfo* mmmojo_readinfo = new MMMojoReadInfo(
        MMMojoInfoMethod::kMMShared, true, std::move(shared_memory),
        shared_memory_size, attach_data);
    delegate_->read_on_shared(request_id, mmmojo_readinfo, user_data_);
    success = true;
  }
  std::move(callback).Run(success);
}

void MMMojoServiceImpl::OnMojoDisconnect(bool is_callback) {
  remote_.reset();
  receiver_.reset();

  if (is_callback && delegate_ && delegate_->remote_on_disconnect)
    delegate_->remote_on_disconnect(user_data_);

  user_data_ = nullptr;
  delegate_ = nullptr;
}

}  // namespace common
}  // namespace mmmojo
