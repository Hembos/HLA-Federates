#pragma once

#include <iostream>
#include <string>

#include "defaultFederate.h"

class Time : public Federate
{
private:
    double acceleration = 1.0;

    double dt = 0.04;

    ObjectInstanceHandle timeOblect_handle;

    void resignAndDelete() override;
    void createOrJoin() override;

    void publishAndSubscribe() override;

    void sendNewDT(const rti1516e::LogicalTime& time);
public:
    Time(const std::wstring& federation_name, const std::wstring& federate_name);
    virtual ~Time();

    void run();
};