#ifndef BAIDU_COMMON_SYS_FUTEX_H_
#define BAIDU_COMMON_SYS_FUTEX_H_

#include <unistd.h>                     // syscall
#include <time.h>                       // timespec
#if defined(__LINUX__)
#include <syscall.h>                    // SYS_futex
#include <linux/futex.h>                // FUTEX_WAIT, FUTEX_WAKE
namespace baidu {
namespace common {

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif

inline int futex_wait_private(
    void* addr1, int expected, const timespec* timeout) {
    return syscall(__NR_futex, addr1, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG),
                   expected, timeout, NULL, 0);
}

inline int futex_wake_private(void* addr1, int nwake) {
    return syscall(__NR_futex, addr1, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG),
                   nwake, NULL, NULL, 0);
}

inline int futex_requeue_private(void* addr1, int nwake, void* addr2) {
    return syscall(__NR_futex, addr1, (FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG),
                   nwake, NULL, addr2, 0);
}

}  // namespace common

} // namespace baidu

#elif defined(__APPLE__) || defined(__OSX__)

namespace baidu {
namespace common {

int futex_wait_private(void* addr1, int expected, const timespec* timeout);

int futex_wake_private(void* addr1, int nwake);

int futex_requeue_private(void* addr1, int nwake, void* addr2);

}  // namespace common

} // namespace baidu

#else

#endif

#endif // SYS_FUTEX_H