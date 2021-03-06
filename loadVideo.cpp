// loadVideo.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "pch.h"
#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <vector>
#include <numeric>
#include <stdlib.h>
#include <stdio.h>

using namespace std;
using namespace cv;


//Bilateral Filtering
int KERNEL_LENGTH = 20;
int DELAY_CAPTION = 1500;
int DELAY_BLUR = 100;

const char* fileName = "C:\\Users\\jaceh\\Desktop\\Computer Vision Projects\\LoadVideo\\IMG_2088.mov";

Mat frame; //Used for Original Video
Mat frame_hsv; //Used for YCrCb color space of original video
Mat edges; //Used for Bilateral Filtering & Skin Color Detection
Mat edges_gray; //Used for Canny Edge Detection testing
Mat labels;
Mat stats;
Mat centroids;
Mat stats_sorted;
Mat dst;
Mat cdst;

vector<Vec2f> lines;

cv::Rect roi;

int jj = 0;
int ii = 0;
int kk = 0;

//Defined function to return RBG pixel values from cursor position
//Used for finding bounds for skin color threshold
static void onMouse(int event, int x, int y, int f, void*) {
	Vec3b pix = frame.at<Vec3b>(y, x); //row,col
	int B_ = pix.val[0];	//Blue Value
	int G_ = pix.val[1];	//Green Value
	int R_ = pix.val[2];	//Red Value
	Vec3b pix_hsv = frame_hsv.at<Vec3b>(y, x); //row, col
	int h_ = pix_hsv.val[0]; //Hue Value
	int s_ = pix_hsv.val[1]; //Saturation Value
	int v_ = pix_hsv.val[2]; //Value
	cout <<"R: " << R_ << " G: " << G_ << " B: " << B_ << endl << "H: " << h_ << " S: " << s_ << " V: " << v_ << endl; //Returns RGB values to command window
}

bool R1(int R, int G, int B) {
	bool e1 = (R > 200) && (R<230) && (G<240) && (G > 200) && (230>B) &&(B > 200);
		//R: 185	G: 150	B:70
		return e1;
}

bool R2(int H, int S, int V) {
	bool e2 = (H > 18) && (H < 30) && (S > 120) && (S < 180) && (V > 150) && (V < 220);
	return e2;
}

Mat GetSkin(Mat const &frame, Mat const &frame_hsv) {
	//allocate the results matrix
	Mat dst = edges.clone();

	Vec3b cwhite = Vec3b::all(255); //White RGB value
	Vec3b cblack = Vec3b::all(0); //Black RGB value

	for (int i = 0; i < edges.rows; i++) {
		for (int j = 0; j < edges.cols; j++) {
			Vec3b pix_bgr = edges.ptr<Vec3b>(i)[j];
			//Vec3b:	Vector with 3 byte entries (R,G,B).
			//ptr<>:	Pretends to be a pointer to an object of type T.
			int B = pix_bgr.val[0];
			int G = pix_bgr.val[1];
			int R = pix_bgr.val[2];

			Vec3b pix_hsv = frame_hsv.ptr<Vec3b>(i)[j];
			int H = pix_hsv.val[0];
			int S = pix_hsv.val[1];
			int V = pix_hsv.val[2];


			//Apply rgb rule
			bool a = R1(R, G, B);
			bool b = R2(H, S, V);

			if (!(a||b))
				edges.ptr<Vec3b>(i)[j] = cblack;
		}
			
	}

	return edges;
}

Mat filterMat(Mat const &frame) {
	//Applying a bilateral filter
	bilateralFilter(frame, edges, KERNEL_LENGTH, KERNEL_LENGTH * 2, KERNEL_LENGTH / 2);
	//bilateralFilter(src,dst,d,sigma_color,sigma_space)
			//src:	Source Image
			//dst:	Destination Image
			//d:	The diameter of each pixel neighborhood
			//sigma_color:	Standard deviation in the color space
			//sigma_color:	Standard deviation in the coordinate space (in pixel terms)

	//Converting image to hsv colorspace for color testing & applying getSkin algorithm
	cvtColor(edges, frame_hsv, CV_BGR2HSV);
	//cvtColor(src,dst,code,dcn)
		//src: Source image.
		//dst: Destination image.

	//Applies GetSkin function to image (threshold elimination)
	Mat edges = GetSkin(edges, frame_hsv);
	//GetSkin(src,src_hsv)
		//src: Source image
		//src_hsv: Source image in HSV colorspace
	
	//Apply Median Blur filter to eliminate noise
	//medianBlur: Smoothes an image using the median filter with the ksize x ksize aperature.
	medianBlur(edges, edges, 3);
	//medianBlur(src,dst,ksize)
		//src: Source image
		//dst: Destination image
		//ksize: Aperture linear size; it must be odd and greater than 1.

	return edges;
}

Mat imageAnalysis(Mat const &edges) {
	//Converting a Matrix into grayscale colorspace
	cvtColor(edges, edges_gray, COLOR_BGR2GRAY);

	//Detecting connected components with stats
	connectedComponentsWithStats(edges_gray, labels, stats, centroids, 8, CV_32S);

	//Sorting the areas (stats.col(4)) of each component detected
	cv::sortIdx(stats.col(4), stats_sorted, CV_SORT_EVERY_COLUMN + CV_SORT_DESCENDING);

	Mat only1;
	Mat only2;
	Mat only3;
	Mat only4;

	//Comparing the original image to the detected components (4 largest components)
	compare(labels, stats_sorted.at<int>(1, 0), only1, CMP_EQ);
	compare(labels, stats_sorted.at<int>(2, 0), only2, CMP_EQ);
	compare(labels, stats_sorted.at<int>(3, 0), only3, CMP_EQ);
	compare(labels, stats_sorted.at<int>(4, 0), only4, CMP_EQ);

	//Combine the four largest components
	Mat dst = only1 + only2 + only3 + only4;

	return dst;
}

bool IsZero (int j) {
	return (j == 0);
};

void SortLines(void) {
	//Sorts the rho values in lines
	sort(lines.begin(), lines.end(), [](const cv::Vec2f &a, const cv::Vec2f &b) {
		return a[0] > b[0];
	});

	int L = lines.size();
	vector<float> rho(L);
	vector<float> theta(L);
	int j = 0;
	float Ttol = 0.2;
	float runAVGrho = 0.0;
	float runAVGtheta = 0.0;

	for (int ii = 0; ii < L; ii++) {
		if (ii == (L - 1)) {
			if (runAVGtheta == 0) {
				theta[j] = lines[ii][1];
				rho[j] = lines[ii][0];
			}
			else {
				runAVGtheta = (runAVGtheta + lines[ii][1]) / 2;
				runAVGrho = (runAVGrho + lines[ii][0]) / 2;
				theta[j] = runAVGtheta;
				rho[j] = runAVGrho;
				j = j + 1;
			}
		}
		else if ((abs(lines[ii][1] - lines[ii + 1][1]) < Ttol)) {
			if (runAVGtheta == 0) {
				runAVGtheta = lines[ii][1];
				runAVGrho = lines[ii][0];
			}
			else {
				runAVGtheta = (runAVGtheta + lines[ii][1]) / 2;
				runAVGrho = (runAVGrho + lines[ii][0]) / 2;
			}
		}
		else {
			if (runAVGtheta != 0.0) {
				theta[j] = runAVGtheta;
				rho[j] = runAVGrho;
			}
			else {
				theta[j] = lines[ii][1];
				rho[j] = lines[ii][0];
			}

			j = j + 1;
			runAVGtheta = 0.0;
			runAVGrho = 0.0;
		}
	}

	for (size_t i = 0; i < lines.size(); i++) {
		float r = rho[i], t = theta[i];
		Point pt1, pt2;
		double a = cos(t), b = sin(t);
		double x0 = a * r, y0 = b * r;
		pt1.x = cvRound(x0 + 1000 * (-b));
		pt1.y = cvRound(y0 + 1000 * (a));
		pt2.x = cvRound(x0 - 1000 * (-b));
		pt2.y = cvRound(y0 - 1000 * (a));
		if ((t > 1.50) && (t < 1.65)) {

		}
		else {
			line(frame(roi), pt1, pt2, Scalar(0, 0, 255), 3, CV_AA);
		}
	}
}

int main(int argc, char* argv)
{
	VideoCapture open(fileName); //Open Video
	if (!open.isOpened()) //Check if we succeeded
		return -1;

	for (;;){
		open >> frame; //get a new frame from the video
		Mat cdst = frame;
		Mat edges = filterMat(frame);
		Mat dst = imageAnalysis(edges);

		roi.x = 0;
		roi.y = frame.size().height / 2;
		roi.width = frame.size().width;
		roi.height = frame.size().height - roi.y;

		dst = dst(roi);

		Canny(dst, dst, 100, 300, 3, false);
		imshow("canny", dst);
		HoughLines(dst, lines, 1, CV_PI/180,80, 0, 0,0,2.5);

		SortLines();

		imshow("Hough", frame);
		
		//imshow("Original", cst); 
		//imshow("HSV Colorspace", frame_hsv);
		//imshow("Bilateral Filter + Skin Color Detection",edges_gray);
		//imshow("Component 2", dst);
		//imshow("Component 2", dst);
		//Shows our gathered frame of video
			//frame:	Original frame from video
			//edges:	Bilateral filter & skin color detection applied
			//edges_gray - Converted grayscale image with Canny Edge Detection

		//setMouseCallback("HSV Colorspace", onMouse, 0);

		string writeFile_hsv = "C:/Users/jaceh/Desktop/Computer Vision Projects/LoadVideo/Results/hsv" + std::to_string(++ii) + ".jpg";
		string writeFile_canny = "C:/Users/jaceh/Desktop/Computer Vision Projects/LoadVideo/Results/canny" + std::to_string(++jj) + ".jpg";
		string writeFile_final = "C:/Users/jaceh/Desktop/Computer Vision Projects/LoadVideo/Results/frame" + std::to_string(++kk) + ".jpg";
		imwrite(writeFile_final, frame);
		imwrite(writeFile_hsv, frame_hsv);
		imwrite(writeFile_canny, dst);
		if (waitKey(1) >= 0) break;
		//waitKey(0);
	}
	return 0;
}