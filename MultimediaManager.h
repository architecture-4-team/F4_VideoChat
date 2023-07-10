#pragma once
#ifndef MULTIMEDIAMANAGER_H
#define MULTIMEDIAMANAGER_H

#include <mutex>
#include <vector>
#include <unordered_map>

#include "MultimediaInterface.h"

class MultimediaManager {
public:
    static MultimediaManager& GetInstance();
    ~MultimediaManager();

    // bool initialize(); // not used for now
    bool setupSender(void* hVideo, std::string ip, int videoPort, int audioPort);
    bool setupReceiver(void* hVideo, int videoPort, int audioPort, int id);
    void makeCall();
    void pauseCall();
    bool acceptCall(/* Callee 객체 받기 */void* hVideo); // not used for now
    void endCall();

    void pauseReceiver(int video); //debug
    void playReceiver(int video); // debug

    void makeReceiverStateChange(int video, int state); // debug
private:
    MultimediaManager();
    MultimediaManager(const MultimediaManager&) = delete;

    MultimediaManager& operator=(const MultimediaManager&) = delete;

    static std::mutex instanceMutex;  // 인스턴스 생성 동기화에 사용할 뮤텍스

    MultimediaInterface& mSender;
    //std::vector<MultimediaInterface*> receiverList;
    std::unordered_map<int, MultimediaInterface*> receiverMap;

    
};
#endif // MULTIMEDIAMANAGER_H
