#pragma once
#include <opencv2/opencv.hpp>
#include <iostream>
#include <set>
#include <string>
#include <bitset>

using namespace std;
using namespace cv;

class qrMaker {
public:
    void drawFinders(Mat &img);
    string getBits(string s);
    void drawBits(Mat &img, string bits);

private:
    void drawFinder(Mat &img, int x, int y);
    void drawTimingPattern(Mat &img);

    void drawLengthBits(Mat &img, string bits);
    void drawData(Mat &img, string bits);
    string addParityBit(string bits);
    string addParityGrid(string bits);

    int getDataType(string s);
    string decToBinary(int n, int ccLength);
    string numericToData(string s);
};