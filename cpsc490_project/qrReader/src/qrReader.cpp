#include "qrReader.h"
#define nan std::numeric_limits<float>::quiet_NaN();

bool qrReader::find(const Mat& img)
{
    possibleCenters.clear();
    estimatedModuleSize.clear();

    int skipRows = 3;

    int stateCount[5] = {0};
    int currentState = 0;
    for(int row=skipRows-1; row<img.rows; row+=skipRows)
    {
        stateCount[0] = 0;
        stateCount[1] = 0;
        stateCount[2] = 0;
        stateCount[3] = 0;
        stateCount[4] = 0;
        currentState = 0;
        const uchar* ptr = img.ptr<uchar>(row);
        for(int col=0; col<img.cols; col++)
        {
            if(ptr[col]<128)
            {
                // We're at a black pixel
                if((currentState & 0x1)==1)
                {
                    // We were counting white pixels
                    // So change the state now

                    // W->B transition
                    currentState++;
                }

                // Works for boths W->B and B->B
                stateCount[currentState]++;
            }
            else
            {
                // We got to a white pixel...
                if((currentState & 0x1)==1)
                {
                    // W->W change
                    stateCount[currentState]++;
                }
                else
                {
                    // ...but, we were counting black pixels
                    if(currentState==4)
                    {
                        // We found the 'white' area AFTER the finder patter
                        // Do processing for it here
                        if(checkRatio(stateCount))
                        {
                            // This is where we do some more checks
                            bool confirmed = handlePossibleCenter(img, stateCount, row, col);
                        }
                        else
                        {
                            currentState = 3;
                            stateCount[0] = stateCount[2];
                            stateCount[1] = stateCount[3];
                            stateCount[2] = stateCount[4];
                            stateCount[3] = 1;
                            stateCount[4] = 0;
                            continue;
                        }
                        currentState = 0;
                        stateCount[0] = 0;
                        stateCount[1] = 0;
                        stateCount[2] = 0;
                        stateCount[3] = 0;
                        stateCount[4] = 0;
                    }
                    else
                    {
                        // We still haven't go 'out' of the finder pattern yet
                        // So increment the state
                        // B->W transition
                        currentState++;
                        stateCount[currentState]++;
                    }
                }
            }
        }
    }
    return (possibleCenters.size()>0);
}

bool qrReader::checkRatio(int stateCount[])
{
    int totalFinderSize = 0;
    for(int i=0; i<5; i++)
    {
        int count = stateCount[i];
        totalFinderSize += count;
        if(count==0)
            return false;
    }

    if(totalFinderSize<7)
        return false;

    // Calculate the size of one module
    int moduleSize = ceil(totalFinderSize / 7.0);
    int maxVariance = moduleSize/2;

    bool retVal= ((abs(moduleSize - (stateCount[0])) < maxVariance) &&
        (abs(moduleSize - (stateCount[1])) < maxVariance) &&
        (abs(3*moduleSize - (stateCount[2])) < 3*maxVariance) &&
        (abs(moduleSize - (stateCount[3])) < maxVariance) &&
        (abs(moduleSize - (stateCount[4])) < maxVariance));

    return retVal;
}

bool qrReader::handlePossibleCenter(const Mat& img, int *stateCount, int row, int col) {
    int stateCountTotal = 0;
    for(int i=0;i<5;i++) {
        stateCountTotal += stateCount[i];
    }

    // Cross check along the vertical axis
    float centerCol = centerFromEnd(stateCount, col);
    float centerRow = crossCheckVertical(img, row, (int)centerCol, stateCount[2], stateCountTotal);
    if(isnan(centerRow)) {
        return false;
    }

    // Cross check along the horizontal axis with the new center-row
    centerCol = crossCheckHorizontal(img, centerRow, centerCol, stateCount[2], stateCountTotal);
    if(isnan(centerCol)) {
        return false;
    }

    // Cross check along the diagonal with the new center row and col
    bool validPattern = crossCheckDiagonal(img, centerRow, centerCol, stateCount[2], stateCountTotal);
    if(!validPattern) {
        return false;
    }

    Point2f ptNew(centerCol, centerRow);
    float newEstimatedModuleSize = stateCountTotal / 7.0f;
    bool found = false;
    int idx = 0;
    // Definitely a finder pattern - but have we seen it before?
    for(Point2f pt : possibleCenters) {
        Point2f diff = pt - ptNew;
        float dist = (float)sqrt(diff.dot(diff));

        // If the distance between two centers is less than 10px, they're the same.
        if(dist < 10) {
            pt = pt + ptNew;
            pt.x /= 2.0f; pt.y /= 2.0f;
            estimatedModuleSize[idx] = (estimatedModuleSize[idx] + newEstimatedModuleSize)/2.0f;
            found = true;
            break;
        }
        idx++;
    }

    if(!found) {
        possibleCenters.push_back(ptNew);
        estimatedModuleSize.push_back(newEstimatedModuleSize);
    }

    return false;
}

float qrReader::crossCheckVertical(const Mat& img, int startRow, int centerCol, int centralCount, int stateCountTotal) {
    int maxRows = img.rows;
    int crossCheckStateCount[5] = {0};
    int row = startRow;
    while(row>=0 && img.at<uchar>(row, centerCol)<128) {
        crossCheckStateCount[2]++;
        row--;
    }
    if(row<0) {
        return nan;
    }

    while(row>=0 && img.at<uchar>(row, centerCol)>=128 && crossCheckStateCount[1]<centralCount) {
        crossCheckStateCount[1]++;
        row--;
    }
    if(row<0 || crossCheckStateCount[1]>=centralCount) {
        return nan;
    }

    while(row>=0 && img.at<uchar>(row, centerCol)<128 && crossCheckStateCount[0]<centralCount) {
        crossCheckStateCount[0]++;
        row--;
    }
    if(row<0 || crossCheckStateCount[0]>=centralCount) {
        return nan;
    }

    // Now we traverse down the center
    row = startRow+1;
    while(row<maxRows && img.at<uchar>(row, centerCol)<128) {
        crossCheckStateCount[2]++;
        row++;
    }
    if(row==maxRows) {
        return nan;
    }

    while(row<maxRows && img.at<uchar>(row, centerCol)>=128 && crossCheckStateCount[3]<centralCount) {
        crossCheckStateCount[3]++;
        row++;
    }
    if(row==maxRows || crossCheckStateCount[3]>=stateCountTotal) {
        return nan;
    }

    while(row<maxRows && img.at<uchar>(row, centerCol)<128 && crossCheckStateCount[4]<centralCount) {
        crossCheckStateCount[4]++;
        row++;
    }
    if(row==maxRows || crossCheckStateCount[4]>=centralCount) {
        return nan;
    }

    int crossCheckStateCountTotal = 0;
    for(int i=0;i<5;i++) {
        crossCheckStateCountTotal += crossCheckStateCount[i];
    }

    if(5*abs(crossCheckStateCountTotal-stateCountTotal) >= 2*stateCountTotal) {
        return nan;
    }

    float center = centerFromEnd(crossCheckStateCount, row);
    return checkRatio(crossCheckStateCount)?center:nan;
}

float qrReader::crossCheckHorizontal(const Mat& img, int centerRow, int startCol, int centerCount, int stateCountTotal) {
    int maxCols = img.cols;
    int stateCount[5] = {0};

    int col = startCol;
    const uchar* ptr = img.ptr<uchar>(centerRow);
    while(col>=0 && ptr[col]<128) {
        stateCount[2]++;
        col--;
    }
    if(col<0) {
        return nan;
    }

    while(col>=0 && ptr[col]>=128 && stateCount[1]<centerCount) {
        stateCount[1]++;
        col--;
    }
    if(col<0 || stateCount[1]==centerCount) {
        return nan;
    }

    while(col>=0 && ptr[col]<128 && stateCount[0]<centerCount) {
        stateCount[0]++;
        col--;
    }
    if(col<0 || stateCount[0]==centerCount) {
        return nan;
    }

    col = startCol + 1;
    while(col<maxCols && ptr[col]<128) {
        stateCount[2]++;
        col++;
    }
    if(col==maxCols) {
        return nan;
    }

    while(col<maxCols && ptr[col]>=128 && stateCount[3]<centerCount) {
        stateCount[3]++;
        col++;
    }
    if(col==maxCols || stateCount[3]==centerCount) {
        return nan;
    }

    while(col<maxCols && ptr[col]<128 && stateCount[4]<centerCount) {
        stateCount[4]++;
        col++;
    }
    if(col==maxCols || stateCount[4]==centerCount) {
        return nan;
    }

    int newStateCountTotal = 0;
    for(int i=0;i<5;i++) {
        newStateCountTotal += stateCount[i];
    }

    if(5*abs(stateCountTotal-newStateCountTotal) >= stateCountTotal) {
        return nan;
    }

    return checkRatio(stateCount)?centerFromEnd(stateCount, col):nan;
}

bool qrReader::crossCheckDiagonal(const Mat &img, float centerRow, float centerCol, int maxCount, int stateCountTotal) {
    int stateCount[5] = {0};

    int i=0;
    while(centerRow>=i && centerCol>=i && img.at<uchar>(centerRow-i, centerCol-i)<128) {
        stateCount[2]++;
        i++;
    }
    if(centerRow<i || centerCol<i) {
        return false;
    }

    while(centerRow>=i && centerCol>=i && img.at<uchar>(centerRow-i, centerCol-i)>=128 && stateCount[1]<=maxCount) {
        stateCount[1]++;
        i++;
    }
    if(centerRow<i || centerCol<i || stateCount[1]>maxCount) {
        return false;
    }

    while(centerRow>=i && centerCol>=i && img.at<uchar>(centerRow-i, centerCol-i)<128 && stateCount[0]<=maxCount) {
        stateCount[0]++;
        i++;
    }
    if(stateCount[0]>maxCount) {
        return false;
    }

        int maxCols = img.cols;
    int maxRows = img.rows;
    i=1;
    while((centerRow+i)<maxRows && (centerCol+i)<maxCols && img.at<uchar>(centerRow+i, centerCol+i)<128) {
        stateCount[2]++;
        i++;
    }
    if((centerRow+i)>=maxRows || (centerCol+i)>=maxCols) {
        return false;
    }

    while((centerRow+i)<maxRows && (centerCol+i)<maxCols && img.at<uchar>(centerRow+i, centerCol+i)>=128 && stateCount[3]<maxCount) {
        stateCount[3]++;
        i++;
    }
    if((centerRow+i)>=maxRows || (centerCol+i)>=maxCols || stateCount[3]>maxCount) {
        return false;
    }

    while((centerRow+i)<maxRows && (centerCol+i)<maxCols && img.at<uchar>(centerRow+i, centerCol+i)<128 && stateCount[4]<maxCount) {
        stateCount[4]++;
        i++;
    }
    if((centerRow+i)>=maxRows || (centerCol+i)>=maxCols || stateCount[4]>maxCount) {
        return false;
    }

    int newStateCountTotal = 0;
    for(int j=0;j<5;j++) {
        newStateCountTotal += stateCount[j];
    }

    return (abs(stateCountTotal - newStateCountTotal) < 2*stateCountTotal) && checkRatio(stateCount);
}

void qrReader::drawFinders(Mat &img) {
    if(possibleCenters.size()==0) {
        return;
    }

    for(int i=0;i<possibleCenters.size();i++) {
        Point2f pt = possibleCenters[i];
        float diff = estimatedModuleSize[i]*3.5f;

        Point2f pt1(pt.x-diff, pt.y-diff);
        Point2f pt2(pt.x+diff, pt.y+diff);
        rectangle(img, pt1, pt2, CV_RGB(255, 0, 0), 4);
    }
}

void qrReader::getTransformedMarker(const Mat &img, Mat& output, int scale) {
    vector<Point2f> src;
    int width = 49 * scale, height = 63 * scale;

    src.push_back(Point2f(3.5f * scale, 3.5f * scale));
    src.push_back(Point2f(width - (3.5f * scale), 3.5f * scale));
    src.push_back(Point2f(width - (3.5f * scale), height - (3.5f * scale)));
    src.push_back(Point2f(3.5f * scale, height - (3.5f * scale)));
    
    Mat transform = getPerspectiveTransform(possibleCenters, src);
    warpPerspective(img, output, transform, Size(width, height), INTER_NEAREST);
}

bool qrReader::checkParityBits(string bits) {
	int length = bits.length();

    int numOne = 0;
    for (int i = 0; i < length; i++) {
        if (bits[i] == '1') {
            numOne++;
        }
    }

    return (numOne % 2 == 0);
}

//  0  1  2 9
//  3  4  5 10
//  6  7  8 11
// 12 13 14
string qrReader::checkParityGrid(string grid1, string grid2) {
	int flipped1[6] = {0};
	int flipped2[6] = {0};
	int nums1[15];
	int nums2[15];

    for (int i = 0; i < 15; i++) {
        nums1[i] = grid1[i] - 48;
        nums2[i] = grid2[i] - 48;
    }

    for (int i = 0; i < 9; i += 3) {
        if ((nums1[i] + nums1[i + 1] + nums1[i + 2] + nums1[9 + (i / 3)]) % 2) {
            flipped1[i / 3] = 1;
        }
        if ((nums2[i] + nums1[i + 1] + nums2[i + 2] + nums2[9 + (i / 3)]) % 2) {
            flipped2[i / 3] = 1;
        }
    }

    for (int i = 0; i < 3; i++) {
        if ((nums1[i] + nums1[i + 3] + nums1[i + 6] + nums1[i + 12]) % 2) {
            flipped1[i + 3] = 1;
        }
        if ((nums2[i] + nums2[i + 3] + nums2[i + 6] + nums2[i + 12]) % 2) {
            flipped2[i + 3] = 1;
        }
    }

    string bits = "";

    for (int i = 0; i < 9; i++) {
    	int row = i / 3;
    	int col = (i % 3) + 3;

    	if (!flipped1[row] && !flipped1[col]) {
    		bits = bits + to_string(nums1[i]);
    	} else if (!flipped2[row] && !flipped2[col]) {
    		bits = bits + to_string(nums2[i]);
    	} else {
    		cout << "Both sets of length bits are corrupted" << endl;
    		exit(1);
    	}
    }

    return bits;
}

int qrReader::extractLength(const Mat &img) {
    string grid1 = "";

    for (int x = 40; x > 37; x--) {
        for (int y = 5; y >= 0; y--) {
            if (img.at<uchar>(y,x) > 128) {
                grid1 = grid1 + "0";
            } else {
                grid1 = grid1 + "1";
            }
        }
    }

    string grid2 = "";

    for (int y = 54; y > 51; y--) {
    	for (int x = 5; x >= 0; x--) {
    		if (img.at<uchar>(y,x) > 128) {
                grid2 = grid2 + "0";
            } else {
                grid2 = grid2 + "1";
            }
    	}
    }

    return stoi(checkParityGrid(grid1, grid2), 0, 2);
}

string qrReader::extractRawData(const Mat &img) {
	int i = 0;
    int round = 0;
    string bits = "";

    int x, y;
    while (round < 3) {
        x = 8;
        y = 5 - round;
        while (x < 38) {
            if (img.at<uchar>(y,x) > 128) {
                bits = bits + "0";
            } else {
                bits = bits + "1";
            }
            i++;
            x++;
        }
        
        x = 43 + round;
        y = 8;
        while (y < 55) {
            if (img.at<uchar>(y,x) > 128) {
                bits = bits + "0";
            } else {
                bits = bits + "1";
            }
            i++;
            y++;
        }

        x = 40;
        y = 57 + round;
        while (x > 7) {
            if (img.at<uchar>(y,x) > 128) {
                bits = bits + "0";
            } else {
                bits = bits + "1";
            }
            i++;
            x--;
        }

        x = 5 - round;
        y = 51;
        while (y > 7) {
            if (img.at<uchar>(y,x) > 128) {
                bits = bits + "0";
            } else {
                bits = bits + "1";
            }
            i++;
            y--;
        }

        round++;
    }

    return bits;
}

string qrReader::convertToDec(string data) {
	string dec = "";

    while (data.length() > 10) {
        int num = stoi(data.substr(0, 10), 0, 2);
        data = data.substr(10, string::npos);
        dec = dec + to_string(num);
    }

    if (data.length() > 0) {
        dec = dec + to_string(stoi(data, 0, 2));
    }

    return dec;
}

string qrReader::getMessage(string data, int length) {
	int numEntries = 462 / length;
	int curNum = 0;
	string entries[numEntries];
	int count[numEntries] = { 0 };

	for (int n = 0; n < numEntries; n++) {
        string curString = data.substr(0, length);
        data = data.substr(length, string::npos);

        if (checkParityBits(curString)) {
	        int i = 0;
	        while (i < curNum) {
	        	if (curString.compare(entries[i]) == 0) {
	        		count[i]++;
	        	}
	        	i++;
	        }


	        if (i == curNum) {
	        	entries[curNum] = curString;
	        	curNum++;
	        }
    	}
    }

    int maxNum = -1;
    int maxEntry = -1;
    for (int i = 0; i < curNum; i++) {
    	if (count[i] > maxNum) {
    		maxNum = count[i];
    		maxEntry = i;
    	}
    }

    return entries[maxEntry].substr(1, string::npos);
}

void qrReader::extract(const Mat& marker) {
    int length = extractLength(marker);
    string data = extractRawData(marker);
    string message = getMessage(data, length);
    cout << convertToDec(message) << endl;
}