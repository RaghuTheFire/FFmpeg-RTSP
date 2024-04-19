# rtspread

# How to Compile
g++ -o Rtsp_VideoCapture Rtsp_VideoCapmain.cpp Rtsp_VideoCapture.cpp -L/usr/local/lib -lavutil -lavcodec -lavformat -ldl -L/usr/lib -lpthread -lz -lswscale -lm `pkg-config --cflags --libs opencv4`

# rtsprecord
# How to Compile
g++ -o ffmpegrecorderdemo ffmpegrecorder.cpp video_recoder.cpp -L/usr/local/lib -lavutil -lavcodec -lavformat -ldl -L/usr/lib -lpthread -lz -lswscale -lm 
