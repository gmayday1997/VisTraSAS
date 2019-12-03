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
  string enginename = argv[1];
  if (enginename.empty())
    {
      cerr << "Invalid Engine Name" << endl;
      exit(EXIT_FAILURE);
    }
  Inference iff;
  if (!iff.loadTRTModel(enginename)) {
    cerr << "Failed to load engine file" << endl;
    exit(EXIT_FAILURE);
  }
  
  int fd;
  string named_pipe = argv[2];
  if (named_pipe.empty())
    {
      cerr << "Invalid PIPE Name" << endl;
      exit(EXIT_FAILURE);
    }
  const char* myfifo = named_pipe.c_str();
  mkfifo(myfifo, 0666);
  fd = open(myfifo, O_WRONLY);
  if (fd==-1) {
    cerr << "Failed to establish Pipe connection" << endl;
    exit(EXIT_FAILURE);
  }
  string dir_path = argv[3];
  if (dir_path.empty())
    {
      cerr << "Directory does not exist" << endl;
      exit(EXIT_FAILURE);
    }

  string previous_file;

  while (1) {
    fs::path latest_file = latestFile(dir_path);
    if (latest_file.empty())
      {
	cerr << "Waiting for initial files" << endl;
	continue;
      }
    string lat_file_path = latest_file.string();
    string lat_file_name = latest_file.filename().string();
    if (previous_file.empty())
      {
	previous_file = lat_file_name;
      }
    else if (lat_file_name.compare(previous_file)==0)
      continue;
    else
      previous_file = lat_file_name;
    cout << "latest_file_path " << lat_file_path << endl;
    cout << "latest_file_name " << lat_file_name << endl;
    cv::VideoCapture cap(lat_file_path);
    cout << "videocap success" << endl;
    int frame_num = 0;
    if(!cap.isOpened()){
      cout << "C++ side ::::::::: Error opening video stream or file" << endl;
      continue;
    }
    cout << "before strcpy" << endl;
    char* lat_fil_cstr = new char[lat_file_name.length()+1];
    strcpy(lat_fil_cstr, lat_file_name.c_str());
    cout << "latest file strcpy :: " << lat_fil_cstr << endl;
    cout << "latest file len :: " << strlen(lat_fil_cstr) << endl;
    write(fd, lat_fil_cstr, strlen(lat_fil_cstr)+1);
    delete [] lat_fil_cstr;
    while(1){
      auto start = chrono::high_resolution_clock::now();
      cv::Mat frame;
      cap >> frame;
      if (frame.empty())
	break;
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
      char delim_char = (unsigned char) op1.size();
      write(fd, &delim_char, sizeof(delim_char));
	
      for(const auto& item : op1)  
	{
	  unsigned char* box = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(&item));
	  write(fd, box, sizeof(item));
	}
      auto stop = chrono::high_resolution_clock::now();
      auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
      cout << "Duration for complete C++ side yolo inference : " << duration.count() << "ms" << endl;
    }
    check_and_delete(dir_path);

  }
  close(fd);
    unlink(myfifo);
    return 0;

}
