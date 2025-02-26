/***********************************************************************************************************************************
     This implementation creates an RTSP restreamer using FFmpeg libraries that can receive an RTSP stream and redistribute it to another RTSP endpoint.

    Key features:
        Uses modern C++ with RAII principles
        Thread-safe implementation with proper cleanup
        Handles stream timing and packet conversion
        Proper error handling with exceptions
        Clean shutdown mechanism

    Dependencies:
        FFmpeg libraries (libavcodec, libavformat, libavutil, libswscale)
        C++11 or later compiler

    Compilation requires linking against FFmpeg libraries:

g++ -o RtspRestreamer RtspRestreamer.cpp -lavformat -lavcodec -lavutil -lswscale -pthread

    Usage example:

./RtspRestreamer rtsp://source:554/stream rtsp://destination:554/stream

    Important notes:
        The code handles proper stream timing conversion between input and output
        Includes proper resource cleanup in case of errors
        Supports graceful shutdown
        Thread-safe implementation allows for clean termination

******************************************************************************************************************************************/



#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

extern "C" 
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class RTSPRestreamer 
{
 public:
  RTSPRestreamer(const std::string& inputUrl, const std::string& outputUrl)
      : inputUrl_(inputUrl), outputUrl_(outputUrl), isRunning_(false) {}

  ~RTSPRestreamer() 
  { 
      stop(); 
  }

  void start() 
  {
    if (isRunning_) return;
    isRunning_ = true;
    streamThread_ = std::thread(&RTSPRestreamer::streamLoop, this);
  }

  void stop() 
  {
    isRunning_ = false;
    if (streamThread_.joinable()) 
    {
      streamThread_.join();
    }
  }

 private:
  void streamLoop() 
  {
    AVFormatContext* inputContext = nullptr;
    AVFormatContext* outputContext = nullptr;

    try 
    {
      // Open input
      if (avformat_open_input(&inputContext, inputUrl_.c_str(), nullptr,
                              nullptr) < 0) {
        throw std::runtime_error("Could not open input stream");
      }

      if (avformat_find_stream_info(inputContext, nullptr) < 0) 
      {
        throw std::runtime_error("Could not find stream information");
      }

      // Create output context
      avformat_alloc_output_context2(&outputContext, nullptr, "rtsp",
                                     outputUrl_.c_str());
      if (!outputContext) 
      {
        throw std::runtime_error("Could not create output context");
      }

      // Copy streams
      for (unsigned int i = 0; i < inputContext->nb_streams; i++) {
        AVStream* inStream = inputContext->streams[i];
        AVStream* outStream = avformat_new_stream(outputContext, nullptr);
        if (!outStream) 
        {
          throw std::runtime_error("Failed to allocate output stream");
        }

        if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) <
            0) 
            {
          throw std::runtime_error("Failed to copy codec params");
        }
      }

      // Open output URL
      if (!(outputContext->oformat->flags & AVFMT_NOFILE)) 
      {
        if (avio_open(&outputContext->pb, outputUrl_.c_str(), AVIO_FLAG_WRITE) <  0) 
        {
          throw std::runtime_error("Could not open output URL");
        }
      }

      // Write header
      if (avformat_write_header(outputContext, nullptr) < 0) 
      {
        throw std::runtime_error("Error occurred when writing header");
      }

      // Read packets and write them
      AVPacket packet;
      while (isRunning_ && av_read_frame(inputContext, &packet) >= 0) 
      {
        AVStream* inStream = inputContext->streams[packet.stream_index];
        AVStream* outStream = outputContext->streams[packet.stream_index];

        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

        packet.duration = av_rescale_q(packet.duration, inStream->time_base,outStream->time_base);

        if (av_interleaved_write_frame(outputContext, &packet) < 0) 
        {
          av_packet_unref(&packet);
          throw std::runtime_error("Error while writing packet");
        }
        av_packet_unref(&packet);
      }

      // Write trailer
      av_write_trailer(outputContext);

    } 
    catch (const std::exception& e) 
    {
      std::cerr << "Error: " << e.what() << std::endl;
    }

    // Cleanup
    if (outputContext && !(outputContext->oformat->flags & AVFMT_NOFILE)) 
    {
      avio_closep(&outputContext->pb);
    }
    if (inputContext) 
    {
      avformat_close_input(&inputContext);
    }
    if (outputContext) 
    {
      avformat_free_context(outputContext);
    }
  }

  std::string inputUrl_;
  std::string outputUrl_;
  std::atomic<bool> isRunning_;
  std::thread streamThread_;
};

int main(int argc, char* argv[]) 
{
  if (argc != 3) 
  {
    std::cerr << "Usage: " << argv[0] << " <input_rtsp_url> <output_rtsp_url>"
              << std::endl;
    return 1;
  }

  try
  {
    RTSPRestreamer restreamer(argv[1], argv[2]);
    restreamer.start();

    std::cout << "Press Enter to stop streaming..." << std::endl;
    std::cin.get();

    restreamer.stop();
  } 
  catch (const std::exception& e) 
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
