#include <pthread.h>
#include <unordered_map>
#include <sys/time.h>                        // timeval, gettimeofday

#include "sys_futex.h"
#include "unique_lock.h"
#include "atomic.h"
#include "timer.h"

#if defined(__APPLE__) || defined(__OSX__)

namespace baidu {
namespace common {


class SimuFutex {
public:
    SimuFutex() : counts(0)
                , ref(0) {
        pthread_mutex_init(&lock, NULL);
        pthread_cond_init(&cond, NULL);
    }
    ~SimuFutex() {
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
    }

public:
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int32_t counts;
    int32_t ref;
};

static pthread_mutex_t s_futex_map_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t init_futex_map_once = PTHREAD_ONCE_INIT;
static std::unordered_map<void*, SimuFutex>* s_futex_map = NULL;
static void InitFutexMap() {
    // Leave memory to process's clean up.
    s_futex_map = new (std::nothrow) std::unordered_map<void*, SimuFutex>();
    if (NULL == s_futex_map) {
        exit(1);
    }
    return;
}

int futex_wait_private(void* addr1, int expected, const timespec* timeout) {
    if (pthread_once(&init_futex_map_once, InitFutexMap) != 0) {
     //   LOG(FATAL) << "Fail to pthread_once";
        exit(1);
    }
    UniqueLock mu(s_futex_map_mutex);
    SimuFutex& simu_futex = (*s_futex_map)[addr1];
    ++simu_futex.ref;
    mu.unlock();

    int rc = 0;
    {
        UniqueLock mu1(simu_futex.lock);
        if (static_cast<std::atomic<int>*>(addr1)->load() == expected) {
            ++simu_futex.counts;
            if (timeout) {
                timespec timeout_abs = timer::timespec_from_now(*timeout);
                if ((rc = pthread_cond_timedwait(&simu_futex.cond, &simu_futex.lock, &timeout_abs)) != 0) {
                    //errno = rc;
                    rc = -1;
                }
            } else {
                if ((rc = pthread_cond_wait(&simu_futex.cond, &simu_futex.lock)) != 0) {
                   // errno = rc;
                    rc = -1;
                }
            }
            --simu_futex.counts;
        } else {
           // errno = EAGAIN;
            rc = -1;
        }
    }

    UniqueLock mu1(s_futex_map_mutex);
    if (--simu_futex.ref == 0) {
        s_futex_map->erase(addr1);
    }
    mu1.unlock();
    return rc;
}

int futex_wake_private(void* addr1, int nwake) {
    if (pthread_once(&init_futex_map_once, InitFutexMap) != 0) {
        //LOG(FATAL) << "Fail to pthread_once";
        exit(1);
    }
    UniqueLock mu(s_futex_map_mutex);
    auto it = s_futex_map->find(addr1);
    if (it == s_futex_map->end()) {
        mu.unlock();
        return 0;
    }
    SimuFutex& simu_futex = it->second;
    ++simu_futex.ref;
    mu.unlock();

    int nwakedup = 0;
    int rc = 0;
    {
        UniqueLock mu1(simu_futex.lock);
        nwake = (nwake < simu_futex.counts)? nwake: simu_futex.counts;
        for (int i = 0; i < nwake; ++i) {
            if ((rc = pthread_cond_signal(&simu_futex.cond)) != 0) {
               // errno = rc;
                break;
            } else {
                ++nwakedup;
            }
        }
    }

    UniqueLock mu2(s_futex_map_mutex);
    if (--simu_futex.ref == 0) {
        s_futex_map->erase(addr1);
    }
    mu2.unlock();
    return nwakedup;
}

} // namespace common
} // namespace baidu

#endif