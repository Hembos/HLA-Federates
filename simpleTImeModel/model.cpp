#include "model.h"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <chrono>

#include "MessageBuffer.hh"

Model::Model(const std::wstring& federation_name, const std::wstring& federate_name)
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

    modelOblect_handle
        = ambassador->registerObjectInstance(ambassador->getObjectClassHandle(L"Model"), federateName + L"_Model");

    if (has_created) {
        register_sync_point(L"Start");
    }

    synchronize(L"Start");
    waitForSynchronization(L"Start");
}

Model::~Model()
{
    resignAndDelete();
}

void Model::modeling(double k)
{
    y += dt * k;
}

void Model::resetY()
{
    y = 0;
}

void Model::createOrJoin()
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

    handle = ambassador->joinFederationExecution(federateName, L"SimpleTimeModel", federationName, modules);
}

void Model::resignAndDelete()
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

void Model::publishAndSubscribe()
{
    std::wcout << __func__ << std::endl;

    auto model_handle = ambassador->getObjectClassHandle(L"Model");
    std::wcout << "Model handle: " << model_handle << std::endl;

    AttributeHandleSet attributes;
    attributes.insert(ambassador->getAttributeHandle(model_handle, L"Y"));

    ambassador->publishObjectClassAttributes(model_handle, attributes);

    auto time_handle = ambassador->getObjectClassHandle(L"Time");
    std::wcout << "Time handle: " << time_handle << std::endl;

    AttributeHandleSet attributesTime;
    attributesTime.insert(ambassador->getAttributeHandle(time_handle, L"DT"));

    ambassador->subscribeObjectClassAttributes(time_handle, attributesTime);

    auto modelParameters_handle = ambassador->getInteractionClassHandle(L"ModelParameters");
    ambassador->subscribeInteractionClass(modelParameters_handle);
    std::wcout << "modelParameters handle: " << modelParameters_handle << std::endl;
}

void Model::receiveInteraction(rti1516e::InteractionClassHandle theInteraction,
                               const ParameterHandleValueMap& theParameterValues,
                               const rti1516e::VariableLengthData& /*theUserSuppliedTag*/,
                               rti1516e::OrderType /*sentOrder*/,
                               rti1516e::TransportationType /*theType*/,
                               const rti1516e::LogicalTime& /*theTime*/,
                               rti1516e::OrderType /*receivedOrder*/,
                               rti1516e::MessageRetractionHandle /*theHandle*/,
                               rti1516e::SupplementalReceiveInfo /*theReceiveInfo*/) throw(FederateInternalError)
{
    std::wcout << __func__ << ", theInteraction=" << theInteraction << std::endl;

    static const auto modelParameters_handle = ambassador->getInteractionClassHandle(L"ModelParameters");
    static const auto param_k_handle = ambassador->getParameterHandle(modelParameters_handle, L"k");
    static const auto param_reset_handle = ambassador->getParameterHandle(modelParameters_handle, L"CMD_RESET");

    if (theInteraction == modelParameters_handle) {
        libhla::MessageBuffer mb;

        auto param_k = theParameterValues.at(param_k_handle);

        mb.resize(param_k.size());
        mb.reset();
        std::memcpy(static_cast<char*>(mb(0)), param_k.data(), param_k.size());
        mb.assumeSizeFromReservedBytes();

        double k = mb.read_double();

        auto param_reset = theParameterValues.at(param_reset_handle);

        mb.resize(param_reset.size());
        mb.reset();
        std::memcpy(static_cast<char*>(mb(0)), param_reset.data(), param_reset.size());
        mb.assumeSizeFromReservedBytes();

        bool reset = mb.read_bool();

        if (reset) {
            resetY();
        }

        modeling(k);

        std::cout << y << std::endl;
    }
}

void Model::run()
{
    // using std::chrono::high_resolution_clock;
    // using std::chrono::duration_cast;
    // using std::chrono::duration;
    // using std::chrono::milliseconds;

    //auto t1 = high_resolution_clock::now();

    while (true) {
        timeGranted = false;

        std::wcout << "time at start is " << currentTime.toString() << std::endl;

        ambassador->queryLogicalTime(currentTime);

        std::wcout << "after query, time is " << currentTime.toString() << std::endl;

        auto time_aux = RTI1516fedTime {currentTime};
        time_aux += lookahead;

        std::wcout << "request advance to " << time_aux.toString() << std::endl;

        timeGranted = false;
        ambassador->timeAdvanceRequest(time_aux);

        // duration<double, std::milli> ms = high_resolution_clock::now() - t1;
        // std::cout << ms.count() << " ms" << std::endl;
        // t1 = high_resolution_clock::now();
        // dt = ms.count() / 1000;

        waitForTimeAdvanceGrant();

        std::wcout << "after wait for grant, time is " << currentTime.toString() << std::endl;

        auto next_step = RTI1516fedTime {currentTime};
        next_step += lookahead;

        sendNewY(next_step);
    }
}

void Model::sendNewY(const LogicalTime& time)
{
    static const auto model_handle = ambassador->getObjectClassHandle(L"Model");
    static const auto attribute_Y_handle = ambassador->getAttributeHandle(model_handle, L"Y");

    libhla::MessageBuffer buffer;

    AttributeHandleValueMap attributes;

    buffer.reset();
    buffer.write_double(y);
    buffer.updateReservedBytes();
    attributes[attribute_Y_handle] = VariableLengthData(static_cast<char*>(buffer(0)), buffer.size());

    std::wstring tag{L""};
    ambassador->updateAttributeValues(modelOblect_handle, attributes, {tag.c_str(), tag.size()}, time);
}

void Model::reflectAttributeValues(rti1516e::ObjectInstanceHandle theObject,
                                   const AttributeHandleValueMap& theAttributeValues,
                                   const rti1516e::VariableLengthData& /*theUserSuppliedTag*/,
                                   rti1516e::OrderType /*sentOrder*/,
                                   rti1516e::TransportationType /*theType*/,
                                   const rti1516e::LogicalTime& /*theTime*/,
                                   rti1516e::OrderType /*receivedOrder*/,
                                   rti1516e::MessageRetractionHandle /*theHandle*/,
                                   rti1516e::SupplementalReflectInfo /*theReflectInfo*/) throw(FederateInternalError)
{
    std::wcout << __func__ << ", theObject=" << theObject << std::endl;

    static const auto time_handle = ambassador->getObjectClassHandle(L"Time");
    static const auto attribute_y_handle = ambassador->getAttributeHandle(time_handle, L"DT");

    for (const auto& kv : theAttributeValues) {
        libhla::MessageBuffer mb;
        mb.resize(kv.second.size());
        mb.reset();
        std::memcpy(
            static_cast<char*>(mb(0)), kv.second.data(), kv.second.size());
        mb.assumeSizeFromReservedBytes();
        dt = mb.read_double();
    }

    std::wcout << "Object handle" << theObject << std::endl;
    std::wcout << "Time handle" << time_handle << std::endl;
}