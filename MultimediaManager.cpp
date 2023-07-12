#include "multimediaManager.h"
#include "MultimediaSender.h"
#include "MultimediaReceiver.h"
#include <gst/gst.h>

std::mutex MultimediaManager::instanceMutex;

MultimediaManager& MultimediaManager::GetInstance() {
    // 멀티스레드 환경에서 안전한 인스턴스 생성을 보장하기 위해 동기화
    std::lock_guard<std::mutex> lock(instanceMutex);
    static MultimediaManager instance;
    return instance;
}

MultimediaManager::MultimediaManager() 
    : mSender(MultimediaSender::GetInstance())
{
    // Initialize GStreamer
    gst_init(NULL, NULL);
};

MultimediaManager::~MultimediaManager() {
    mSender.stop();
    mSender.cleanup();

    for (auto& pair : receiverMap) {
        pair.second->stop();
        pair.second->cleanup();
        delete pair.second;  // Delete the object
    }

    gst_deinit();
}

bool MultimediaManager::setupSender(void* hVideo, std::string ip, int videoPort, int audioPort) {

    mSender = MultimediaSender::GetInstance();

    // Initialize sender pipelines
    if (!mSender.initialize())
    {
        std::cerr << "Failed to initialize sender pipelines." << std::endl;
        return true;
    }

    // Set video resolution
    dynamic_cast<MultimediaSender&>(mSender).setVideoResolution();

    // Set receiver IP and port - Todo : change to Server IP and Port
    dynamic_cast<MultimediaSender&>(mSender).setReceiverIP(ip);
    dynamic_cast<MultimediaSender&>(mSender).setPort(videoPort, audioPort);

    // Set camera index (if necessary)
    dynamic_cast<MultimediaSender&>(mSender).setCameraIndex(0);

    // Set video flip method (if necessary)
    dynamic_cast<MultimediaSender&>(mSender).setVideoFlipMethod(4); // Horizontal flip

    // Set video encoding tune (if necessary)
    dynamic_cast<MultimediaSender&>(mSender).setVideoEncTune();
    dynamic_cast<MultimediaSender&>(mSender).setVideoEncBitRate();

    // Set audio encoding type (if necessary)
    //int audioType, 
	//generic (2049), voice (2048), restricted-lowdelay (2051) 
	//int bandwidth
	//narrowband (1101), mediumband(1102), wideband(1103), superwideband (1104), fullband (1105), auto (-1000)
	//int bitrate
	//default : 64000
	//int bitrateType
	//cbr (0) ??CBR, vbr (1), constrained-vbr (2)

    dynamic_cast<MultimediaSender&>(mSender).setAudioOpusencAudioType(2051, 1105, 64000, 2); //fist pre-set
    //dynamic_cast<MultimediaSender&>(mSender).setAudioOpusencAudioType(2048, 1105, 192000, 1); //High-voip Quality
    //dynamic_cast<MultimediaSender&>(mSender).setAudioOpusencAudioType(2049, 1105, 192000, 0); //High Quality
    //dynamic_cast<MultimediaSender&>(mSender).setAudioOpusencAudioType(2048, 1103, 128000, 1); //Mid Quality
	//dynamic_cast<MultimediaSender&>(mSender).setAudioOpusencAudioType(2048, -1000, 64000, 2); //Low voip

    mSender.setWindow(hVideo);

    return true;
}

bool MultimediaManager::setupReceiver(void* hVideo, int videoPort, int audioPort, int id)
{
    MultimediaReceiver* receiver;
    if (!receiverMap[id])
    {
        receiver = new MultimediaReceiver(); // MultimediaReceiver 객체 생성
        receiverMap[id] = receiver;
    }
    else
    {
        receiver = static_cast<MultimediaReceiver*>(receiverMap[id]);
    }

    if (!receiver->initialize())
    {
        std::cerr << "Failed to initialize MultimediaReceiver." << std::endl;
        return -1;
    }
    receiver->setId(id);

    receiver->setPort(videoPort, audioPort);     // Set receive port

    receiver->setWindow(hVideo);       // Set window handle to show window
    
    return true;
}

bool MultimediaManager::acceptCall(/* Callee 객체 받기 */void* hVideo)
{
    return true;
}

void MultimediaManager::makeCall()
{
    mSender.runThread();
 }

void MultimediaManager::pauseCall()
{
    mSender.stop();
}

void MultimediaManager::endCall()
{
    mSender.stop();

    for (auto& pair : receiverMap) {
        pair.second->stop();
        pair.second->cleanup();
        delete pair.second;  // Delete the object
    }
    receiverMap.clear();  // Clear the map
}

// For debugging
void MultimediaManager::pauseReceiver(int video)
{
    receiverMap[video]->stop();
}

void MultimediaManager::playReceiver(int video)
{
    receiverMap[video]->runThread();
}

void MultimediaManager::makeReceiverStateChange(int video, int state)
{
    MultimediaReceiver* receiver = static_cast<MultimediaReceiver*>(receiverMap[video]);
    receiver->changeState(state);
}