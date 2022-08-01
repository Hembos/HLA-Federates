#pragma once

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTI1516fedTime.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace rti1516e;

class Federate : NullFederateAmbassador {
protected:
    std::wstring federationName;
    std::wstring federateName;

    bool regulationEnabled = false;
    bool constrainEnabled = false;

    bool timeGranted = false;

    bool has_created = false;

    RTI1516fedTime currentTime = 0.;
    RTI1516fedTimeInterval lookahead = 1.0;

    rti1516e::FederateHandle handle;

    std::unique_ptr<rti1516e::RTIambassador> ambassador{rti1516e::RTIambassadorFactory().createRTIambassador()};

    void register_sync_point(const std::wstring& label);

    void synchronize(const std::wstring& label);

    void waitForAnnounce(const std::wstring& label);

    void tick();

    bool hasSynchronizationPending(const std::wstring& label);

    void waitForSynchronization(const std::wstring& label);

    virtual void publishAndSubscribe() = 0;
    virtual void resignAndDelete() = 0;
    virtual void createOrJoin() = 0;

    void enableTimeRegulation();

    void waitForTimeAdvanceGrant();

private:
    std::vector<std::wstring> synchronization_points;

    void announceSynchronizationPoint(
        const std::wstring& label,
        const VariableLengthData& theUserSuppliedTag) throw(FederateInternalError) override;


    void timeConstrainedEnabled(const LogicalTime& theFederateTime) throw(FederateInternalError) override;

    void timeRegulationEnabled(const LogicalTime& theFederateTime) throw(FederateInternalError) override;

    virtual void federationSynchronized(const std::wstring& label,
                                        const FederateHandleSet& failedToSyncSet) throw(FederateInternalError) override;

    virtual void timeAdvanceGrant(const rti1516e::LogicalTime& theTime) throw(FederateInternalError) override;


public:
    Federate(const std::wstring& federation_name, const std::wstring& federate_name);
    virtual ~Federate();
};