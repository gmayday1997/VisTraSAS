#include "inference.hpp"
#include "get_latest.hpp"

#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <chrono>
#include "opencv2/opencv.hpp"

using namespace std;

int main(int argc, char** argv)
{ 
  if (argc < 4)
    return -1;

  // cout << "C++ side ::::::::: hello" << endl;
  // string enginename = "/home/rbccps2080ti/projects/TensorRT-Yolov3/yolov3_fp32.engine";
  string enginename = argv[1];
  // cout << enginename << endl;
  Inference iff;
  iff.loadTRTModel(enginename);
  int fd;
  string named_pipe = argv[2];
  // const char* myfifo = "/tmp/fifopipe";
  const char* myfifo = named_pipe.c_str();
  /* create the FIFO (named pipe) */
  mkfifo(myfifo, 0666);
  // cout << "C++ side ::::::::: Waiting for connection to open\n";
  /* write message to the FIFO */
  fd = open(myfifo, O_WRONLY);
  // cout << "C++ side ::::::::: Connected\n";
  

  while (1) {
    // Get list of videos realtime
    string dir_path = argv[3];
    fs::path latest_file = latestFile(dir_path);
    string lat_file_path = latest_file.string();
    string lat_file_name = latest_file.filename().string();
    cout << "latest_file_path " << lat_file_path << endl;
    cout << "latest_file_name " << lat_file_name << endl;
    // cv::VideoCapture cap("rtsp_2019-11-06_17-42-06.flv");
    cv::VideoCapture cap(lat_file_path);
    cout << "videocap success" << endl;
    int frame_num = 0;
    // Check if camera opened successfully
    if(!cap.isOpened()){
      cout << "C++ side ::::::::: Error opening video stream or file" << endl;
      continue;
      // return -1;
    }
    // unsigned char *lat_fil_cstr;
    cout << "before strcpy" << endl;
    // strcpy(lat_fil_cstr, lat_file_name.c_str());
    char* lat_fil_cstr = new char[lat_file_name.length()+1];
    strcpy(lat_fil_cstr, lat_file_name.c_str());
    // unsigned char* lat_fil_cstr = reinterpret_cast<unsigned char*>(lat_file_name.c_str()); 
    cout << "latest file strcpy :: " << lat_fil_cstr << endl;
    cout << "latest file len :: " << strlen(lat_fil_cstr) << endl;
    write(fd, lat_fil_cstr, strlen(lat_fil_cstr)+1);
    delete [] lat_fil_cstr;
    while(1){
      auto start = chrono::high_resolution_clock::now();
      cv::Mat frame;
      // Capture frame-by-frame
      cap >> frame;
  
      // If the frame is empty, break immediately
      // cout << "C++ side %%%%%%%%%%%" << frame.size() << endl;
      // cv::imshow("asdf", frame);
      // cv::waitKey(0);
      if (frame.empty())
	break; 

      // cout << "C++ side ::::::::: frame_num ::::::::: " << frame_num << endl;
      frame_num++;
      

      vector<Bbox> op1 = iff.infer_single_image(frame);
      frame.release();

      if (op1.empty())
	{
	  char delim_char = (unsigned char) 0;
	  cout << "delim :: number :: " << op1.size() << endl;
	  cout << "delim :: sizeof :: " << sizeof(delim_char) << endl;
	  write(fd, &delim_char, sizeof(delim_char));
	  continue;
	}

      // int delim = op1.size();
      char delim_char = (unsigned char) op1.size();
      // cout << "delim :: number :: " << op1.size() << endl;
      // cout << "delim :: sizeof :: " << sizeof(delim_char) << endl;
      write(fd, &delim_char, sizeof(delim_char));
      // fwrite(delim_char, sizeof(delim_char));
      // unsigned char total_box[24*op1.size()];
      // cout << "C++ side ::::::::: sizeof total_box ::::::::: " << sizeof(total_box) << endl;
      
      // auto first = true;
      // auto i = 0;
	
      for(const auto& item : op1)  
	{
	  // cout << "C++ side ::::::::: size of item ::::::::: " << sizeof(item) << endl;
	  // string newbox = reinterpret_cast<string>(item);
	  // char* box = const_cast<char*>(reinterpret_cast<const char*>(&item));
	  // string box_s(box);
	  // if (first) {
	  //   // strcpy(total_box, box);
	  //   memcpy(total_box, box, 24);
	  //   cout << "C++ side ::::::::: size of box ::::::::: " << sizeof(box) << endl;
	  //   first = false;
	  // }
	  // else 
	  //   memcpy(total_box+(24*i), box, 24);
	  // i++;
	  // cout << "C++ side ::::::::: class=" << item.classId << " prob=" << item.score*100 << endl;  
	  // cout << "C++ side ::::::::: left=" << item.left << " right=" << item.right << " top=" << item.top << " bot=" << item.bot << endl;  

	  unsigned char* box = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(&item));
	  // cout << "C++ size ::::::::: size of BOX :::::::: " << sizeof(box) << endl;
	  // total_box += box;
	  // strcat(total_box, box);
	  // fwrite(box, 24,1,stdout);
	  write(fd, box, sizeof(item));
	  
	  // fwrite(newbox, sizeof(newbox), 1,stdout);
	}
      // cout << "C++ side ::::::::: write finished" << endl;
      // this_thread::sleep_for(chrono::seconds(1));
      auto stop = chrono::high_resolution_clock::now();
      auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
      cout << "Duration for complete C++ side yolo inference : " << duration.count() << "ms" << endl;
    }
    check_and_delete(dir_path);

    }
  close(fd);
  // cout << "C++ side ::::::::: closed finished" << endl;
  /* remove the FIFO */
    unlink(myfifo);

    // cout << "C++ side ::::::::: unlink finished" << endl;

    return 0;

}
