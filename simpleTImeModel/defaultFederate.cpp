#include "defaultFederate.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "MessageBuffer.hh"

Federate::Federate(const std::wstring& federationName, const std::wstring& federateName)
: federationName(federationName), federateName(federateName)
{
    ambassador->connect(*this, HLA_EVOKED);
}

Federate::~Federate()
{

}

void Federate::register_sync_point(const std::wstring& label)
{
    std::wcout << __func__ << ", label:" << label << std::endl;

    std::wstring tag{L""};
    ambassador->registerFederationSynchronizationPoint(label, {tag.c_str(), tag.size()});
}

void Federate::synchronize(const std::wstring& label)
{
    std::wcout << __func__ << ", label:" << label << std::endl;

    waitForAnnounce(label);

    ambassador->synchronizationPointAchieved(label, true);
}

void Federate::waitForAnnounce(const std::wstring& label)
{
    std::wcout << __func__ << ", label:" << label << std::endl;

    while (!hasSynchronizationPending(label)) {
        tick();
    }
}

void Federate::tick()
{
    ambassador->evokeCallback(0.04);
}

bool Federate::hasSynchronizationPending(const std::wstring& label)
{
    auto find_it = std::find(std::begin(synchronization_points), std::end(synchronization_points), label);

    return find_it != std::end(synchronization_points);
}

void Federate::announceSynchronizationPoint(
    const std::wstring& label, const VariableLengthData& /*theUserSuppliedTag*/) throw(FederateInternalError)
{
    std::wcout << __func__ << ", label:" << label << std::endl;
    synchronization_points.push_back(label);
}

void Federate::waitForSynchronization(const std::wstring& label)
{
    std::wcout << __func__ << ", label:" << label << std::endl;

    while (hasSynchronizationPending(label)) {
        tick();
    }
}

void Federate::enableTimeRegulation()
{
    std::wcout << __func__ << std::endl;
    ambassador->enableTimeConstrained();

    while (!constrainEnabled) {
        tick();
    }

    ambassador->enableTimeRegulation(lookahead);

    while (!regulationEnabled) {
        tick();
    }
}

void Federate::timeConstrainedEnabled(const LogicalTime& theFederateTime) throw(FederateInternalError)
{
    std::wcout << __func__ << ", time=" << theFederateTime.toString() << std::endl;
    constrainEnabled = true;
}

void Federate::timeRegulationEnabled(const LogicalTime& theFederateTime) throw(FederateInternalError)
{
    std::wcout << __func__ << ", time=" << theFederateTime.toString() << std::endl;
    regulationEnabled = true;
}

void Federate::federationSynchronized(const std::wstring& label,
                                     const FederateHandleSet& /*failedToSyncSet*/) throw(FederateInternalError)
{
    std::wcout << __func__ << ", label:" << label << std::endl;
    std::wcout << L"Federation synchronized on label " << label << std::endl;
    auto find_it = std::find(std::begin(synchronization_points), std::end(synchronization_points), label);
    if (find_it == end(synchronization_points)) {
        throw FederateInternalError(L"Synchronization point <" + label + L"> achieved but never announced");
    }
    synchronization_points.erase(find_it);
}

void Federate::timeAdvanceGrant(const LogicalTime& theTime) throw(FederateInternalError)
{
    currentTime = theTime;
    timeGranted = true;
}

void Federate::waitForTimeAdvanceGrant()
{
    while (!timeGranted) {
        tick();
    }
}