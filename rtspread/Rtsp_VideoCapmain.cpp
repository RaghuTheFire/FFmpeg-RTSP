//g++ -o Rtsp_VideoCapture Rtsp_VideoCapmain.cpp Rtsp_VideoCapture.cpp -L/usr/local/lib -lavutil -lavcodec -lavformat -ldl -L/usr/lib -lpthread -lz -lswscale -lm `pkg-config --cflags --libs opencv4`

#include <iostream>
#include "Rtsp_VideoCapture.h"
/*************************************************************/
int Rtsp_VideoCapture();
/*************************************************************/
int main()
{
    Rtsp_VideoCapture();    
}
/*************************************************************/
int Rtsp_VideoCapture()
{
    cv::Mat frame;
    //VideoCapture cap("rtsp://service:service@172.196.129.152:554/ufirststream?inst=2"); //BEL PF OFFICE
    VideoCapture cap("rtsp://172.196.128.151:554/1/h264minor");
    //VideoCapture cap("rtsp://admin:admin@172.196.128.155:554/videoinput_1:0/h264_2/media.stm");
    //VideoCapture cap("rtsp://admin:admin@172.196.128.155:554/videoinput_1:0/h264_1/media.stm");
    
    if (!cap.isOpened())
    {
        std::cout << "Open capture failed";
        return 0;
    }
    
    while (1)
    {        
        cap >> frame;
        if (frame.empty())
            break;
        else                
        cv::imshow("ffmpeg rtsp capture", frame);        
        if(cv::waitKey(1) != -1)
            break;       
    }    
}
/*************************************************************/
