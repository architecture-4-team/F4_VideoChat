#pragma once
#ifndef MULTIMEDIASENDER_H
#define MULTIMEDIASENDER_H

#include <gst/gst.h>
#include <iostream>
#include <string>
#include "MultimediaInterface.h"

class MultimediaSender : public MultimediaInterface
{
public:
    MultimediaSender();
    ~MultimediaSender();

    bool initialize();
    void cleanup();
    void start();
    void stop();

    void setVideoResolution(int width, int height);
    void setReceiverIP(const std::string& ip);
    void setPort(int videoPort, int audioPort);
    void setCameraIndex(int index);
    void setVideoFlipMethod(int method);
    void setVideoEncTune(int tune);
    void setAudioOpusencAudioType(int audioType);
    void setWindow(void* hVideo);
private:
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
