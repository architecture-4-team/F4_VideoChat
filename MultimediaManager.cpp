#include "multimediaManager.h"
#include "MultimediaSender.h"
#include "MultimediaReceiver.h"


MultimediaManager::MultimediaManager() 
    : mSender(MultimediaSender::GetInstance()), receiverList(0, nullptr)
{

};

MultimediaManager::~MultimediaManager() {

}

bool MultimediaManager::initialize() {

    mSender = MultimediaSender::GetInstance();

    return true;
}

