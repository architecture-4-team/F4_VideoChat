#pragma once
#ifndef MULTIMEDIAMANAGER_H
#define MULTIMEDIAMANAGER_H

#include <mutex>
#include <vector>
#include "MultimediaInterface.h"

class MultimediaManager {
public:
    static MultimediaManager& GetInstance() {
        // 멀티스레드 환경에서 안전한 인스턴스 생성을 보장하기 위해 동기화
        std::lock_guard<std::mutex> lock(instanceMutex);
        static MultimediaManager instance;
        return instance;
    }
    ~MultimediaManager();

    bool initialize();

private:
    MultimediaManager();
    MultimediaManager(const MultimediaManager&) = delete;
    MultimediaManager& operator=(const MultimediaManager&) = delete;

    static std::mutex instanceMutex;  // 인스턴스 생성 동기화에 사용할 뮤텍스

    MultimediaInterface& mSender;
    std::vector<MultimediaInterface*> receiverList;
};
#endif // MULTIMEDIAMANAGER_H
