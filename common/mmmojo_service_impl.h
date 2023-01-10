

#ifndef MMMOJO_COMMON_MMMOJO_SERVICE_IMPL_H_
#define MMMOJO_COMMON_MMMOJO_SERVICE_IMPL_H_

#include "base/memory/raw_ptr.h"

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

#include "mmmojo/common/mmmojo_environment_callbacks.h"

#include "mmmojo/common/mojom/mmmojo.mojom.h"

namespace mmmojo {
namespace common {

class MMMojoServiceImpl : mmmojo::mojom::MMMojoService {
 public:
  explicit MMMojoServiceImpl(
      mojo::ScopedMessagePipeHandle remote,
      mojo::PendingReceiver<mmmojo::mojom::MMMojoService> receiver,
      void* user_data,
      MMMojoEnvironmentCallbacks* delegate,
      bool set_disconnect_handler,
      bool connected);

  MMMojoServiceImpl(const MMMojoServiceImpl&) = delete;
  MMMojoServiceImpl& operator=(const MMMojoServiceImpl&) = delete;

  ~MMMojoServiceImpl() override;

 private:
  void Hello(uint32_t request_id,
             const absl::optional<std::vector<uint8_t>>& request_data,
             const absl::optional<std::vector<uint8_t>>& attach_data) override;

  void Hi(uint32_t request_id,
          const absl::optional<std::vector<uint8_t>>& request_data,
          const absl::optional<std::vector<uint8_t>>& attach_data,
          mmmojo::mojom::MMMojoService::HiCallback callback) override;

  void Hey(uint32_t request_id,
           ::mojo::ScopedSharedBufferHandle shared_memory,
           uint64_t shared_memory_size,
           const absl::optional<std::vector<uint8_t>>& attach_data) override;

  void ShakeHandHello(
      uint32_t request_id,
      const absl::optional<std::vector<uint8_t>>& request_data,
      const absl::optional<std::vector<uint8_t>>& attach_data,
      mmmojo::mojom::MMMojoService::ShakeHandHelloCallback callback) override;

  void ShakeHandHi(
      uint32_t request_id,
      const absl::optional<std::vector<uint8_t>>& request_data,
      const absl::optional<std::vector<uint8_t>>& attach_data,
      mmmojo::mojom::MMMojoService::ShakeHandHiCallback callback) override;

  void ShakeHandHey(
      uint32_t request_id,
      ::mojo::ScopedSharedBufferHandle shared_memory,
      uint64_t shared_memory_size,
      const absl::optional<std::vector<uint8_t>>& attach_data,
      mmmojo::mojom::MMMojoService::ShakeHandHeyCallback callback) override;

  void OnMojoDisconnect(bool is_callback);

  friend class MMMojoStream;

  raw_ptr<void> user_data_ = nullptr;
  raw_ptr<MMMojoEnvironmentCallbacks> delegate_ = nullptr;

  mojo::Remote<mmmojo::mojom::MMMojoService> remote_;
  mojo::Receiver<mmmojo::mojom::MMMojoService> receiver_{this};
};

}  // namespace common
}  // namespace mmmojo

#endif  // MMMOJO_COMMON_MMMOJO_SERVICE_IMPL_H_
