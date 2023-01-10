

#include <memory>

#include "base/lazy_instance.h"
#include "base/threading/thread.h"

#include "mmmojo/common/mmmojo_io_thread.h"

namespace mmmojo {
namespace common {

class MojoIpcSupport {
 public:
  MojoIpcSupport() = default;

  MojoIpcSupport(const MojoIpcSupport&) = delete;
  MojoIpcSupport& operator=(const MojoIpcSupport&) = delete;

  ~MojoIpcSupport() = default;

  void Start() {
    mojo_io_thread_ = std::make_unique<base::Thread>("MMMojo IO Thread");
    mojo_io_thread_->StartWithOptions(
        base::Thread::Options(base::MessagePumpType::IO, 0));
  }
  void Shutdown() { mojo_io_thread_.reset(); }
  scoped_refptr<base::SingleThreadTaskRunner> GetIOThreadTaskRunner() {
    return mojo_io_thread_->task_runner();
  }

 private:
  std::unique_ptr<base::Thread> mojo_io_thread_;
};

base::LazyInstance<MojoIpcSupport>::Leaky mojo_io_initializer;

void InitializeMMMojoIOThread() {
  mojo_io_initializer.Get().Start();
}

scoped_refptr<base::SingleThreadTaskRunner> GetMMMojoIOThreadTaskRunner() {
  return mojo_io_initializer.Get().GetIOThreadTaskRunner();
}

void ShutdownMMMojoIOThread() {
  mojo_io_initializer.Get().Shutdown();
}

}  // namespace common
}  // namespace mmmojo
