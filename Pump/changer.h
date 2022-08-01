#pragma once

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTI1516fedTime.h>

#include <memory>
#include <string>
#include <vector>

using namespace rti1516e;

class Changer : public NullFederateAmbassador {
public:
    Changer(const std::wstring& federation_name, const std::wstring& federate_name);

    virtual ~Changer();

    void resignAndDelete();
    void createOrJoin();

    void register_sync_point(const std::wstring& label);

    void run();

    bool hasSynchronizationPending(const std::wstring& label);
    void waitForAnnounce(const std::wstring& label);
    void synchronize(const std::wstring& label);
    void waitForSynchronization(const std::wstring& label);

    void publishAndSubscribe();

    void enableTimeRegulation();

    void waitForTimeAdvanceGrant();

    void tick();

    void sendDx(const LogicalTime& time);

    void announceSynchronizationPoint(
        const std::wstring& label,
        const rti1516e::VariableLengthData& theUserSuppliedTag) throw(FederateInternalError) override;

    virtual void federationSynchronized(const std::wstring& label,
                                        const FederateHandleSet& failedToSyncSet) throw(FederateInternalError) override;
    
    void timeConstrainedEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError) override;

    void timeRegulationEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError) override;

    virtual void timeAdvanceGrant(const rti1516e::LogicalTime& theTime) throw(FederateInternalError) override;

private:
    const std::wstring federation_name;
    const std::wstring federate_name;

    rti1516e::FederateHandle my_handle;

    ObjectInstanceHandle pump_object_handle;

    std::unique_ptr<rti1516e::RTIambassador> my_ambassador {rti1516e::RTIambassadorFactory().createRTIambassador()};

    bool has_created {false};
    bool has_collision_enabled {false};

    std::vector<std::wstring> my_synchronization_points;

    bool my_is_time_constrained {false};
    bool my_is_time_regulated {false};

    bool my_time_granted {false};

    RTI1516fedTimeInterval my_time_interval {100.0};

    RTI1516fedTime my_local_time {0.0};

    double dx1 {1.};
    double dx2 {10.};
};