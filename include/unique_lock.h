
#ifndef  COMMON_UNIQUE_LOCK_H_
#define  COMMON_UNIQUE_LOCK_H_

#include <mutex>
#include <pthread.h>
namespace baidu {

namespace common {

class UniqueLock {
public:
    typedef pthread_mutex_t         mutex_type;
    UniqueLock() : _mutex(NULL), _owns_lock(false) {}
    explicit UniqueLock(mutex_type& mutex)
        : _mutex(&mutex), _owns_lock(true) {
        pthread_mutex_lock(_mutex);
    }
    UniqueLock(const UniqueLock&) = delete;
    void operator=(const UniqueLock&) = delete;
    
    UniqueLock(mutex_type& mutex, std::defer_lock_t)
        : _mutex(&mutex), _owns_lock(false)
    {}
    UniqueLock(mutex_type& mutex, std::try_to_lock_t) 
        : _mutex(&mutex), _owns_lock(pthread_mutex_trylock(&mutex) == 0)
    {}
    UniqueLock(mutex_type& mutex, std::adopt_lock_t) 
        : _mutex(&mutex), _owns_lock(true)
    {}

    ~UniqueLock() {
        if (_owns_lock) {
            pthread_mutex_unlock(_mutex);
        }
    }

    void lock() {
        if (_owns_lock) {
            //CHECK(false) << "Detected deadlock issue";     
            return;
        }
#if !defined(NDEBUG)
        const int rc = pthread_mutex_lock(_mutex);
        if (rc) {
            //LOG(FATAL) << "Fail to lock pthread_mutex=" << _mutex << ", " << berror(rc);
            return;
        }
        _owns_lock = true;
#else
        _owns_lock = true;
        pthread_mutex_lock(_mutex);
#endif  // NDEBUG
    }

    bool try_lock() {
        if (_owns_lock) {
            //CHECK(false) << "Detected deadlock issue";     
            return false;
        }
        _owns_lock = !pthread_mutex_trylock(_mutex);
        return _owns_lock;
    }

    void unlock() {
        if (!_owns_lock) {
            //CHECK(false) << "Invalid operation";
            return;
        }
        pthread_mutex_unlock(_mutex);
        _owns_lock = false;
    }

    void swap(UniqueLock& rhs) {
        std::swap(_mutex, rhs._mutex);
        std::swap(_owns_lock, rhs._owns_lock);
    }

    mutex_type* release() {
        mutex_type* saved_mutex = _mutex;
        _mutex = NULL;
        _owns_lock = false;
        return saved_mutex;
    }

    mutex_type* mutex() { return _mutex; }
    bool owns_lock() const { return _owns_lock; }
    operator bool() const { return owns_lock(); }

private:
    mutex_type*                     _mutex;
    bool                            _owns_lock;
};

}
}
#endif