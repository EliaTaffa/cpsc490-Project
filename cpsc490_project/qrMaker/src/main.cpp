#include <iostream>
#include <opencv2/opencv.hpp>
#include "qrMaker.h"

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
    if(argc != 2) {
        cout << "Usage: " << argv[0] << "\"Text Here\"" << endl;
        exit(0);
    }

    Mat img(65, 51, CV_8UC3, CV_RGB(127, 127, 127));

    qrMaker maker = qrMaker();
    maker.drawFinders(img);
    string bits = maker.getBits(argv[1]);
    maker.drawBits(img, bits);

    resize(img, img, Size(510, 650), 0, 0, INTER_NEAREST);
    imwrite("./output.png", img);
    waitKey(0);

    return 0;
}