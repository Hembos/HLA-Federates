#include "panel.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#include <MessageBuffer.hh>

AppCore::AppCore(QObject* parent) : QObject(parent)
{
    panel = new Panel(L"PumpFederation", L"Panel");
    timer = new QTimer();
    std::cout << "Timer connection" << std::endl;
    connect(timer, SIGNAL(timeout()), this, SLOT(changeAtrInQml()));
    timer->start();
    std::cout << "Timer started" << std::endl;
}

void AppCore::changeAtrInQml()
{
    panel->run();
    emit sendToQml(panel->atr1);
}

Panel::Panel(const std::wstring& federation_name, const std::wstring& federate_name)
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

    if (has_created) {
        register_sync_point(L"Start");
    }

    synchronize(L"Start");
    waitForSynchronization(L"Start");
}

Panel::~Panel()
{
    resignAndDelete();
}

void Panel::resignAndDelete()
{
    my_ambassador->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);

    try {
        my_ambassador->destroyFederationExecution(federation_name);
    }
    catch (FederatesCurrentlyJoined& e) {
    }
}

void Panel::createOrJoin()
{
    try {
        my_ambassador->createFederationExecution(federation_name, L"Base.xml");

        has_created = true;

        std::cout << "Created federation" << std::endl;
    }
    catch (FederationExecutionAlreadyExists& e) {
        std::cout << "Federation already exists" << std::endl;
    }

    std::wcout << federation_name << std::endl;

    std::vector<std::wstring> modules;

    modules.emplace_back(L"Objects.xml");

    std::wcout << federate_name << std::endl;

    my_handle = my_ambassador->joinFederationExecution(federate_name, L"Panel", federation_name, modules);
    std::wcout << federate_name << std::endl;

}

void Panel::run()
{
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

    std::wcout << my_local_time.toString() << std::endl;
}

void Panel::register_sync_point(const std::wstring& label)
{
    std::wstring tag {L"hi"};
    my_ambassador->registerFederationSynchronizationPoint(label, {tag.c_str(), tag.size()});
}

void Panel::synchronize(const std::wstring& label)
{
    waitForAnnounce(label);

    my_ambassador->synchronizationPointAchieved(label, true);
}

void Panel::waitForAnnounce(const std::wstring& label)
{
    while (!hasSynchronizationPending(label)) {
        tick();
    }
}

bool Panel::hasSynchronizationPending(const std::wstring& label)
{
    auto find_it = std::find(std::begin(my_synchronization_points), std::end(my_synchronization_points), label);

    return find_it != end(my_synchronization_points);
}

void Panel::tick()
{
    my_ambassador->evokeCallback(1.0);
}

void Panel::publishAndSubscribe()
{
    auto pump_handle = my_ambassador->getObjectClassHandle(L"Pump");

    AttributeHandleSet attributes;
    attributes.insert(my_ambassador->getAttributeHandle(pump_handle, L"atr1"));
    attributes.insert(my_ambassador->getAttributeHandle(pump_handle, L"atr2"));

    my_ambassador->subscribeObjectClassAttributes(pump_handle, attributes);
}

void Panel::enableTimeRegulation()
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

void Panel::waitForTimeAdvanceGrant()
{
    while (!my_time_granted) {
        tick();
    }
}

void Panel::waitForSynchronization(const std::wstring& label)
{
    while (hasSynchronizationPending(label)) {
        tick();
    }
}

void Panel::announceSynchronizationPoint(const std::wstring& label,
                                         const VariableLengthData& /*theUserSuppliedTag*/) throw(FederateInternalError)
{
    std::wcout << __func__ << ", label:" << label << std::endl;
    my_synchronization_points.push_back(label);
}

void Panel::federationSynchronized(const std::wstring& label,
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

void Panel::timeConstrainedEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError)
{
    std::wcout << __func__ << ", time=" << theFederateTime.toString() << std::endl;
    my_is_time_constrained = true;
}

void Panel::timeRegulationEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError)
{
    std::wcout << __func__ << ", time=" << theFederateTime.toString() << std::endl;
    my_is_time_regulated = true;
}

void Panel::timeAdvanceGrant(const LogicalTime& theTime) throw(FederateInternalError)
{
    my_local_time = theTime;
    my_time_granted = true;
}

void Panel::reflectAttributeValues(rti1516e::ObjectInstanceHandle theObject,
                                   const AttributeHandleValueMap& theAttributeValues,
                                   const rti1516e::VariableLengthData& /*theUserSuppliedTag*/,
                                   rti1516e::OrderType /*sentOrder*/,
                                   rti1516e::TransportationType /*theType*/,
                                   const rti1516e::LogicalTime& /*theTime*/,
                                   rti1516e::OrderType /*receivedOrder*/,
                                   rti1516e::MessageRetractionHandle /*theHandle*/,
                                   rti1516e::SupplementalReflectInfo /*theReflectInfo*/) throw(FederateInternalError)
{
    static const auto pump_handle = my_ambassador->getObjectClassHandle(L"Pump");
    static const auto atr1_handle = my_ambassador->getAttributeHandle(pump_handle, L"atr1");
    static const auto atr2_handle = my_ambassador->getAttributeHandle(pump_handle, L"atr2");

    for (const auto& kv : theAttributeValues) {
        libhla::MessageBuffer mb;
        mb.resize(kv.second.size());
        mb.reset();
        std::memcpy(static_cast<char*>(mb(0)), kv.second.data(), kv.second.size());
        mb.assumeSizeFromReservedBytes();
        //std:: cout << mb.read_double() << std::endl;
        this->atr1 = mb.read_double();
        std:: cout << this->atr1 << std::endl;
    }
}
