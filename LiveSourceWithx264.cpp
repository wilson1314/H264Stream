#include "LiveSourceWithx264.h"


LiveSourceWithx264* LiveSourceWithx264::createNew(UsageEnvironment& env)
{
    return new LiveSourceWithx264(env);
}

EventTriggerId LiveSourceWithx264::eventTriggerId = 0;

unsigned LiveSourceWithx264::referenceCount = 0;

LiveSourceWithx264::LiveSourceWithx264(UsageEnvironment& env):FramedSource(env)
{
    if(referenceCount == 0)
    {

    }
    ++referenceCount;
    videoCaptureDevice.open(0);
    videoCaptureDevice.set(CV_CAP_PROP_FRAME_WIDTH,1920);
    videoCaptureDevice.set(CV_CAP_PROP_FRAME_HEIGHT,1080);
    encoder = new x264Encoder();
    encoder->initilize();
    if(eventTriggerId == 0)
    {
        eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    }
    count_frame_ = 0;
    std::cout << "haha----1" << std::endl;
}


LiveSourceWithx264::~LiveSourceWithx264(void)
{
    --referenceCount;
    videoCaptureDevice.release();
    encoder->unInitilize();
    //envir().taskScheduler().deleteEventTrigger(eventTriggerId);
    eventTriggerId = 0;
    count_frame_ = 0;
}

void LiveSourceWithx264::encodeNewFrame()
{
        rawImage.data = NULL;
        while(rawImage.data == NULL)
        {
            videoCaptureDevice >> rawImage;
//            cv::waitKey(100);
        }
//        cv::imshow("win", rawImage);
//        cv::waitKey(20);
        count_frame_++;
//        std::cout << "Current Frame Number: " << count_frame_ << std::endl;
        // Got new image to stream
        assert(rawImage.data != NULL);
        encoder->encodeFrame(rawImage);
//        cv::imshow("win", rawImage);
//        cv::waitKey(20);
        // Take all nals from encoder output queue to our input queue
        while(encoder->isNalsAvailableInOutputQueue() == true)
        {
//            std::cout << "haha----2" << std::endl;
            x264_nal_t nal = encoder->getNalUnit();
            nalQueue.push(nal);
        }
}

void LiveSourceWithx264::deliverFrame0(void* clientData)
{
    ((LiveSourceWithx264*)clientData)->deliverFrame();
    std::cout << "haha----deliverFrame0" << std::endl;
}

void LiveSourceWithx264::doGetNextFrame()
{
    if(nalQueue.empty() == true)
    {
        encodeNewFrame();
        gettimeofday(&currentTime,NULL);
        deliverFrame();
    }
    else
    {
        deliverFrame();
    }
}

void LiveSourceWithx264::deliverFrame()
{
    std::cout << "haha----1" << std::endl;
    if(!isCurrentlyAwaitingData())
    {
        std::cout << "haha----error-error" << std::endl;
        return;
    }
    x264_nal_t nal = nalQueue.front();
    nalQueue.pop();
    assert(nal.p_payload != NULL);
    // You need to remove the start code which is there in front of every nal unit.  
    // the start code might be 0x00000001 or 0x000001. so detect it and remove it. pass remaining data to live555    
    int trancate = 0;
    std::cout << "haha----2" << std::endl;
    if (nal.i_payload >= 4 && nal.p_payload[0] == 0 && nal.p_payload[1] == 0 && nal.p_payload[2] == 0 && nal.p_payload[3] == 1 )
    {
        trancate = 4;
            std::cout << "haha----3" << std::endl;
    }
    else
    {
        if(nal.i_payload >= 3 && nal.p_payload[0] == 0 && nal.p_payload[1] == 0 && nal.p_payload[2] == 1 )
        {
            trancate = 3;
                std::cout << "haha----4" << std::endl;
        }
            std::cout << "haha----5" << std::endl;
    }

    if(nal.i_payload-trancate > fMaxSize)
    {
        std::cout << "haha----6" << std::endl;
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = nal.i_payload-trancate - fMaxSize;
    }
    else
    {
        fFrameSize = nal.i_payload-trancate;
            std::cout << "haha----7" << std::endl;
    }
    fPresentationTime = currentTime;
    memmove(fTo,nal.p_payload+trancate,fFrameSize);
    std::cout << "haha----8" << std::endl;
    FramedSource::afterGetting(this);
    std::cout << "haha----9" << std::endl;
    std::cout << "haha----deliverFrame" << std::endl;
} 
