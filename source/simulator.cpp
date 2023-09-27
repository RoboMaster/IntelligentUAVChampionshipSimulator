#include "simulator.h"

Simulator::Simulator()
{
    m_status = Status::pending;
    std::cout<<"Construct simulator."<<std::endl;
}

Simulator::~Simulator(){std::cout<<"Destruct simulator."<<std::endl;};

Status Simulator::getStatus()
{
    return m_status;
}
