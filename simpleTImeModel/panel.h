#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "defaultFederate.h"

#include <chrono>
#include <iostream>
#include <string>

struct NameVal {
    std::wstring name = L"";
    double value = 0.0;
};

class Panel : public Federate {
private:

    NameVal ys[2];

    double k = 1;
    bool CMD_RESET = false;

    void resignAndDelete() override;
    void createOrJoin() override;

    void publishAndSubscribe() override;

    void sendKAndReset(const LogicalTime& time);

    void reflectAttributeValues(rti1516e::ObjectInstanceHandle theObject,
                                const AttributeHandleValueMap& theAttributeValues,
                                const rti1516e::VariableLengthData& theUserSuppliedTag,
                                rti1516e::OrderType sentOrder,
                                rti1516e::TransportationType theType,
                                const rti1516e::LogicalTime& theTime,
                                rti1516e::OrderType receivedOrder,
                                rti1516e::MessageRetractionHandle theHandle,
                                rti1516e::SupplementalReflectInfo theReflectInfo) throw(FederateInternalError) override;


public:
    Panel(const std::wstring& federation_name, const std::wstring& federate_name);
    virtual ~Panel();

    NameVal* getY()
    {
        return ys;
    }

    double getK()
    {
        return k;
    }

    void setK(double k)
    {
        this->k = k;
    }

    void setReset(bool reset)
    {
        this->CMD_RESET = reset;
    }
    void run();
};

class AppCore : public QObject {
    Q_OBJECT
public:
    explicit AppCore(QObject* parent = nullptr);
    virtual ~AppCore();

    Q_INVOKABLE void setK(double value)
    {
        panel->setK(value);
    }

    Q_INVOKABLE void setResetOnPressed()
    {
        panel->setReset(true);
    }

    Q_INVOKABLE void setResetOnReleased()
    {
        panel->setReset(false);
        panel->run();
        time = std::chrono::system_clock::now();
    }

signals:
    void y1Changed(double value, QString objectName);
    void y2Changed(double value, QString objectName);
    void timeChanged(double value);
    void deltaChanged(double value);
    void kDeltaChanged(double value);
    void errorChanged(double value);

public slots:
    void changeAtrInQml();

private:
    Panel* panel;
    QTimer* timer;
    std::chrono::system_clock::time_point time;
    std::chrono::system_clock::time_point startTime;
};
