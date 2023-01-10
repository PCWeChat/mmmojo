

#ifndef MMMOJO_COMMON_MMMOJO_IO_THREAD_H_
#define MMMOJO_COMMON_MMMOJO_IO_THREAD_H_

#include "base/memory/scoped_refptr.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace mmmojo {
namespace common {

void InitializeMMMojoIOThread();
scoped_refptr<base::SingleThreadTaskRunner> GetMMMojoIOThreadTaskRunner();
void ShutdownMMMojoIOThread();

}  // namespace common
}  // namespace mmmojo

#endif  // MMMOJO_COMMON_MMMOJO_IO_THREAD_H_
