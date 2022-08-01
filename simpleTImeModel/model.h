#pragma once

#include <iostream>
#include <string>

#include "defaultFederate.h"

class Model : public Federate
{
private:
    double y = 0;

    double dt = 0.04;

    ObjectInstanceHandle modelOblect_handle;

    void modeling(double k);
    void resetY();  

    void resignAndDelete() override;
    void createOrJoin() override;

    void publishAndSubscribe() override;

    void receiveInteraction(rti1516e::InteractionClassHandle theInteraction,
                            const ParameterHandleValueMap& theParameterValues,
                            const rti1516e::VariableLengthData& theUserSuppliedTag,
                            rti1516e::OrderType sentOrder,
                            rti1516e::TransportationType theType,
                            const rti1516e::LogicalTime& theTime,
                            rti1516e::OrderType receivedOrder,
                            rti1516e::MessageRetractionHandle theHandle,
                            rti1516e::SupplementalReceiveInfo theReceiveInfo) throw(FederateInternalError) override;

    void reflectAttributeValues(rti1516e::ObjectInstanceHandle theObject,
                                const AttributeHandleValueMap& theAttributeValues,
                                const rti1516e::VariableLengthData& theUserSuppliedTag,
                                rti1516e::OrderType sentOrder,
                                rti1516e::TransportationType theType,
                                const rti1516e::LogicalTime& theTime,
                                rti1516e::OrderType receivedOrder,
                                rti1516e::MessageRetractionHandle theHandle,
                                rti1516e::SupplementalReflectInfo theReflectInfo) throw(FederateInternalError) override;

    void sendNewY(const rti1516e::LogicalTime& time);
public:
    Model(const std::wstring& federation_name, const std::wstring& federate_name);
    virtual ~Model();

    void run();
};