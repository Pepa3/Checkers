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
cv::Point2f corners[4] = rect(width,height,100);
int ncorners = 0;
vector<cv::Point2f> chessboard;
const cv::Point2f window[4] = rect(width,height,0);

void mouseCallback(int event, int x, int y, int flags, void *userdata){
	(void)flags;(void)userdata;
	if(event==cv::EVENT_LBUTTONDOWN){
		corners[ncorners]=cv::Point2f(x,y);
		ncorners++;
		ncorners %= 4;
	}
}

int main(int argc, char** argv){
	if(argc<2){
		cout << "Provide pictures" << endl;
		return -1;
	}
//	cv::VideoCapture cap = cv::VideoCapture(ci,cv::CAP_ANY);
//	if(!cap.isOpened()){
//		cout << "Cannot open the camera!" << endl;
//		return 1;
//	}
	cv::namedWindow("original");
	cv::setMouseCallback("original",mouseCallback);
	bool quit = false;//, doCapture=false;
//	chrono::nanoseconds time = chrono::high_resolution_clock::now().time_since_epoch();
	vector<cv::String> fn;
	cv::glob(argv[1], fn, false);
	
	vector<cv::Mat> images;
	size_t count = fn.size(); //number of png files in images folder
	cout << "Found " << count << " images" << endl;
	for (size_t i=0; i<count; i++)
	    images.push_back(cv::imread(fn[i]));
	size_t current = 0;
	cv::Mat mTrans, mProc;
	cv::Mat Irender, Iorig, Itrans, ItransRend;
	cv::resize(images[current], Iorig, {width, height});

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
					mTrans = cv::getPerspectiveTransform(corners,window);
					cv::warpPerspective(Iorig,Itrans,mTrans,{width,height});
					mProc = h::extract(Itrans);
					cv::imshow("processed", mProc);
					ItransRend = Itrans.clone();
					for(int i = 1; i <= 8; i++){
						cv::line(ItransRend, cv::Point2f(i * (width / 8), 0), cv::Point2f(i * width / 8, height), cv::Scalar(0, 255, 0));
						cv::line(ItransRend, cv::Point2f(0, i * (height / 8)), cv::Point2f(width, i * (height / 8)), cv::Scalar(0, 255, 0));
					}
					cv::imshow("transformed", ItransRend);
					/*if(!cv::findChessboardCorners(f2, cv::Size(6, 6), chessboard, cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE)){
						cout << "Couldn't find the chessboard" << endl;
					}else{
						cout << "Found the chessboard!" << endl;	
					}*/
					break;
				case 'n':
					current++;
					cv::resize(images[current], Iorig, cv::Size(900, 700));
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
		Irender = Iorig.clone();
		h::p4(Irender,cv::Scalar(0,255,0),corners[0],corners[1],corners[2],corners[3]);
		bool first = true;
		cv::Point2f p2 = cv::Point2f(0,0);
		for(auto& p : chessboard){
			if(first){
				p2=p;
				first=false;
				continue;
			}
			cv::line(Irender,p2,p,cv::Scalar(0,255,0));
			p2=p;
		}
		cv::imshow("original", Irender);
	}
	cv::destroyAllWindows();
	return 0;
}
