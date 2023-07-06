#ifndef MULTIMEDIA_MANAGER_H
#define MULTIMEDIA_MANAGER_H

#include <gst/gst.h>
#include <string>

class MultimediaManager {
public:
    MultimediaManager();
    ~MultimediaManager();

    bool initialize();
    void cleanup();

    void playAudio(const std::string& audioFilePath);
    void stopAudio();

    void playVideo(const std::string& videoFilePath);
    void stopVideo();

private:
    GstElement* audioPipeline;
    GstElement* audioSource;
    GstElement* audioDecoder;
    GstElement* audioSink;

    GstElement* videoPipeline;
    GstElement* videoSource;
    GstElement* videoDecoder;
    GstElement* videoSink;

    GMainLoop* mainLoop;

    static gboolean handleBusMessage(GstBus* bus, GstMessage* message, gpointer data);
};

#endif // MULTIMEDIA_MANAGER_H
