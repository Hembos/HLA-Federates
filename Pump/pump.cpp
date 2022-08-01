#include "pump.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#include <MessageBuffer.hh>

Pump::Pump(const std::wstring& federation_name, const std::wstring& federate_name)
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

    pump_object_handle = my_ambassador->registerObjectInstance(my_ambassador->getObjectClassHandle(L"Pump"), L"Pump");

    if (has_created) {
        register_sync_point(L"Start");
    }

    synchronize(L"Start");
    waitForSynchronization(L"Start");
}

Pump::~Pump()
{
    resignAndDelete();
}

void Pump::resignAndDelete()
{
    my_ambassador->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);

    try {
        my_ambassador->destroyFederationExecution(federation_name);
    }
    catch (FederatesCurrentlyJoined& e) {
    }
}

void Pump::createOrJoin()
{
    try {
        my_ambassador->createFederationExecution(federation_name, L"Base.xml");

        has_created = true;

        std::cout << "Created federation" << std::endl;
    }
    catch (FederationExecutionAlreadyExists& e) {
    }

    std::wcout << federation_name << std::endl;

    std::vector<std::wstring> modules;

    modules.emplace_back(L"Interactions.xml");
    modules.emplace_back(L"Objects.xml");

    my_handle = my_ambassador->joinFederationExecution(federate_name, L"Pump", federation_name, modules);
}

void Pump::run()
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

        sendNewAtrs(next_step);
        std::wcout << my_local_time.toString() << std::endl;
    }

    std::cout << atr1 << std::endl;
    std::cout << atr2 << std::endl;
}

void Pump::register_sync_point(const std::wstring& label)
{
    std::wstring tag {L"hi"};
    my_ambassador->registerFederationSynchronizationPoint(label, {tag.c_str(), tag.size()});
}

void Pump::synchronize(const std::wstring& label)
{
    waitForAnnounce(label);

    my_ambassador->synchronizationPointAchieved(label, true);
}

void Pump::waitForAnnounce(const std::wstring& label)
{
    while (!hasSynchronizationPending(label)) {
        tick();
    }
}

bool Pump::hasSynchronizationPending(const std::wstring& label)
{
    auto find_it = std::find(std::begin(my_synchronization_points), std::end(my_synchronization_points), label);

    return find_it != end(my_synchronization_points);
}

void Pump::tick()
{
    my_ambassador->evokeCallback(1.0);
}

void Pump::publishAndSubscribe()
{
    auto pump_handle = my_ambassador->getObjectClassHandle(L"Pump");

    AttributeHandleSet attributes;
    attributes.insert(my_ambassador->getAttributeHandle(pump_handle, L"atr1"));
    attributes.insert(my_ambassador->getAttributeHandle(pump_handle, L"atr2"));

    my_ambassador->publishObjectClassAttributes(pump_handle, attributes);

    auto changer_handle = my_ambassador->getInteractionClassHandle(L"ChangeAttrs");
    my_ambassador->subscribeInteractionClass(changer_handle);
}

void Pump::enableTimeRegulation()
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

void Pump::receiveInteraction(rti1516e::InteractionClassHandle theInteraction,
                              const ParameterHandleValueMap& theParameterValues,
                              const rti1516e::VariableLengthData& /*theUserSuppliedTag*/,
                              rti1516e::OrderType /*sentOrder*/,
                              rti1516e::TransportationType /*theType*/,
                              const rti1516e::LogicalTime& /*theTime*/,
                              rti1516e::OrderType /*receivedOrder*/,
                              rti1516e::MessageRetractionHandle /*theHandle*/,
                              rti1516e::SupplementalReceiveInfo /*theReceiveInfo*/) throw(FederateInternalError)
{
    std::cout << "Interaction" << std::endl;

    static const auto changer_handle = my_ambassador->getInteractionClassHandle(L"ChangeAttrs");
    static const auto param_dx1_handle = my_ambassador->getParameterHandle(changer_handle, L"dx1");
    static const auto param_dx2_handle = my_ambassador->getParameterHandle(changer_handle, L"dx2");

    if (theInteraction == changer_handle) {
        libhla::MessageBuffer mb;

        auto param_dx = theParameterValues.at(param_dx1_handle);

        mb.resize(param_dx.size());
        mb.reset();
        std::memcpy(static_cast<char*>(mb(0)), param_dx.data(), param_dx.size());
        mb.assumeSizeFromReservedBytes();

        atr1 += mb.read_double();

        auto param_dy = theParameterValues.at(param_dx2_handle);

        mb.resize(param_dy.size());
        mb.reset();
        std::memcpy(static_cast<char*>(mb(0)), param_dy.data(), param_dy.size());
        mb.assumeSizeFromReservedBytes();

        atr2 += mb.read_double();
    }
}

void Pump::receiveInteraction(rti1516e::InteractionClassHandle theInteraction,
                              const ParameterHandleValueMap& /*theParameterValues*/,
                              const rti1516e::VariableLengthData& /*theUserSuppliedTag*/,
                              rti1516e::OrderType /*sentOrder*/,
                              rti1516e::TransportationType /*theType*/,
                              const rti1516e::LogicalTime& /*theTime*/,
                              rti1516e::OrderType /*receivedOrder*/,
                              rti1516e::SupplementalReceiveInfo /*theReceiveInfo*/) throw(FederateInternalError)
{
    std::wcout << "########        " << __func__ << " 2, theInteraction=" << theInteraction << std::endl;
    getchar();
}

void Pump::receiveInteraction(rti1516e::InteractionClassHandle theInteraction,
                              const ParameterHandleValueMap& /*theParameterValues*/,
                              const rti1516e::VariableLengthData& /*theUserSuppliedTag*/,
                              rti1516e::OrderType /*sentOrder*/,
                              rti1516e::TransportationType /*theType*/,
                              rti1516e::SupplementalReceiveInfo /*theReceiveInfo*/) throw(FederateInternalError)
{
    std::wcout << "########        " << __func__ << " 3, theInteraction=" << theInteraction << std::endl;
    getchar();
}

void Pump::waitForTimeAdvanceGrant()
{
    while (!my_time_granted) {
        tick();
    }
}

void Pump::sendNewAtrs(const LogicalTime& time)
{
    static const auto pump_handle = my_ambassador->getObjectClassHandle(L"Pump");
    static const auto atr1_handle = my_ambassador->getAttributeHandle(pump_handle, L"atr1");
    static const auto atr2_handle = my_ambassador->getAttributeHandle(pump_handle, L"atr2");

    libhla::MessageBuffer buffer;

    AttributeHandleValueMap attributes;

    buffer.reset();
    buffer.write_double(atr1);
    buffer.updateReservedBytes();
    attributes[atr1_handle] = VariableLengthData(static_cast<char*>(buffer(0)), buffer.size());

    buffer.reset();
    buffer.write_double(atr2);
    buffer.updateReservedBytes();
    attributes[atr2_handle] = VariableLengthData(static_cast<char*>(buffer(0)), buffer.size());

    std::wstring tag {L""};
    my_ambassador->updateAttributeValues(pump_object_handle, attributes, {tag.c_str(), tag.size()}, time);
}

void Pump::waitForSynchronization(const std::wstring& label)
{
    while (hasSynchronizationPending(label)) {
        tick();
    }
}

void Pump::announceSynchronizationPoint(const std::wstring& label,
                                        const VariableLengthData& /*theUserSuppliedTag*/) throw(FederateInternalError)
{
    std::wcout << __func__ << ", label:" << label << std::endl;
    my_synchronization_points.push_back(label);
}

void Pump::federationSynchronized(const std::wstring& label,
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

void Pump::timeConstrainedEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError)
{
    std::wcout << __func__ << ", time=" << theFederateTime.toString() << std::endl;
    my_is_time_constrained = true;
}

void Pump::timeRegulationEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError)
{
    std::wcout << __func__ << ", time=" << theFederateTime.toString() << std::endl;
    my_is_time_regulated = true;
}

void Pump::timeAdvanceGrant(const LogicalTime& theTime) throw(FederateInternalError)
{
    my_local_time = theTime;
    my_time_granted = true;
}