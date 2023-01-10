

#ifndef MMMOJO_COMMON_MMMOJO_STREAM_H_
#define MMMOJO_COMMON_MMMOJO_STREAM_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/task/single_thread_task_executor.h"
#include "base/threading/thread.h"

#include "mmmojo/common/mmmojo_environment_callbacks.h"
#include "mmmojo/common/mmmojo_service_impl.h"
#include "mmmojo/mmmojo.h"

#include "mmmojo/common/mojom/mmmojo.mojom.h"

namespace mmmojo {
namespace common {

// readinfo
struct MMMojoReadInfo {
  explicit MMMojoReadInfo(
      MMMojoInfoMethod _method,
      bool _sync,
      const absl::optional<std::vector<uint8_t>>& _request_data,
      const absl::optional<std::vector<uint8_t>>& _attach_data);

  explicit MMMojoReadInfo(
      MMMojoInfoMethod _method,
      bool _sync,
      const absl::optional<std::vector<uint8_t>>& _request_data,
      mmmojo::mojom::MMMojoService::HiCallback _callback,
      const absl::optional<std::vector<uint8_t>>& _attach_data);

  explicit MMMojoReadInfo(
      MMMojoInfoMethod _method,
      bool _sync,
      const absl::optional<std::vector<uint8_t>>& _request_data,
      mmmojo::mojom::MMMojoService::ShakeHandHiCallback _callback,
      const absl::optional<std::vector<uint8_t>>& _attach_data);

  explicit MMMojoReadInfo(
      MMMojoInfoMethod _method,
      bool _sync,
      ::mojo::ScopedSharedBufferHandle _shared_memory,
      uint64_t _shared_memory_size,
      const absl::optional<std::vector<uint8_t>>& _attach_data);

  MMMojoReadInfo(const MMMojoReadInfo&) = delete;
  MMMojoReadInfo& operator=(const MMMojoReadInfo&) = delete;

  ~MMMojoReadInfo() = default;

  const MMMojoInfoMethod method;
  const bool sync;
  // kPush & kPull
  absl::optional<std::vector<uint8_t>> request_data;
  // kPull
  absl::optional<mmmojo::mojom::MMMojoService::HiCallback> callback;
  absl::optional<mmmojo::mojom::MMMojoService::ShakeHandHiCallback>
      callbacksync;
  // kShared
  absl::optional<mojo::ScopedSharedBufferHandle> shared_memory;
  absl::optional<mojo::ScopedSharedBufferMapping> shared_buffer;
  absl::optional<uint64_t> shared_memory_size;
  // attach data
  absl::optional<std::vector<uint8_t>> attach_data;
};

// writeinfo
struct MMMojoWriteInfo {
  explicit MMMojoWriteInfo(MMMojoInfoMethod _method,
                           bool _sync,
                           uint32_t _request_id);

  MMMojoWriteInfo(const MMMojoWriteInfo&) = delete;
  MMMojoWriteInfo& operator=(const MMMojoWriteInfo&) = delete;

  ~MMMojoWriteInfo() = default;

  const MMMojoInfoMethod method;
  const bool sync;
  // type
  const uint32_t request_id;
  // kPush & kPull
  absl::optional<std::vector<uint8_t>> request_data;
  // kPull
  absl::optional<mmmojo::mojom::MMMojoService::HiCallback> callback;
  absl::optional<mmmojo::mojom::MMMojoService::ShakeHandHiCallback>
      callbacksync;
  // kShared
  absl::optional<mojo::ScopedSharedBufferHandle> shared_memory;
  absl::optional<mojo::ScopedSharedBufferMapping> shared_buffer;
  absl::optional<uint64_t> shared_memory_size;
  // attach data
  absl::optional<std::vector<uint8_t>> attach_data;

  // append message pipe
  absl::optional<int32_t> message_pipe;
  // sync response
  absl::optional<void**> response_info;
};

// stream
class MMMojoStream {
 public:
  explicit MMMojoStream(const std::string& name,
                        void* user_data,
                        MMMojoEnvironmentCallbacks* delegate);

  MMMojoStream(const MMMojoStream&) = delete;
  MMMojoStream& operator=(const MMMojoStream&) = delete;

  ~MMMojoStream() = default;

  scoped_refptr<base::SingleThreadTaskRunner> GetIOThreadTaskRunner();

  void CreateRemoteAndReceiverStream(mojo::ScopedMessagePipeHandle remote,
                                     mojo::ScopedMessagePipeHandle receiver,
                                     bool set_disconnect_handler,
                                     bool connected);
  void RemoveRemoteAndReceiverStream();

  void PushMMMojoWriteInfo(std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo);
  void PullReqMMMojoWriteInfo(
      std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo);
  void PullRespMMMojoWriteInfo(
      std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo);
  void SharedMMMojoWriteInfo(std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo);

  // sync
  void PushMMMojoWriteInfoSync(
      std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
      bool* success,
      base::WaitableEvent* event);
  void PullReqMMMojoWriteInfoSync(
      std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
      bool* success,
      base::WaitableEvent* event);
  void PullRespMMMojoWriteInfoSync(
      std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo);
  void SharedMMMojoWriteInfoSync(
      std::unique_ptr<MMMojoWriteInfo> mmmojo_writeinfo,
      bool* success,
      base::WaitableEvent* event);

 private:
  void OnPushReqMMMojoReadInfo(
      uint32_t request_id,
      const absl::optional<std::vector<uint8_t>>& request_data,
      const absl::optional<std::vector<uint8_t>>& attach_data);

  // sync
  void OnPushReqMMMojoReadInfoSync(
      uint32_t request_id,
      const absl::optional<std::vector<uint8_t>>& request_data,
      const absl::optional<std::vector<uint8_t>>& attach_data,
      bool success);

  raw_ptr<void> user_data_ = nullptr;
  raw_ptr<MMMojoEnvironmentCallbacks> delegate_ = nullptr;

  std::unique_ptr<MMMojoServiceImpl> impl_;
  std::unique_ptr<base::Thread> thread_;
};

}  // namespace common
}  // namespace mmmojo

#endif  // MMMOJO_COMMON_MMMOJO_STREAM_H_
