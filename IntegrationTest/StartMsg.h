#ifndef START_MSG_H
#define START_MSG_H

#include "DelegateMQ.h"

class StartMsg : public serialize::I
{
public:
    int loops;  // Number of times to loop the tests

    virtual std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, loops);
        return os;
    }

    virtual std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, loops);
        return is;
    }
};

#endif