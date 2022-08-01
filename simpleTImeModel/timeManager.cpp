#include "timeManager.h"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <chrono>

#include "MessageBuffer.hh"

Time::Time(const std::wstring& federation_name, const std::wstring& federate_name)
    : Federate(federation_name, federate_name)
{
    createOrJoin();

    if (has_created) {
        register_sync_point(L"Init");

        std::wcout << "Press ENTER when all federates have joined" << std::endl;
        getchar();
    }

    synchronize(L"Init");
    waitForSynchronization(L"Init");

    publishAndSubscribe();

    enableTimeRegulation();

    timeOblect_handle
        = ambassador->registerObjectInstance(ambassador->getObjectClassHandle(L"Time"), federateName + L"_Time");

    if (has_created) {
        register_sync_point(L"Start");
    }

    synchronize(L"Start");
    waitForSynchronization(L"Start");
}

Time::~Time()
{
    resignAndDelete();
}

void Time::createOrJoin()
{
    try {
        ambassador->createFederationExecution(federationName, L"base.xml");
        has_created = true;
        std::cout << "Federation created" << std::endl;
    }
    catch (FederationExecutionAlreadyExists& e) {
        std::cerr << "Federation execution already exists" << std::endl;
    }

    std::vector<std::wstring> modules;

    modules.emplace_back(L"objects.xml");

    modules.emplace_back(L"interactions.xml");

    handle = ambassador->joinFederationExecution(federateName, L"Time", federationName, modules);
}

void Time::resignAndDelete()
{
    ambassador->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);

    try {
        ambassador->destroyFederationExecution(federationName);
        std::wcout << "Federation " << federationName << " destroyed" << std::endl;
    }
    catch (FederatesCurrentlyJoined& e) {
        std::cerr << "Federates currently joined" << std::endl;
    }
}

void Time::publishAndSubscribe()
{
    std::wcout << __func__ << std::endl;

    auto time_handle = ambassador->getObjectClassHandle(L"Time");
    std::wcout << "Time handle: " << time_handle << std::endl;

    AttributeHandleSet attributes;
    attributes.insert(ambassador->getAttributeHandle(time_handle, L"DT"));

    ambassador->publishObjectClassAttributes(time_handle, attributes);
}

void Time::run()
{
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    auto t1 = high_resolution_clock::now();

    while (true) {


        std::wcout << "time at start is " << currentTime.toString() << std::endl;

        ambassador->queryLogicalTime(currentTime);

        std::wcout << "after query, time is " << currentTime.toString() << std::endl;

        auto time_aux = RTI1516fedTime {currentTime};
        time_aux += lookahead;

        std::wcout << "request advance to " << time_aux.toString() << std::endl;

        timeGranted = false;
        ambassador->timeAdvanceRequest(time_aux);


        waitForTimeAdvanceGrant();

        std::wcout << "after wait for grant, time is " << currentTime.toString() << std::endl;

        auto next_step = RTI1516fedTime {currentTime};
        next_step += lookahead;

        timeGranted = false;

        tick();
        auto curTime = high_resolution_clock::now();
        duration<double, std::milli> ms = (curTime - t1);
        std::wcout << ms.count() << L" ms" << std::endl;
        dt = ms.count() / 1000;
        t1 = curTime;
        sendNewDT(next_step);
    }
}

void Time::sendNewDT(const LogicalTime& time)
{
    static const auto time_handle = ambassador->getObjectClassHandle(L"Time");
    static const auto attribute_DT_handle = ambassador->getAttributeHandle(time_handle, L"DT");

    libhla::MessageBuffer buffer;

    AttributeHandleValueMap attributes;

    buffer.reset();
    buffer.write_double(dt);
    buffer.updateReservedBytes();
    attributes[attribute_DT_handle] = VariableLengthData(static_cast<char*>(buffer(0)), buffer.size());

    std::wstring tag{L""};
    ambassador->updateAttributeValues(timeOblect_handle, attributes, {tag.c_str(), tag.size()}, time);
}
