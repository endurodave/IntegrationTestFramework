#ifndef STATUS_MSG_H
#define STATUS_MSG_H

#include "DelegateMQ.h"

class StatusMsg : public serialize::I
{
public:
    enum class Status { STARTED, STOPPED, COMPLETED };

    Status status;

    virtual std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, status);
        return os;
    }

    virtual std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, status);
        return is;
    }
};

#endif