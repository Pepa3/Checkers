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
	cv::namedWindow("original");
	cv::setMouseCallback("original",mouseCallback);
	bool quit = false;
	vector<cv::String> fn;
	cv::glob(argv[1], fn, false);

	size_t count = fn.size();
	vector<cv::Mat> images = vector<cv::Mat>(count);
	cout << "Found " << count << " images" << endl;
	for(size_t i = 0; i < count; i++)
		cv::cvtColor(cv::imread(fn[i]),images[i], cv::COLOR_BGR2GRAY);//HSV is worse
	size_t current = 0;
	cv::Mat mTrans, mProc;
	cv::Mat Irender, Iorig, Itrans, ItransRend, ItransLast, Idiff;

	cv::resize(images[current], Iorig, {width, height});
	Itrans = Iorig.clone();

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
					ItransLast = Itrans.clone();
					cv::warpPerspective(Iorig,Itrans,mTrans,{width,height});
					cv::absdiff(Itrans, ItransLast, Idiff);
					mProc = h::extract(Idiff);
					ItransRend = Itrans.clone();
					for(int i = 1; i <= 8; i++){
						cv::line(ItransRend, cv::Point2f(i * (width / 8), 0), cv::Point2f(i * width / 8, height), cv::Scalar(0, 255, 0));
						cv::line(ItransRend, cv::Point2f(0, i * (height / 8)), cv::Point2f(width, i * (height / 8)), cv::Scalar(0, 255, 0));
					}
					/*
					cv::Mat canny_output;
					cv::Canny(src_gray, canny_output, thresh, thresh * 2);

					vector<vector<cv::Point> > contours;
					vector<cv::Vec4i> hierarchy;
					cv::findContours(canny_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

					cv::Mat drawing = cv::Mat::zeros(canny_output.size(), cv::CV_8UC3);
					for(size_t i = 0; i < contours.size(); i++){
						cv::Scalar color = cv::Scalar(rng.uniform(0, 256), cv::rng.uniform(0, 256), cv::rng.uniform(0, 256));
						cv::drawContours(drawing, contours, (int) i, color, 2, cv::LINE_8, hierarchy, 0);
					}

					imshow("Contours", drawing);
					*/
					//cv::imshow("transformed", ItransRend);
					cv::imshow("processed", mProc);
					cv::imshow("difference", Idiff);
					break;
				case 'n':
					current++;
					cv::resize(images[current], Iorig, cv::Size(900, 700));
					if(current>count)
						current--;
					break;
			}
		}
		Irender = Iorig.clone();
		h::p4(Irender,cv::Scalar(255,255,255),corners[0],corners[1],corners[2],corners[3]);
		bool first = true;
		cv::Point2f p2 = cv::Point2f(0,0);
		for(auto& p : chessboard){
			if(first){
				p2=p;
				first=false;
				continue;
			}
			cv::line(Irender,p2,p,cv::Scalar(255, 255, 255));
			p2=p;
		}
		cv::imshow("original", Irender);
	}
	cv::destroyAllWindows();
	return 0;
}
