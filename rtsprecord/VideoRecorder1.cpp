
#include <iostream>
#include <string>
#include <future>
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

class VideoRecorder 
{
  private:
      std::string rtsp_url;
      std::string file_name;
      bool recording;
      std::future<void> handler_future;
      AVFormatContext* output_context;
      AVStream* output_stream;
  
  public:
      VideoRecorder(const std::string& rtsp_url, const std::string& file_name) 
      {
          this->rtsp_url = rtsp_url;
          this->file_name = file_name;
          this->recording = false;
      }
  
      bool start() 
      {
          this->recording = true;
  
          AVDictionary* options = nullptr;
          av_dict_set(&options, "buffer_size", "1024000", 0); // Set the cache size, 1080p can jump the value to the maximum
          av_dict_set(&options, "rtsp_transport", "tcp", 0); // open with tcp
          av_dict_set(&options, "stimeout", "5000000", 0); // Set the timeout disconnection time, the unit is microseconds
          av_dict_set(&options, "max_delay", "500000", 0); // set maximum delay
  
          AVFormatContext* input_context = nullptr;
          int ret = avformat_open_input(&input_context, this->rtsp_url.c_str(), nullptr, &options);
          if (ret < 0) 
          {
              std::cerr << "Failed to open input stream: " << av_err2str(ret) << std::endl;
              return false;
          }
  
          ret = avformat_find_stream_info(input_context, nullptr);
          if (ret < 0) 
          {
              std::cerr << "Failed to find stream information: " << av_err2str(ret) << std::endl;
              avformat_close_input(&input_context);
              return false;
          }
  
          ret = avformat_alloc_output_context2(&this->output_context, nullptr, nullptr, this->file_name.c_str());
          if (ret < 0) 
          {
              std::cerr << "Failed to create output context: " << av_err2str(ret) << std::endl;
              avformat_close_input(&input_context);
              return false;
          }
  
          AVStream* input_stream = input_context->streams[0];
          this->output_stream = avformat_new_stream(this->output_context, input_stream->codec->codec);
          if (!this->output_stream)
          {
              std::cerr << "Failed to create output stream" << std::endl;
              avformat_close_input(&input_context);
              avformat_free_context(this->output_context);
              return false;
          }
  
          ret = avcodec_parameters_copy(this->output_stream->codecpar, input_stream->codecpar);
          if (ret < 0) 
          {
              std::cerr << "Failed to copy codec parameters: " << av_err2str(ret) << std::endl;
              avformat_close_input(&input_context);
              avformat_free_context(this->output_context);
              return false;
          }
  
          this->output_stream->codecpar->codec_tag = 0;
          av_dump_format(this->output_context, 0, this->file_name.c_str(), 1);
  
          ret = avio_open(&this->output_context->pb, this->file_name.c_str(), AVIO_FLAG_WRITE);
          if (ret < 0) 
          {
              std::cerr << "Failed to open output file: " << av_err2str(ret) << std::endl;
              avformat_close_input(&input_context);
              avformat_free_context(this->output_context);
              return false;
          }
  
          ret = avformat_write_header(this->output_context, nullptr);
          if (ret < 0) 
          {
              std::cerr << "Failed to write header: " << av_err2str(ret) << std::endl;
              avio_closep(&this->output_context->pb);
              avformat_close_input(&input_context);
              avformat_free_context(this->output_context);
              return false;
          }
  
          AVPacket packet;
          while (this->recording && av_read_frame(input_context, &packet) >= 0) 
          {
              av_packet_rescale_ts(&packet, input_context->streams[packet.stream_index]->time_base, this->output_stream->time_base);
              packet.stream_index = this->output_stream->index;
              ret = av_interleaved_write_frame(this->output_context, &packet);
              av_packet_unref(&packet);
              if (ret < 0) 
              {
                  std::cerr << "Failed to write frame: " << av_err2str(ret) << std::endl;
                  break;
              }
          }
  
          av_write_trailer(this->output_context);
          avio_closep(&this->output_context->pb);
          avformat_close_input(&input_context);
          avformat_free_context(this->output_context);
  
          std::cout << "Recording completed" << std::endl;
          return true;
      }
  
      bool stop() 
      {
          this->recording = false;
          return true;
      }
};
