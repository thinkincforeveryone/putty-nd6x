
#ifndef __base_non_thread_safe_impl_h__
#define __base_non_thread_safe_impl_h__

#pragma once

#include "thread_checker_impl.h"

namespace base
{

// Full implementation of NonThreadSafe, for debug mode or for occasional
// temporary use in release mode e.g. when you need to CHECK on a thread
// bug that only occurs in the wild.
//
// Note: You should almost always use the NonThreadSafe class to get
// the right version of the class for your build configuration.
class NonThreadSafeImpl
{
public:
    ~NonThreadSafeImpl();

    bool CalledOnValidThread() const;

protected:
    // Changes the thread that is checked for in CalledOnValidThread. The next
    // call to CalledOnValidThread will attach this class to a new thread. It is
    // up to the NonThreadSafe derived class to decide to expose this or not.
    // This may be useful when an object may be created on one thread and then
    // used exclusively on another thread.
    void DetachFromThread();

private:
    ThreadCheckerImpl thread_checker_;
};

} //namespace base

#endif //__base_non_thread_safe_impl_h__