
// This file defines the type used to provide details for NotificationService
// notifications.

#ifndef __notification_details_h__
#define __notification_details_h__

#pragma once

#include "base/basic_types.h"

// Do not declare a NotificationDetails directly--use either
// "Details<detailsclassname>(detailsclasspointer)" or
// NotificationService::NoDetails().
class NotificationDetails
{
public:
    NotificationDetails();
    NotificationDetails(const NotificationDetails& other);
    ~NotificationDetails();

    // NotificationDetails can be used as the index for a map; this method
    // returns the pointer to the current details as an identifier, for use as a
    // map index.
    uintptr_t map_key() const
    {
        return reinterpret_cast<uintptr_t>(ptr_);
    }

    bool operator!=(const NotificationDetails& other) const
    {
        return ptr_ != other.ptr_;
    }

    bool operator==(const NotificationDetails& other) const
    {
        return ptr_ == other.ptr_;
    }

protected:
    explicit NotificationDetails(const void* ptr);

    // Declaring this const allows Details<T> to be used with both T = Foo and
    // T = const Foo.
    const void* ptr_;
};

template<class T>
class Details : public NotificationDetails
{
public:
    // TODO(erg): Our code hard relies on implicit conversion
    Details(T* ptr) : NotificationDetails(ptr) {} // NOLINT
    Details(const NotificationDetails& other) // NOLINT
        : NotificationDetails(other) {}

    T* operator->() const
    {
        return ptr();
    }
    // The casts here allow this to compile with both T = Foo and T = const Foo.
    T* ptr() const
    {
        return static_cast<T*>(const_cast<void*>(ptr_));
    }
};

#endif //__notification_details_h__