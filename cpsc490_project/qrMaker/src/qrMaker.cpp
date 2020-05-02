#include "qrMaker.h"

set<char> const SPECIAL_ALPHA_NUM = {' ', '$', '%', '*', '+', '-', '.', '/', ':'};
int width = 51;
int height = 65;
Vec3b black = Vec3b(0, 0, 0);
Vec3b white = Vec3b(255, 255, 255);

void qrMaker::drawFinder(Mat &img, int x, int y) {
    rectangle(
        img,
        Point(x - 1, y - 1),
        Point(x + 7, y + 7),
        CV_RGB(255, 255, 255),
        -1);
    rectangle(
        img,
        Point(x, y),
        Point(x + 6, y + 6),
        CV_RGB(0, 0, 0),
        1);
    rectangle(
        img,
        Point(x + 2, y + 2),
        Point(x + 4, y + 4),
        CV_RGB(0, 0, 0),
        -1);
}

void qrMaker::drawTimingPattern(Mat &img) {
    int x = 7;
    int y = 9;
    int z = width - 8;

    while (y < height - 7) {
        if (y % 2 == 0) {
            img.at<Vec3b>(y, x) = white;
            img.at<Vec3b>(y, z) = white;
        } else {
            img.at<Vec3b>(y, x) = black;
            img.at<Vec3b>(y, z) = black;
        }
        y++;
    }


    x = 9;
    y = 7;
    z = height - 8;

    while (x < width - 7) {
        if (x % 2 == 0) {
            img.at<Vec3b>(y, x) = white;
            img.at<Vec3b>(z, x) = white;
        } else {
            img.at<Vec3b>(y, x) = black;
            img.at<Vec3b>(z, x) = black;
        }
        x++;
    }
}

void qrMaker::drawFinders(Mat &img) {
    drawFinder(img, 1, 1);
    drawFinder(img, width - 8, 1);
    drawFinder(img, 1, height - 8);
    drawFinder(img, width - 8, height - 8);
    drawTimingPattern(img);
}

// **********************************************************************
void qrMaker::drawLengthBits(Mat &img, string bits) {
    int i = 0;

    int y = 55;
    for (int x = 41; x > 38; x--) {
        for (int n = 6; n > 0; n--) {
            if (bits[i] == '1') {
                img.at<Vec3b>(n, x) = black;
                img.at<Vec3b>(y, n) = black;
            } else {
                img.at<Vec3b>(n, x) = white;
                img.at<Vec3b>(y, n) = white;
            }
            i++;
        }
        y--;
    }

}

void qrMaker::drawData(Mat &img, string bits) {
    int i = 0;
    int round = 0;
    int numBits = bits.length();

    int x, y;
    while (round < 3) {
        x = 9;
        y = 6 - round;
        while (i < numBits && x < 39) {
            if (bits[i] == '1') {
                img.at<Vec3b>(y, x) = black;
            } else {
                img.at<Vec3b>(y, x) = white;
            }
            i = (i + 1) % numBits;
            x++;
        }
        
        x = 44 + round;
        y = 9;
        while (i < numBits && y < 56) {
            if (bits[i] == '1') {
                img.at<Vec3b>(y, x) = black;
            } else {
                img.at<Vec3b>(y, x) = white;
            }
            i = (i + 1) % numBits;
            y++;
        }

        x = 41;
        y = 58 + round;
        while (i < numBits && x > 8) {
            if (bits[i] == '1') {
                img.at<Vec3b>(y, x) = black;
            } else {
                img.at<Vec3b>(y, x) = white;
            }
            i = (i + 1) % numBits;
            x--;
        }

        x = 6 - round;
        y = 52;
        while (i < numBits && y > 8) {
            if (bits[i] == '1') {
                img.at<Vec3b>(y, x) = black;
            } else {
                img.at<Vec3b>(y, x) = white;
            }
            i = (i + 1) % numBits;
            y--;
        }

        round++;
    }
}

string qrMaker::addParityBit(string bits) {
    int length = bits.length();

    int numOne = 0;
    for (int i = 0; i < length; i++) {
        if (bits[i] == '1') {
            numOne++;
        }
    }

    if (numOne % 2 == 0) {
        return '0' + bits;
    } else {
        return '1' + bits;
    }
}

string qrMaker::addParityGrid(string bits) {
    int grid[9];

    for (int i = 0; i < 9; i++) {
        grid[i] = bits[i] - 48;
    }

    for (int i = 0; i < 9; i += 3) {
        if ((grid[i] + grid[i + 1] + grid[i + 2]) % 2) {
            bits = bits + "1";
        } else {
            bits = bits + "0";
        }
    }

    for (int i = 0; i < 3; i++) {
        if ((grid[i] + grid[i + 3] + grid[i + 6]) % 2) {
            bits = bits + "1";
        } else {
            bits = bits + "0";
        }
    }

    bits = bits + "000";

    return bits;
}

void qrMaker::drawBits(Mat &img, string bits) {
    bits = addParityBit(bits);
    int length = bits.length();
    string lengthBits = addParityGrid(decToBinary(length, 9));

    drawLengthBits(img, lengthBits);
    drawData(img, bits);
}


// **********************************************************************

bool isNumeric(string s)
{
    int i = 0;
    while (s[i] != '\0') {
        if (!isdigit(s[i])) {
            return false;
        }
        i++;
    }
    return true;
}

bool isAlphaNumeric(string s) {
    int i = 0;
    while (s[i] != '\0') {
        if ((!isalnum(s[i])) && (!SPECIAL_ALPHA_NUM.count(s[i]))) {
            printf("%c\n", s[i]);
            return false;
        }
        i++;
    }
    return true;
}

int qrMaker::getDataType(string s)
{
    if (isNumeric(s)) {
        return 1;
    } else if (isAlphaNumeric(s)) {
        return 2;
    } else {
        return -1;
    }
}

string qrMaker::numericToData(string s) {
    string data = "";

    while (s.length() > 3) {
        int num = stoi(s.substr(0, 3));
        s = s.substr(3, string::npos);
        data = data + decToBinary(num, 10);
    }

    if (s.length() > 0) {
        data = data + decToBinary(stoi(s), 10);
    }

    return data;
}

string qrMaker::decToBinary(int n, int ccLength) 
{ 
    string binary = ""; 
    while (ccLength > 0) {
        binary = to_string(n % 2) + binary;
        n = n / 2;
        ccLength--;
    } 
   
    return binary;
}

string qrMaker::getBits(string s) {
    int dataType = getDataType(s);
    string data = "";
    if (dataType == 1) {
        data = numericToData(s);
    }

    return data;
}


