#include "MultiMediaSender.h"
#include <iostream>
#include <gst/video/videooverlay.h>

static gboolean handle_sender_video_bus_message(GstBus* bus, GstMessage* msg, gpointer data);
static gboolean handle_sender_audio_bus_message(GstBus* bus, GstMessage* msg, gpointer data);

std::mutex MultimediaSender::instanceMutex;

MultimediaSender::MultimediaSender()
    : senderVideoPipeline(nullptr), senderAudioPipeline(nullptr),
    videoSrc(nullptr), videoFlip(nullptr), videoCapsfilter(nullptr),
    videoEnc(nullptr), videoPay(nullptr), videoSink(nullptr), tee(nullptr),
    queueDisplay(nullptr), queueNetwork(nullptr), videoDisplaySink(nullptr),
    audioSrc(nullptr), audioConv(nullptr), audioResample(nullptr),
    audioOpusenc(nullptr), audioPay(nullptr), audioSink(nullptr),
    senderVideoBus(nullptr), senderAudioBus(nullptr), 
    mainLoop(nullptr), receiverIp("127.0.0.1"), initialized(false)
{
    // Initialize GStreamer it is done at main
    // gst_init(nullptr, nullptr);
}

MultimediaSender::~MultimediaSender()
{
    cleanup();
    //gst_deinit();
}

bool MultimediaSender::initialize()
{
	if(initialized) return true;

    // Create sender video pipeline
    senderVideoPipeline = gst_pipeline_new("senderVideoPipeline");

    // Create video elements
    videoSrc = gst_element_factory_make("mfvideosrc", "videoSrc");
    videoFlip = gst_element_factory_make("videoflip", "videoFlip");
    videoCapsfilter = gst_element_factory_make("capsfilter", "videoCapsfilter");
    videoEnc = gst_element_factory_make("x264enc", "videoEnc");
    videoPay = gst_element_factory_make("rtph264pay", "videoPay");
    videoSink = gst_element_factory_make("udpsink", "videoSink");
    tee = gst_element_factory_make("tee", "tee");
    queueDisplay = gst_element_factory_make("queue", "queueDisplay");
    queueNetwork = gst_element_factory_make("queue", "queueNetwork");
    
    //videoDisplaySink = gst_element_factory_make("autovideosink", "videoSinkDisplay");
    videoDisplaySink = gst_element_factory_make("d3dvideosink", "videoSinkDisplay");

    // Create sender audio pipeline
    senderAudioPipeline = gst_pipeline_new("senderAudioPipeline");

    // Create audio elements
    audioSrc = gst_element_factory_make("autoaudiosrc", "audioSrc");
    audioConv = gst_element_factory_make("audioconvert", "audioConv");
    audioResample = gst_element_factory_make("audioresample", "audioResample");
    audioOpusenc = gst_element_factory_make("opusenc", "audioOpusenc");
    audioPay = gst_element_factory_make("rtpopuspay", "audioPay");
    audioSink = gst_element_factory_make("udpsink", "audioSink");

    // Check if all elements are created successfully
    if (!senderVideoPipeline || !senderAudioPipeline || !videoSrc || !videoFlip ||
        !videoCapsfilter || !videoEnc || !videoPay || !videoSink || !tee ||
        !queueDisplay || !queueNetwork || !videoDisplaySink || !audioSrc ||
        !audioConv || !audioResample || !audioOpusenc || !audioPay || !audioSink)
    {
        std::cerr << "Failed to create GStreamer elements." << std::endl;
        cleanup();
        return false;
    }

    // Add elements to pipelines
    gst_bin_add_many(GST_BIN(senderVideoPipeline), videoSrc, videoFlip, videoCapsfilter,
        videoEnc, videoPay, videoSink, tee, queueDisplay, queueNetwork,
        videoDisplaySink, nullptr);
    gst_bin_add_many(GST_BIN(senderAudioPipeline), audioSrc, audioConv, audioResample,
        audioOpusenc, audioPay, audioSink, nullptr);

    // Link video elements
    gst_element_link_many(videoSrc, videoFlip, videoCapsfilter, tee, nullptr);
    GstPad* displayPad = gst_element_request_pad_simple(tee, "src_%u");
    GstPad* displaySinkPad = gst_element_get_static_pad(queueDisplay, "sink");
    gst_pad_link(displayPad, displaySinkPad);
    gst_element_link(queueDisplay, videoDisplaySink);
    GstPad* networkPad = gst_element_request_pad_simple(tee, "src_%u");
    GstPad* networkSinkPad = gst_element_get_static_pad(queueNetwork, "sink");
    gst_pad_link(networkPad, networkSinkPad);
    gst_element_link_many(queueNetwork, videoEnc, videoPay, videoSink, nullptr);

    // Link audio elements
    gst_element_link_many(audioSrc, audioConv, audioResample, audioOpusenc, audioPay, audioSink, nullptr);

    // Get the bus for the pipelines
    senderVideoBus = gst_element_get_bus(senderVideoPipeline);
    senderAudioBus = gst_element_get_bus(senderAudioPipeline);

    // Add watch to the bus to handle messages
    gst_bus_add_watch(senderVideoBus, (GstBusFunc)handle_sender_video_bus_message, this);
    gst_bus_add_watch(senderAudioBus, (GstBusFunc)handle_sender_audio_bus_message, this);

    initialized = true;

    return true;
}

void MultimediaSender::cleanup()
{
    // Cleanup the main loop
    g_main_loop_unref(mainLoop);
    mainLoop = nullptr;

    if (senderVideoPipeline)
    {
        gst_element_set_state(senderVideoPipeline, GST_STATE_NULL);
        gst_object_unref(senderVideoPipeline);
        senderVideoPipeline = nullptr;
    }

    if (senderAudioPipeline)
    {
        gst_element_set_state(senderAudioPipeline, GST_STATE_NULL);
        gst_object_unref(senderAudioPipeline);
        senderAudioPipeline = nullptr;
    }

    // Release video elements
    gst_object_unref(videoSrc);
    gst_object_unref(videoFlip);
    gst_object_unref(videoCapsfilter);
    gst_object_unref(videoEnc);
    gst_object_unref(videoPay);
    gst_object_unref(videoSink);
    gst_object_unref(tee);
    gst_object_unref(queueDisplay);
    gst_object_unref(queueNetwork);
    gst_object_unref(videoDisplaySink);

    // Release audio elements
    gst_object_unref(audioSrc);
    gst_object_unref(audioConv);
    gst_object_unref(audioResample);
    gst_object_unref(audioOpusenc);
    gst_object_unref(audioPay);
    gst_object_unref(audioSink);

    // Release video bus
    if (senderVideoBus)
    {
        gst_object_unref(senderVideoBus);
        senderVideoBus = nullptr;
    }

    // Release audio bus
    if (senderAudioBus)
    {
        gst_object_unref(senderAudioBus);
        senderAudioBus = nullptr;
    }

	initialized = false;
}

void MultimediaSender::start()
{
    if (senderVideoPipeline)
        gst_element_set_state(senderVideoPipeline, GST_STATE_PLAYING);

    if (senderAudioPipeline)
        gst_element_set_state(senderAudioPipeline, GST_STATE_PLAYING);

    if (!mainLoop) {
        // Create a GMainLoop to handle events
        mainLoop = g_main_loop_new(nullptr, FALSE);
    }

    // Run the main loop
    g_main_loop_run(mainLoop);

}

void MultimediaSender::stop()
{
    if (senderVideoPipeline)
        gst_element_set_state(senderVideoPipeline, GST_STATE_NULL);

    if (senderAudioPipeline)
        gst_element_set_state(senderAudioPipeline, GST_STATE_NULL);

    // Quit the main loop
    g_main_loop_quit(mainLoop);
}

// 쓰레드 함수
DWORD WINAPI MultimediaSender::threadCallback(LPVOID lpParam)
{
    // 쓰레드에서 실행할 로직을 작성합니다.
    // 예: 버튼 클릭 이벤트에 대한 처리 등
    MultimediaSender* pSender = static_cast<MultimediaSender*>(lpParam);

    pSender->start();

    WaitForSingleObject(pSender->hThread, INFINITE);
    CloseHandle(pSender->hThread);
    pSender->hThread = INVALID_HANDLE_VALUE;
    return 0;
}

bool MultimediaSender::runThread()
{
    hThread = CreateThread(NULL, 0, MultimediaSender::threadCallback, this, 0, NULL);
    return hThread != NULL;
}

void MultimediaSender::setVideoResolution()
{
    GstCaps* videoCaps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, sendVideoWidth,
        "height", G_TYPE_INT, sendVideoHeight,
        nullptr);
    g_object_set(G_OBJECT(videoCapsfilter), "caps", videoCaps, nullptr);
    gst_caps_unref(videoCaps);
}


std::string MultimediaSender::getReceiverIP()
{
	printf("receiveIp:%s\n", receiverIp.c_str());
	return receiverIp;
}

void MultimediaSender::setReceiverIP(std::string ip)
{
    receiverIp = ip;
	printf("receiveIp:%s\n", receiverIp.c_str());
    g_object_set(G_OBJECT(videoSink), "host", receiverIp.c_str(), nullptr);
    g_object_set(G_OBJECT(audioSink), "host", receiverIp.c_str(), nullptr);
}

void MultimediaSender::setPort(int videoPort, int audioPort)
{
    g_object_set(G_OBJECT(videoSink), "port", videoPort, nullptr);
    g_object_set(G_OBJECT(audioSink), "port", audioPort, nullptr);
}

void MultimediaSender::setCameraIndex(int index)
{
    g_object_set(G_OBJECT(videoSrc), "device-index", index, nullptr);
}

void MultimediaSender::setVideoFlipMethod(int method)
{
/*
	Video-flip-method 
	(0) Identity (no rotation)
	(1) Rotate clockwise 90 degrees
	(2) Rotate 180 degrees
	(3) Rotate counter-clockwise 90 degrees
	(4) Flip horizontally
	(5) Flip vertically
	(6) Flip across upper left/lower right diagonal
	(7) Flip across upper right/lower left diagonal
	(8) Select flip method based on image-orientation tag
*/	
    g_object_set(G_OBJECT(videoFlip), "method", method, nullptr);
}

void MultimediaSender::setVideoEncBitRate()
{
	GstElement *encoder = videoEnc;
	gint bitrate;
	g_object_get(G_OBJECT(encoder), "bitrate", &bitrate, NULL);
	printf("EncBitRate:%u->", bitrate);
	g_object_set(G_OBJECT(videoEnc), "bitrate", sendVideoBitRate, nullptr);
	g_object_get(G_OBJECT(encoder), "bitrate", &bitrate, NULL);
	printf("%u[KBps]\n", bitrate);
}

void MultimediaSender::setVideoEncTune()
{
    g_object_set(G_OBJECT(videoEnc), "tune", sendVideoTune, nullptr);
}

void MultimediaSender::setAudioOpusencAudioType(int audioType, int bandwidth, int bitrate, int bitrateType, int frameSize, bool inbandFEC)
{
    g_object_set(G_OBJECT(audioOpusenc), "audio-type", audioType, "bandwidth", 
					bandwidth, "bitrate", bitrate, "bitrate-type", bitrateType, 
					"frame-size", frameSize, "inband-fec", inbandFEC, nullptr);}

void MultimediaSender::setWindow(void* hVideo)
{
    // VideoDisplaySink를 윈도우와 연결
     // 비디오 출력 설정
    g_object_set(G_OBJECT(videoDisplaySink), "force-aspect-ratio", TRUE, NULL);

    // 비디오 오버레이 설정
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(videoDisplaySink), (guintptr)hVideo);

    //g_object_set(G_OBJECT(videoDisplaySink), "window-handle", reinterpret_cast<guintptr>(hVideo), nullptr);
}

static gboolean handle_sender_video_bus_message(GstBus* bus, GstMessage* msg, gpointer data)
{
    MultimediaSender* sender = static_cast<MultimediaSender*>(data);

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        GError* err;
        gchar* debug_info;
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_error_free(err);
        g_free(debug_info);
        sender->stop(); // Quit the main loop in case of an error
        break;
    }
    case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        sender->stop(); // Quit the main loop when the end of the stream is reached
        break;
    case GST_MESSAGE_STATE_CHANGED:
    {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
        g_print("Sender Video state changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
        break;
    }
    case GST_MESSAGE_BUFFERING:
    {
        gint percent = 0;
        gst_message_parse_buffering(msg, &percent);
        g_print("Buffering %d%%\n", percent);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

static gboolean handle_sender_audio_bus_message(GstBus* bus, GstMessage* msg, gpointer data)
{
    MultimediaSender* sender = static_cast<MultimediaSender*>(data);

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        GError* err;
        gchar* debug_info;
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_error_free(err);
        g_free(debug_info);
        sender->stop(); // Quit the main loop in case of an error
        break;
    }
    case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        sender->stop(); // Quit the main loop when the end of the stream is reached
        break;
    case GST_MESSAGE_STATE_CHANGED:
    {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
        g_print("Sender Audio state changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
        break;
    }
    case GST_MESSAGE_BUFFERING:
    {
        gint percent = 0;
        gst_message_parse_buffering(msg, &percent);
        g_print("Buffering %d%%\n", percent);
        break;
    }
    default:
        break;
    }

    return TRUE;
}