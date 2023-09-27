#pragma once

#include <string>
#include <iostream>
#include "json/json.h"

enum Status
{
    pending=0,
    running=1,
    finished=2
};

class Simulator
{
public:
    Simulator();
    virtual ~Simulator();
    virtual bool reset()=0;
    virtual void start()=0;
    virtual void getResult(std::string& result)=0;
    virtual Status getStatus();

protected:
    Status m_status;
    std::string m_results;
};