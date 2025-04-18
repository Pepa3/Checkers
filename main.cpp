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
		images[i]=cv::imread(fn[i]);
		//cv::cvtColor(cv::imread(fn[i]),images[i], cv::COLOR_BGR2GRAY);//HSV is worse
	size_t current = 0;
	cv::Mat mTrans;
	cv::Mat Irender, Iorig, Itrans, ItransRend, IprocLast, Idiff, Iproc, Icontours;
	cv::Mat canny_output;
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	cv::Point a = cv::Point(0,0),b = cv::Point(0,1);
	double c = 0, d = 0;

	cv::resize(images[current], Iorig, {width, height});
	Iproc = cv::Mat::zeros(8, 8, CV_8UC1);
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
					//transform mTrans
					mTrans = cv::getPerspectiveTransform(corners,window);
					//warp Itrans
					cv::warpPerspective(Iorig,Itrans,mTrans,{width,height});
					//Canny canny_out
					cv::Canny(Itrans, canny_output, 70, 150);
					cv::findContours(canny_output, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
					//contours Icontours
					Icontours = cv::Mat::zeros(canny_output.size(), CV_8UC3);
					for(size_t i = 0; i < contours.size(); i++){
						cv::drawContours(Icontours, contours, (int) i, cv::Scalar(255, 255, 255), 2, cv::LINE_8, hierarchy, 0);
					}
					cv::imshow("contours", Icontours);
					//copy last proc
					IprocLast = Iproc.clone();
					//processed
					Iproc = h::extract(Icontours);
					cv::imshow("processed", Iproc);
					//diff Idiff
					cv::absdiff(Iproc, IprocLast, Idiff);
					cv::imshow("difference", Idiff);
					ItransRend = Itrans.clone();
					for(int i = 1; i <= 8; i++){
						cv::line(ItransRend, cv::Point2f(i * (width / 8), 0), cv::Point2f(i * width / 8, height), cv::Scalar(0, 255, 0));
						cv::line(ItransRend, cv::Point2f(0, i * (height / 8)), cv::Point2f(width, i * (height / 8)), cv::Scalar(0, 255, 0));
					}
					c=0;d=0;
					for(int i = 0;i<64;i++){
						double x = Idiff.ptr<uchar>(i/8)[i%8];
						if(x>c){
							d=c;
							c=x;
							b=a;
							a=cv::Point((i%8)*width/8,(i/8)*height/8+30);
						}
						if(x<c&&x>=d){
							d=x;
							b=cv::Point((i%8)*width/8,(i/8)*height/8+30);
						}
					}
					cv::putText(ItransRend, "first", a, cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(0,0,255));
					cv::putText(ItransRend, "second", b, cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(0,0,255));
					cv::imshow("transformed", ItransRend);
					break;
				case 'n':
					current++;
					cv::resize(images[current], Iorig, cv::Size(width, height));
					if(current>count)
						current--;
					break;
				case 'b':
					if(current!=0)
					current--;
					cv::resize(images[current], Iorig, cv::Size(width, height));
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
