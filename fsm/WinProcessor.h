#ifndef WINPROCESSOR_H
#define WINPROCESSOR_H

#include "WinWorker.h"
#include "Singleton.hpp"

#include <string>

namespace Net
{
namespace Protocol
{
class ProcessorSensor;
}
}

namespace Processor
{
class WinWorker;

class WinProcessor
{
public:
    friend class Net::Protocol::ProcessorSensor;

    WinProcessor(const unsigned theThreadCount = 1);
    WinProcessor(const std::string& theName, const unsigned theThreadCount);
    ~WinProcessor();

    void start();
    void waitStop();
    void stop();

    struct min_heap_item_t* addLocalTimer(
        const unsigned long long theId,
        const struct timeval& theInterval,
        TimeoutFn theCallback,
        void* theArg)
    {
        unsigned workerId = theId % threadCountM;
        return workersM[workerId].addLocalTimer(theInterval, theCallback, theArg);
    }
    inline void cancelLocalTimer(
        const unsigned long long theId,
        struct min_heap_item_t*& theEvent)
    {
        unsigned workerId = theId % threadCountM;
        return workersM[workerId].cancelLocalTimer(theEvent);
    }

    inline int process(
        const unsigned long long theId,
        IJob* job)
    {
        unsigned workerId = theId % threadCountM;
        return workersM[workerId].process(job);
    }


private:
    unsigned threadCountM;
    WinWorker* workersM;
    PlatformThreadHandle* threadsM;
    std::string nameM;
    Lock threadMutexM;
};
}
#define NEW_PROCESSOR_JOB(...) new Processor::IJob(boost::bind(__VA_ARGS__))


#define g_bg_processor (DesignPattern::Singleton<Processor::WinProcessor, 0>::instance())

#endif /* WINPROCESSOR_H */

