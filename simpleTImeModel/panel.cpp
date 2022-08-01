#include "panel.h"

#include <cstring>
#include <iomanip>
#include <iostream>

#include "MessageBuffer.hh"

AppCore::AppCore(QObject* parent) : QObject(parent)
{
    startTime = std::chrono::system_clock::now();
    panel = new Panel(L"timeFederation", L"Panel");
    timer = new QTimer();
    std::cout << "Timer connection" << std::endl;
    connect(timer, SIGNAL(timeout()), this, SLOT(changeAtrInQml()));
    timer->start();
    std::cout << "Timer started" << std::endl;
}

AppCore::~AppCore()
{
    delete timer;
    delete panel;
}

void AppCore::changeAtrInQml()
{
    NameVal* ys = panel->getY();
    emit y1Changed(ys[0].value, QString::fromStdWString(ys[0].name));
    emit y2Changed(ys[1].value, QString::fromStdWString(ys[1].name));

    panel->run();

    std::chrono::duration<double, std::milli> ms = (time - startTime);
    emit timeChanged(ms.count() / 1e+3);

    ms = std::chrono::system_clock::now() - time;
    double delta = ms.count() / 1e+3;
    emit errorChanged(panel->getK() * delta - ys[0].value);
    emit deltaChanged(delta);
    emit kDeltaChanged(panel->getK() * delta);
}

Panel::Panel(const std::wstring& federation_name, const std::wstring& federate_name)
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

void Panel::createOrJoin()
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

    handle = ambassador->joinFederationExecution(federateName, L"SimpleTimePanel", federationName, modules);
}

void Panel::resignAndDelete()
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

void Panel::publishAndSubscribe()
{
    std::wcout << __func__ << std::endl;

    auto model_handle = ambassador->getObjectClassHandle(L"Model");
    std::wcout << "Model handle: " << model_handle << std::endl;

    AttributeHandleSet attributes;
    attributes.insert(ambassador->getAttributeHandle(model_handle, L"Y"));

    ambassador->subscribeObjectClassAttributes(model_handle, attributes);

    auto modelParameters_handle = ambassador->getInteractionClassHandle(L"ModelParameters");
    ambassador->publishInteractionClass(modelParameters_handle);
    std::wcout << "modelParameters handle: " << modelParameters_handle << std::endl;
}

void Panel::sendKAndReset(const LogicalTime& time)
{
    static const auto modelParam_handle = ambassador->getInteractionClassHandle(L"ModelParameters");
    static const auto param_k_handle = ambassador->getParameterHandle(modelParam_handle, L"k");
    static const auto param_reset_handle = ambassador->getParameterHandle(modelParam_handle, L"CMD_RESET");

    libhla::MessageBuffer buffer;

    ParameterHandleValueMap parameters;

    buffer.reset();
    buffer.write_double(k);
    buffer.updateReservedBytes();
    parameters[param_k_handle] = VariableLengthData(static_cast<char*>(buffer(0)), buffer.size());

    buffer.reset();
    std::cout << CMD_RESET << std::endl;
    buffer.write_bool(CMD_RESET);
    buffer.updateReservedBytes();
    parameters[param_reset_handle] = VariableLengthData(static_cast<char*>(buffer(0)), buffer.size());

    std::wstring tag {L""};
    ambassador->sendInteraction(modelParam_handle, parameters, {tag.c_str(), tag.size()}, time);
}

void Panel::run()
{
    timeGranted = false;

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

    std::wcout << currentTime.toString() << std::endl;

    sendKAndReset(next_step);
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
    std::wcout << __func__ << ", theObject=" << theObject << std::endl;

    static const auto model_handle = ambassador->getObjectClassHandle(L"Model");
    static const auto attribute_y_handle = ambassador->getAttributeHandle(model_handle, L"Y");

    for (const auto& kv : theAttributeValues) {
        libhla::MessageBuffer mb;
        mb.resize(kv.second.size());
        mb.reset();
        std::memcpy(
            static_cast<char*>(mb(0)), kv.second.data(), kv.second.size());
        mb.assumeSizeFromReservedBytes();

        if (ys[0].name == L"" || ys[0].name == theObject.toString()) {
            ys[0].name = theObject.toString();
            ys[0].value = mb.read_double();
        } else if (ys[1].name == L"" || ys[1].name == theObject.toString()) {
            ys[1].name = theObject.toString();
            ys[1].value = mb.read_double();
        }

    }

    std::wcout << "Object handle" << theObject << std::endl;
    std::wcout << "Model handle" << model_handle << std::endl;
}