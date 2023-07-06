#pragma once
#include <string>
#include <iostream>

class MultimediaInterface
{
public:
    virtual ~MultimediaInterface() {}

    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setPort(int videoPort, int audioPort) = 0;
    virtual void setWindow(void* hVideo) = 0;
};
