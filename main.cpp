#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include "helper.cpp"

using namespace std;

const size_t width = 900, height = 700;
cv::Point2f* corners = h::rect(width,height,100);
int ncorners = 0;
vector<cv::Point2f> chessboard;
const cv::Point2f * const window = h::rect(width,height);

void mouseCallback(int event, int x, int y, int flags, void *userdata){
	(void)flags;(void)userdata;
	if(event==cv::EVENT_LBUTTONDOWN){
		corners[ncorners]=cv::Point2f(x,y);
		ncorners++;
		ncorners %= 4;
	}
}

int main(int argc, char** argv){
	(void)argc;(void)argv;
	//if(argc<2){
	//	cout << "Provide picture folder" << endl;
	//	return -1;
	//}
//	cv::VideoCapture cap = cv::VideoCapture(ci,cv::CAP_ANY);
//	if(!cap.isOpened()){
//		cout << "Cannot open the camera!" << endl;
//		return 1;
//	}
	cv::namedWindow("camera");
	cv::setMouseCallback("camera",mouseCallback);
	bool quit = false;//, doCapture=false;
//	chrono::nanoseconds time = chrono::high_resolution_clock::now().time_since_epoch();
	vector<cv::String> fn;
	cv::glob("./pic/*.jpg", fn, false);
	
	vector<cv::Mat> images;
	size_t count = fn.size(); //number of png files in images folder
	for (size_t i=0; i<count; i++)
	    images.push_back(cv::imread(fn[i]));
	size_t current = 0;
	cv::Mat frame, f2, tr;
	while(!quit){
		int key;
		while((key=cv::pollKey())!=-1){
			switch(key&0xff){
				case 'r':
					ncorners=0;
					break;
				case 'q':
					quit=true;
					break;
				case 't':
					tr = cv::getPerspectiveTransform(corners,window);
					cv::warpPerspective(frame,f2,tr,{900,700});
					cv::imshow("out",f2);
					
					if(!cv::findChessboardCorners(f2,cv::Size(7,7),chessboard,cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE)){
						cout << "Couldn't find the chessboard" << endl;
					}else{
						cout << "Found the chessboard!" << endl;	
					}
					break;
				case 'n':
					current++;
					if(current>count)
						current--;
					break;
			}
		}
		//cap.read(frame);
		//if(frame.empty()){
		//	cerr << "ERROR! blank frame grabbed" << endl;
        	//	break;
		//}
		//cout << time << endl;
		//if(chrono::high_resolution_clock::now().time_since_epoch()-time>=1s){
		//cv::cvtColor(frame,image,cv::COLOR_BGR2GRAY);
		
		//time=chrono::high_resolution_clock::now().time_since_epoch();
		//}
		cv::resize(images[current],frame,cv::Size(900,700));
		h::p4(frame,cv::Scalar(0,255,0),corners[0],corners[1],corners[2],corners[3]);
		bool first = true;
		cv::Point2f p2 = cv::Point2f(0,0);
		for(auto& p : chessboard){
			if(first){
				p2=p;
				first=false;
				continue;
			}
			cv::line(frame,p2,p,cv::Scalar(0,255,0));
			p2=p;
		}
		cv::imshow("camera", frame);
	}
	return 0;
}
