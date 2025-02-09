
#ifndef __base_thread_checker_impl_h__
#define __base_thread_checker_impl_h__

#pragma once

#include "base/synchronization/lock.h"
#include "platform_thread.h"

namespace base
{

// Real implementation of ThreadChecker, for use in debug mode, or
// for temporary use in release mode (e.g. to CHECK on a threading issue
// seen only in the wild).
//
// Note: You should almost always use the ThreadChecker class to get the
// right version for your build configuration.
class ThreadCheckerImpl
{
public:
    ThreadCheckerImpl();
    ~ThreadCheckerImpl();

    bool CalledOnValidThread() const;

    // Changes the thread that is checked for in CalledOnValidThread.  This may
    // be useful when an object may be created on one thread and then used
    // exclusively on another thread.
    void DetachFromThread();

private:
    void EnsureThreadIdAssigned() const;

    mutable base::Lock lock_;
    // This is mutable so that CalledOnValidThread can set it.
    // It's guarded by |lock_|.
    mutable PlatformThreadId valid_thread_id_;
};

} //namespace base

#endif //__base_thread_checker_impl_h__