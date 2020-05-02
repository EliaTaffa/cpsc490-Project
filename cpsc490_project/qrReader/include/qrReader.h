#pragma once
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;


class qrReader {
public:

	// This section of codes was adapted from https://aishack.in/tutorials/scanning-qr-codes-1/
	// ****************************************************************************************
    bool find(const Mat& img);
    void drawFinders(Mat& img);
   	// ****************************************************************************************

    void getTransformedMarker(const Mat &img, Mat& output, int scale);
    void extract(const Mat &img);

private:

	// This section of codes was adapted from https://aishack.in/tutorials/scanning-qr-codes-1/
	// ****************************************************************************************

    bool checkRatio(int* stateCount);
    bool handlePossibleCenter(const Mat& img, int* stateCount, int row, int col);
    bool crossCheckDiagonal(const Mat &img, float centerRow, float centerCol, int maxCount, int stateCountTotal);
    float crossCheckVertical(const Mat& img, int startRow, int centerCol, int stateCount, int stateCountTotal);
    float crossCheckHorizontal(const Mat& img, int centerRow, int startCol, int stateCount, int stateCountTotal);

    inline float centerFromEnd(int* stateCount, int end) {
        return (float)(end-stateCount[4]-stateCount[3])-(float)stateCount[2]/2.0f;
    }

    vector<Point2f> possibleCenters;
    vector<float> estimatedModuleSize;
	// ****************************************************************************************


    int extractLength(const Mat &img);
    bool checkParityBits(string bits);
    string checkParityGrid(string grid1, string grid2);
    string extractRawData(const Mat &img);
    string convertToDec(string data);
    string getMessage(string data, int length);
};