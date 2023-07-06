#pragma once
#ifndef MULTIMEDIASENDER_H
#define MULTIMEDIASENDER_H

#include <mutex>
#include <gst/gst.h>
#include "MultimediaInterface.h"

class MultimediaSender : public MultimediaInterface
{
public:
    static MultimediaSender& GetInstance() {
        // 멀티스레드 환경에서 안전한 인스턴스 생성을 보장하기 위해 동기화
        std::lock_guard<std::mutex> lock(instanceMutex);

        static MultimediaSender instance;
        return instance;
    }

	bool initMultimediaSender = false;
	std::string receiverIp = "127.0.0.1"; 


    ~MultimediaSender();

    bool initialize();
    void cleanup();
    void start();
    void stop();

    void setVideoResolution();
	std::string getReceiverIP();
    void setReceiverIP();
    void setPort(int videoPort, int audioPort);
    void setCameraIndex(int index);
    void setVideoFlipMethod(int method);
	void setVideoEncBitRate();
    void setVideoEncTune();
    void setAudioOpusencAudioType(int audioType);
    void setWindow(void* hVideo);
private:
    MultimediaSender();
    MultimediaSender(const MultimediaSender&) = delete;
    MultimediaSender& operator=(const MultimediaSender&) = delete;

    static std::mutex instanceMutex;  // 인스턴스 생성 동기화에 사용할 뮤텍스

	unsigned int sendVideoWidth		= 320;
	unsigned int sendVideoHeight	= 240;

	unsigned int sendVideoBitRate	= 512;	//(x264 codec)default : 2048
	unsigned int sendVideoTune		= 0x4;	//(x264 codec)(bitwise) 0x1: still image, 0x2: Fast Decode, 0x4: zero latency
	
    GstElement* senderVideoPipeline;
    GstElement* senderAudioPipeline;

    GstElement* videoSrc;
    GstElement* videoFlip;
    GstElement* videoCapsfilter;
    GstElement* videoEnc;
    GstElement* videoPay;
    GstElement* videoSink;
    GstElement* tee;
    GstElement* queueDisplay;
    GstElement* queueNetwork;
    GstElement* videoDisplaySink;
    GstElement* videoSinkWindow;

    GstElement* audioSrc;
    GstElement* audioConv;
    GstElement* audioResample;
    GstElement* audioOpusenc;
    GstElement* audioPay;
    GstElement* audioSink;

    GstBus* senderVideoBus;
    GstBus* senderAudioBus;

    GMainLoop* mainLoop;
};

#endif  // MULTIMEDIA_SENDER_H
