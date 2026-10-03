#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main(){
    Mat img = imread("image.png");
    if(img.empty()){
        cout << "Could not read the image" << endl;
        return 1;
    }
    return 0;
}
