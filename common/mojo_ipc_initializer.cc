

#include <memory>

#include "mojo/core/embedder/configuration.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"

#include "base/lazy_instance.h"
#include "base/threading/thread.h"

#include "mmmojo/common/mojo_ipc_initializer.h"

namespace mmmojo {
namespace common {

namespace {

// Initializes and owns mojo.
// https://source.chromium.org/chromium/chromium/src/+/main:components/viz/demo/demo_main.cc

// https://source.chromium.org/chromium/chromium/src/+/main:content/app/mojo/mojo_init.cc
class MojoInitializer {
 public:
  MojoInitializer() { mojo::core::Init(); }
};

base::LazyInstance<MojoInitializer>::Leaky mojo_initializer;

// https://source.chromium.org/chromium/chromium/src/+/main:content/app/mojo_ipc_support.h
class MojoIpcSupport {
 public:
  MojoIpcSupport() = default;

  MojoIpcSupport(const MojoIpcSupport&) = delete;
  MojoIpcSupport& operator=(const MojoIpcSupport&) = delete;

  ~MojoIpcSupport() = default;

  void Start() {
    mojo_ipc_thread_ = std::make_unique<base::Thread>("MMMojo IPC Thread");
    mojo_ipc_thread_->StartWithOptions(
        base::Thread::Options(base::MessagePumpType::IO, 0));
    mojo_ipc_support_ = std::make_unique<mojo::core::ScopedIPCSupport>(
        mojo_ipc_thread_->task_runner(),
        mojo::core::ScopedIPCSupport::ShutdownPolicy::FAST);
  }
  void Shutdown() {
    mojo_ipc_support_.reset();
    mojo_ipc_thread_.reset();
  }

 private:
  std::unique_ptr<base::Thread> mojo_ipc_thread_;
  std::unique_ptr<mojo::core::ScopedIPCSupport> mojo_ipc_support_;
};

base::LazyInstance<MojoIpcSupport>::Leaky mojo_ipc_initializer;

}  //  namespace

void InitializeMojo() {
  mojo_initializer.Get();
}

void InitializeMojoIPC() {
  mojo_ipc_initializer.Get().Start();
}

void ShutdownMojoIPC() {
  mojo_ipc_initializer.Get().Shutdown();
}

}  // namespace common
}  // namespace mmmojo
