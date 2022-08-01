#include "changer.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#include <MessageBuffer.hh>

Changer::Changer(const std::wstring& federation_name, const std::wstring& federate_name)
    : federation_name(federation_name), federate_name(federate_name)
{
    my_ambassador->connect(*this, HLA_EVOKED);

    createOrJoin();

    if (has_created) {
        register_sync_point(L"Init");
        std::wcout << "Press ENTER when all federates have joined" << std::endl;
        getchar();
    }

    std::cout << "Init" << std::endl;
    synchronize(L"Init");
    std::cout << "waitForSynchronization" << std::endl;
    waitForSynchronization(L"Init");
    std::cout << "InitEnd" << std::endl;

    std::cout << "publishAndSubscribe" << std::endl;
    publishAndSubscribe();
    std::cout << "publishAndSubscribeEnd" << std::endl;

    std::cout << "enableTimeRegulation" << std::endl;
    enableTimeRegulation();
    std::cout << "enableTimeRegulationEnd" << std::endl;

    std::cout << "tick" << std::endl;
    tick();
    std::cout << "tickEnd" << std::endl;

    //pump_object_handle = my_ambassador->registerObjectInstance(my_ambassador->getObjectClassHandle(L"Pump"), L"Pump");

    if (has_created) {
        register_sync_point(L"Start");
    }

    synchronize(L"Start");
    waitForSynchronization(L"Start");
}

Changer::~Changer()
{
    resignAndDelete();
}

void Changer::resignAndDelete()
{
    my_ambassador->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);

    try {
        my_ambassador->destroyFederationExecution(federation_name);
    }
    catch (FederatesCurrentlyJoined& e) {
    }
}

void Changer::createOrJoin()
{
    try {
        my_ambassador->createFederationExecution(federation_name, L"Base.xml");

        has_created = true;

        std::cout << "Created federation" << std::endl;
    }
    catch (FederationExecutionAlreadyExists& e) {
    }

    std::wcout << federation_name << std::endl;
    std::wcout << federate_name << std::endl;

    std::vector<std::wstring> modules;

    modules.emplace_back(L"Interactions.xml");

    my_handle = my_ambassador->joinFederationExecution(federate_name, L"Changer", federation_name, modules);
}

void Changer::run()
{
    while (true) {
        my_time_granted = false;

        std::wcout << "time at start is " << my_local_time.toString() << std::endl;

        my_ambassador->queryLogicalTime(my_local_time);

        std::wcout << "after query, time is " << my_local_time.toString() << std::endl;

        auto time_aux = RTI1516fedTime {my_local_time};
        time_aux += my_time_interval;

        std::wcout << "request advance to " << time_aux.toString() << std::endl;

        my_time_granted = false;
        my_ambassador->timeAdvanceRequest(time_aux);

        waitForTimeAdvanceGrant();

        std::wcout << "after wait for grant, time is " << my_local_time.toString() << std::endl;

        auto next_step = RTI1516fedTime {my_local_time};
        next_step += my_time_interval;

        sendDx(next_step);
        std::wcout << my_local_time.toString() << std::endl;
    }
}

void Changer::register_sync_point(const std::wstring& label)
{
    std::wstring tag {L"hi"};
    my_ambassador->registerFederationSynchronizationPoint(label, {tag.c_str(), tag.size()});
}

void Changer::synchronize(const std::wstring& label)
{
    waitForAnnounce(label);

    my_ambassador->synchronizationPointAchieved(label, true);
}

void Changer::waitForAnnounce(const std::wstring& label)
{
    while (!hasSynchronizationPending(label)) {
        tick();
    }
}

bool Changer::hasSynchronizationPending(const std::wstring& label)
{
    auto find_it = std::find(std::begin(my_synchronization_points), std::end(my_synchronization_points), label);

    return find_it != end(my_synchronization_points);
}

void Changer::tick()
{
    my_ambassador->evokeCallback(1.0);
}

void Changer::publishAndSubscribe()
{
    auto changer_handle = my_ambassador->getInteractionClassHandle(L"ChangeAttrs");
    my_ambassador->publishInteractionClass(changer_handle);
}

void Changer::enableTimeRegulation()
{
    my_ambassador->enableTimeConstrained();

    while (!my_is_time_constrained) {
        tick();
    }

    my_ambassador->enableTimeRegulation(my_time_interval);

    while (!my_is_time_regulated) {
        tick();
    }
}

void Changer::waitForTimeAdvanceGrant()
{
    while (!my_time_granted) {
        tick();
    }
}

void Changer::waitForSynchronization(const std::wstring& label)
{
    while (hasSynchronizationPending(label)) {
        tick();
    }
}

void Changer::announceSynchronizationPoint(const std::wstring& label,
                                        const VariableLengthData& /*theUserSuppliedTag*/) throw(FederateInternalError)
{
    std::wcout << __func__ << ", label:" << label << std::endl;
    my_synchronization_points.push_back(label);
}

void Changer::federationSynchronized(const std::wstring& label,
                                  const FederateHandleSet& /*failedToSyncSet*/) throw(FederateInternalError)
{
    std::wcout << __func__ << ", label:" << label << std::endl;
    std::wcout << L"Federation synchronized on label " << label << std::endl;
    auto find_it = std::find(begin(my_synchronization_points), end(my_synchronization_points), label);
    if (find_it == end(my_synchronization_points)) {
        throw FederateInternalError(L"Synchronization point <" + label + L"> achieved but never announced");
    }
    my_synchronization_points.erase(find_it);
}

void Changer::timeConstrainedEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError)
{
    std::wcout << __func__ << ", time=" << theFederateTime.toString() << std::endl;
    my_is_time_constrained = true;
}

void Changer::timeRegulationEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError)
{
    std::wcout << __func__ << ", time=" << theFederateTime.toString() << std::endl;
    my_is_time_regulated = true;
}

void Changer::timeAdvanceGrant(const LogicalTime& theTime) throw(FederateInternalError)
{
    my_local_time = theTime;
    my_time_granted = true;
}

void Changer::sendDx(const LogicalTime& time)
{
    static const auto collision_handle = my_ambassador->getInteractionClassHandle(L"ChangeAttrs");
    static const auto param_dx1_handle = my_ambassador->getParameterHandle(collision_handle, L"dx1");
    static const auto param_dy2_handle = my_ambassador->getParameterHandle(collision_handle, L"dx2");
    libhla::MessageBuffer buffer;

    ParameterHandleValueMap parameters;

    buffer.reset();
    buffer.write_double(dx1);
    buffer.updateReservedBytes();
    parameters[param_dx1_handle] = VariableLengthData(static_cast<char*>(buffer(0)), buffer.size());

    buffer.reset();
    buffer.write_double(dx2);
    buffer.updateReservedBytes();
    parameters[param_dy2_handle] = VariableLengthData(static_cast<char*>(buffer(0)), buffer.size());

    std::wstring tag{L""};
    my_ambassador->sendInteraction(collision_handle, parameters, {tag.c_str(), tag.size()}, time);
}