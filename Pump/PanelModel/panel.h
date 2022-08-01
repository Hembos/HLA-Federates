#ifndef APPCORE_H
#define APPCORE_H

#include <QObject>
#include <QTimer>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTI1516fedTime.h>

#include <memory>
#include <string>
#include <vector>

using namespace rti1516e;

class Panel : public NullFederateAmbassador {
public:
    Panel(const std::wstring& federation_name, const std::wstring& federate_name);

    virtual ~Panel();

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

    void announceSynchronizationPoint(
        const std::wstring& label,
        const rti1516e::VariableLengthData& theUserSuppliedTag) throw(FederateInternalError) override;

    virtual void federationSynchronized(const std::wstring& label,
                                        const FederateHandleSet& failedToSyncSet) throw(FederateInternalError) override;

    void timeConstrainedEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError) override;

    void timeRegulationEnabled(const rti1516e::LogicalTime& theFederateTime) throw(FederateInternalError) override;

    virtual void timeAdvanceGrant(const rti1516e::LogicalTime& theTime) throw(FederateInternalError) override;

    void reflectAttributeValues(rti1516e::ObjectInstanceHandle theObject,
                                const AttributeHandleValueMap& theAttributeValues,
                                const rti1516e::VariableLengthData& theUserSuppliedTag,
                                rti1516e::OrderType sentOrder,
                                rti1516e::TransportationType theType,
                                const rti1516e::LogicalTime& theTime,
                                rti1516e::OrderType receivedOrder,
                                rti1516e::MessageRetractionHandle theHandle,
                                rti1516e::SupplementalReflectInfo theReflectInfo) throw(FederateInternalError) override;

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

    RTI1516fedTimeInterval my_time_interval {1.0};

    RTI1516fedTime my_local_time {0.0};

public:
    double atr1;
};

class AppCore : public QObject
{
    Q_OBJECT
public:
    explicit AppCore(QObject *parent = nullptr);

signals:
    void sendToQml(double atr1);

public slots:
    void changeAtrInQml();

private:
    int m_counter {0};
    QTimer *timer;
    Panel* panel;
};

#endif // APPCORE_H
