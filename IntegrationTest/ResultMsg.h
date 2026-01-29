#ifndef RESULT_MSG_H
#define RESULT_MSG_H

#include "DelegateMQ.h"

class ResultMsg : public serialize::I
{
public:
    std::string results;

    virtual std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, results);
        return os;
    }

    virtual std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, results);
        return is;
    }
};

#endif