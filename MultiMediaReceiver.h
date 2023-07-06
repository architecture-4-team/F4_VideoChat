#pragma once
#ifndef MULTIMEDIARECEIVER_H
#define MULTIMEDIARECEIVER_H

#include <gst/gst.h>
#include <iostream>
#include "MultimediaInterface.h"

class MultimediaReceiver : public MultimediaInterface {
public:
    MultimediaReceiver();
    ~MultimediaReceiver();
    bool initialize();
    void cleanup();
    void start();
    void stop();
    void setPort(int videoPort, int audioPort);
    void setJitterBuffer(int latency);
    void setRTP();
    void setWindow(void* hVideo);
private:
    // Pipeline
    GstElement* receiverVideoPipeline;
    GstElement* receiverAudioPipeline;

    GstElement* videoSrc;
    GstElement* videoCapsfilter;
    GstElement* jitterbufferVideo;
    GstElement* videoDepay;
    GstElement* videoDec;
    GstElement* videoSink;
    
    GstElement* audioSrc;
    GstElement* audioCapsfilter;
    GstElement* jitterbufferAudio;
    GstElement* audioDec;
    GstElement* audioDepay;
    GstElement* audioConv;
    GstElement* audioSink;

    GstBus* receiverVideoBus;
    GstBus* receiverAudioBus;

    GMainLoop* receiverLoop;
};
static gboolean handle_receiver_video_bus_message(GstBus* bus, GstMessage* msg, gpointer data);
static gboolean handle_receiver_audio_bus_message(GstBus* bus, GstMessage* msg, gpointer data);

static GstPadProbeReturn probe_callback(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

#endif  // MULTIMEDIARECEIVER_H