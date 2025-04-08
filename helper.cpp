#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <array>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>

namespace h{
	const size_t width = 900, height = 700;
#define rect(width,height,offset) (cv::Point2f[4]){cv::Point2f{(offset),(offset)},{(width)-(offset),(offset)},{(width)-(offset),(height)-(offset)},{(offset),(height)-(offset)}}

void p4(cv::Mat frame, const cv::Scalar color, const cv::Point2f p1, const cv::Point2f p2, const cv::Point2f p3, const cv::Point2f p4){
	cv::line(frame,p1,p2,color);
	cv::line(frame,p2,p3,color);
	cv::line(frame,p3,p4,color);
	cv::line(frame,p4,p1,color);
}

cv::Mat extract(cv::Mat in){
	cv::Mat out = cv::Mat::zeros(8, 8, CV_8UC1);
	for(size_t i = 0; i < 8; i++){
		uchar* o1 = out.ptr<uchar>(i);
		for(size_t j = 0; j < 8; j++){
			uchar mean = cv::mean(cv::Mat(in, cv::Rect(j * (width / 8), i * (height / 8), width / 8, height / 8)))[0];
			o1[j] = mean;
		}
	}
	return out;
}

}
