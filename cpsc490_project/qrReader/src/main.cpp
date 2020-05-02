#include <iostream>
#include <opencv2/opencv.hpp>
#include "qrReader.h"

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
	if(argc == 1) {
        cout << "Usage: " << argv[0] << " <image>" << endl;
        exit(0);
    }

    Mat img = imread(argv[1]);
    Mat imgBW;
    cvtColor(img, imgBW, CV_BGR2GRAY);
    adaptiveThreshold(imgBW, imgBW, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 51, 0);

    qrReader reader = qrReader();
    bool found = reader.find(imgBW);

    if(!found) {
    	cout << "No Barcode Found" << endl;
        exit(1);
    }

    imwrite("./output.png", img);
    imwrite("./outputBW.png", imgBW);
    // Mat newBW = imread("outputBW.png");
    // reader.drawFinders(newBW);
    // imwrite("./outputBW.png", newBW);

    Mat imgWarped;
    Mat imgWarpedBW;
    reader.getTransformedMarker(img, imgWarped, 20);
    reader.getTransformedMarker(imgBW, imgWarpedBW, 1);

    imwrite("./outputWarped.png", imgWarpedBW);
    resize(imgWarped, imgWarped, Size(490, 630), 0, 0, INTER_NEAREST);
    imwrite("./outputResized.png", imgWarped);

    reader.extract(imgWarpedBW);

    waitKey(0);

    return 0;
}