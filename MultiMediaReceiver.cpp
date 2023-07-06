#include "MultiMediaReceiver.h"
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

MultimediaReceiver::MultimediaReceiver()
    : receiverVideoPipeline(nullptr), receiverAudioPipeline(nullptr),
    videoSrc(nullptr), videoCapsfilter(nullptr),
    jitterbufferVideo(nullptr), videoDepay(nullptr), videoDec(nullptr),
    videoSink(nullptr), audioSrc(nullptr), audioCapsfilter(nullptr),
    jitterbufferAudio(nullptr), audioDec(nullptr), audioDepay(nullptr),
    audioConv(nullptr), audioSink(nullptr), receiverVideoBus(nullptr),
    receiverLoop(nullptr)
{
    // Initialize GStreamer
    gst_init(nullptr, nullptr);
}

MultimediaReceiver::~MultimediaReceiver()
{
    cleanup();
    gst_deinit();
}

bool MultimediaReceiver::initialize()
{
    // Create receiver video pipeline
    receiverVideoPipeline = gst_pipeline_new("receiverVideoPipeline");

    // Create video elements
    videoSrc = gst_element_factory_make("udpsrc", "videoSrc");
    videoCapsfilter = gst_element_factory_make("capsfilter", "videoCapsfilter");
    jitterbufferVideo = gst_element_factory_make("rtpjitterbuffer", "jitterbufferVideo");
    videoDepay = gst_element_factory_make("rtph264depay", "videoDepay");
    videoDec = gst_element_factory_make("avdec_h264", "videoDec");
    videoSink = gst_element_factory_make("d3dvideosink", "videoSink");

    // Create receiver audio pipeline
    receiverAudioPipeline = gst_pipeline_new("receiverAudioPipeline");

    // Create audio elements
    audioSrc = gst_element_factory_make("udpsrc", "audioSrc");
    audioCapsfilter = gst_element_factory_make("capsfilter", "audioCapsfilter");
    jitterbufferAudio = gst_element_factory_make("rtpjitterbuffer", "jitterbufferAudio");
    audioDec = gst_element_factory_make("opusdec", "audioDec");
    audioDepay = gst_element_factory_make("rtpopusdepay", "audioDepay");
    audioConv = gst_element_factory_make("audioconvert", "audioConv");
    audioSink = gst_element_factory_make("autoaudiosink", "audioSink");

    // Check if all elements are created successfully
    if (!receiverVideoPipeline || !receiverAudioPipeline || !videoSrc || !videoCapsfilter ||
        !jitterbufferVideo || !videoDepay || !videoDec || !videoSink || !audioSrc ||
        !audioCapsfilter || !jitterbufferAudio || !audioDec || !audioDepay || !audioConv || !audioSink)
    {
        std::cerr << "Failed to create GStreamer elements." << std::endl;
        cleanup();
        return false;
    }

    // Add elements to pipelines
    gst_bin_add_many(GST_BIN(receiverVideoPipeline), videoSrc, videoCapsfilter, jitterbufferVideo,
        videoDepay, videoDec, videoSink, nullptr);
    gst_bin_add_many(GST_BIN(receiverAudioPipeline), audioSrc, audioCapsfilter, jitterbufferAudio,
        audioDec, audioDepay, audioConv, audioSink, nullptr);

    // Link video elements
    gst_element_link_many(videoSrc, videoCapsfilter, jitterbufferVideo, videoDepay, videoDec, videoSink, nullptr);

    // Link audio elements
    gst_element_link_many(audioSrc, audioCapsfilter, jitterbufferAudio, audioDepay, audioDec, audioConv, audioSink, nullptr);

    // Get the bus for the pipelines
    receiverVideoBus = gst_element_get_bus(receiverVideoPipeline);
    receiverAudioBus = gst_element_get_bus(receiverAudioPipeline);

    // Add watch to the bus to handle messages
    gst_bus_add_watch(receiverVideoBus, (GstBusFunc)handle_receiver_audio_bus_message, this);
    gst_bus_add_watch(receiverAudioBus, (GstBusFunc)handle_receiver_audio_bus_message, this);

    // Get the sink pad of the sink element
    GstPad* pad = gst_element_get_static_pad(videoCapsfilter, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)probe_callback, NULL, NULL);

    return true;
}

void MultimediaReceiver::cleanup()
{
    if (receiverVideoPipeline)
    {
        gst_element_set_state(receiverVideoPipeline, GST_STATE_NULL);
        gst_object_unref(receiverVideoPipeline);
        receiverVideoPipeline = nullptr;
    }

    if (receiverAudioPipeline)
    {
        gst_element_set_state(receiverAudioPipeline, GST_STATE_NULL);
        gst_object_unref(receiverAudioPipeline);
        receiverAudioPipeline = nullptr;
    }

    // Release video elements
    gst_object_unref(videoSrc);
    gst_object_unref(videoCapsfilter);
    gst_object_unref(jitterbufferVideo);
    gst_object_unref(videoDepay);
    gst_object_unref(videoDec);
    gst_object_unref(videoSink);

    // Release audio elements
    gst_object_unref(audioSrc);
    gst_object_unref(audioCapsfilter);
    gst_object_unref(jitterbufferAudio);
    gst_object_unref(audioDec);
    gst_object_unref(audioDepay);
    gst_object_unref(audioConv);
    gst_object_unref(audioSink);

    // Release video bus
    if (receiverVideoBus)
    {
        gst_object_unref(receiverVideoBus);
        receiverVideoBus = nullptr;
    }

    // Release audio bus
    if (receiverAudioBus)
    {
        gst_object_unref(receiverAudioBus);
        receiverAudioBus = nullptr;
    }
}

void MultimediaReceiver::start()
{
    if (receiverVideoPipeline)
        gst_element_set_state(receiverVideoPipeline, GST_STATE_PLAYING);

    if (receiverAudioPipeline)
        gst_element_set_state(receiverAudioPipeline, GST_STATE_PLAYING);

    // Create a GMainLoop to handle events
    receiverLoop = g_main_loop_new(nullptr, FALSE);

    // Run the main loop
    g_main_loop_run(receiverLoop);

    // Cleanup the main loop
    g_main_loop_unref(receiverLoop);
}

void MultimediaReceiver::stop()
{
    if (receiverVideoPipeline)
        gst_element_set_state(receiverVideoPipeline, GST_STATE_NULL);

    if (receiverAudioPipeline)
        gst_element_set_state(receiverAudioPipeline, GST_STATE_NULL);

    // Quit the main loop
    g_main_loop_quit(receiverLoop);
}

void MultimediaReceiver::setPort(int videoPort, int audioPort)
{
    g_object_set(G_OBJECT(videoSrc), "port", videoPort, nullptr);
    g_object_set(G_OBJECT(audioSrc), "port", audioPort, nullptr);
}

void MultimediaReceiver::setJitterBuffer(int latency)
{
    // latency: 지터 버퍼의 대기 시간(밀리초)을 설정합니다. 이 값은 네트워크 지연을 허용하는 최대 시간을 나타냅니다. 기본값은 200ms입니다.
    // drop - on - latency: 이 속성이 true로 설정되면, 지터 버퍼는 대기 시간 초과 패킷을 버립니다.기본값은 false입니다.
    g_object_set(G_OBJECT(jitterbufferVideo), "latency", latency, "do-lost", TRUE, NULL);
    g_object_set(G_OBJECT(jitterbufferAudio), "latency", latency, "do-lost", TRUE, NULL);
}

void MultimediaReceiver::setRTP()
{
    // RTP video format (caps) settings
    GstCaps* videoCaps = gst_caps_from_string("application/x-rtp, media=(string)video, payload=(int)96");
    g_object_set(G_OBJECT(videoCapsfilter), "caps", videoCaps, NULL);
    gst_caps_unref(videoCaps);

    // RTP audio format (caps) settings
    GstCaps* audioCaps = gst_caps_from_string("application/x-rtp, media=(string)audio, encoding-name=OPUS,  payload=(int)96");
    g_object_set(G_OBJECT(audioCapsfilter), "caps", audioCaps, NULL);
    gst_caps_unref(audioCaps);
}

void MultimediaReceiver::setWindow(void* hVideo)
{
    // VideoDisplaySink를 윈도우와 연결
     // 비디오 출력 설정
    g_object_set(G_OBJECT(videoSink), "force-aspect-ratio", TRUE, NULL);

    // 비디오 오버레이 설정
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(videoSink), (guintptr)hVideo);

    //g_object_set(G_OBJECT(videoDisplaySink), "window-handle", reinterpret_cast<guintptr>(hVideo), nullptr);
}

static gboolean handle_receiver_video_bus_message(GstBus* bus, GstMessage* msg, gpointer data)
{
    MultimediaReceiver* receiver = static_cast<MultimediaReceiver*>(data);

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
        receiver->stop(); // Quit the main loop in case of an error
        break;
    }
    case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        receiver->stop(); // Quit the main loop when the end of the stream is reached
        break;
    case GST_MESSAGE_STATE_CHANGED:
    {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
        g_print("Receiver Video state changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
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

static gboolean handle_receiver_audio_bus_message(GstBus* bus, GstMessage* msg, gpointer data)
{
    MultimediaReceiver* receiver = static_cast<MultimediaReceiver*>(data);

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
        receiver->stop(); // Quit the main loop in case of an error
        break;
    }
    case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        receiver->stop(); // Quit the main loop when the end of the stream is reached
        break;
    case GST_MESSAGE_STATE_CHANGED:
    {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
        g_print("Receiver Audio state changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
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

static GstPadProbeReturn probe_callback(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    guint64 bufferSize = gst_buffer_get_size(buffer);
    guint64 timestamp = GST_BUFFER_TIMESTAMP(buffer);
    static guint64 prevTimestamp = 0;
    static guint64 totalBuffer = 0;

    // Calculate data rate in bits per second
    guint64 durationNs = timestamp - prevTimestamp;

    totalBuffer += bufferSize;

    if (durationNs > GST_SECOND)
    {
        guint64 dataRate = (totalBuffer * 8 * GST_SECOND) / durationNs;

        g_print("Receive Data Rate: %lu bps\n", dataRate);

        totalBuffer = 0;
        prevTimestamp = timestamp;
    }

    return GST_PAD_PROBE_OK;
}

