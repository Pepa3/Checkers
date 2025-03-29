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

using namespace std;

cv::Point2f corners[4];
int ncorners = 0;
cv::Mat image;
vector<cv::Point2f> chessboard;

void mouseCallback(int event, int x, int y, int flags, void *userdata){
	if(event==cv::EVENT_LBUTTONDOWN){
		if(ncorners<4){
			corners[ncorners]=cv::Point2f(x,y);
			ncorners++;
		}else{
			cout << "All 4 corners already selected. Reset with 'R' key." << endl;
		}
	}
}

int main(int argc, char** argv){
	if(argc<2){
		cout << "Provide camera index(0)" << endl;
		return -1;
	}
	int ci = stoi(argv[1]);
	cout << "Hello, World!" << endl;
	cv::VideoCapture cap = cv::VideoCapture(ci,cv::CAP_ANY);
	if(!cap.isOpened()){
		cout << "Cannot open the camera!" << endl;
		return 1;
	}
	cv::namedWindow("camera");
	cv::setMouseCallback("camera",mouseCallback);
	cv::Mat frame, gray;
	bool quit = false, doCapture=false;
	chrono::nanoseconds time = chrono::high_resolution_clock::now().time_since_epoch();
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
			}
		}
		cap.read(frame);
		if(frame.empty()){
			cerr << "ERROR! blank frame grabbed" << endl;	
        		break;
		}
		//cout << time << endl;
		if(chrono::high_resolution_clock::now().time_since_epoch()-time>=1s){
			cv::cvtColor(frame,image,cv::COLOR_BGR2GRAY);
			bool found = cv::findChessboardCorners(image,cv::Size(7,7),chessboard,cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE + cv::CALIB_CB_FAST_CHECK);
			if(!found){
				cout << "Couldn't find the chessboard" << endl; 
			}
			time=chrono::high_resolution_clock::now().time_since_epoch();
		}
		cv::Point2f p2 = cv::Point2f(0,0);
		for(auto& p : chessboard){
			cv::line(frame,p2,p,cv::Scalar(0,255,0));
			p2=p;
		}
		cv::imshow("camera", frame);
	}
	return 0;
}
