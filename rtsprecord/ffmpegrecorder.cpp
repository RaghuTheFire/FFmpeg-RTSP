
#include <iostream>
#include "video_recoder.h"
/*************************************************************/
int test_VideoRecoder();
/*************************************************************/
int main()
{    
    test_VideoRecoder();
}
/*************************************************************/
int test_VideoRecoder()
{    
    VideoRecoder recoder("rtsp://172.196.128.151:554/1/h264minor","test.mp4");
    recoder.Start();    
    std::this_thread::sleep_for(std::chrono::seconds(15));
    if (!recoder.Stop())
    {
        std::cout << "recoder failed" << std::endl;
    }    
    return 0;
}
/*************************************************************/
