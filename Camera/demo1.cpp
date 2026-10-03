#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main(){
    Mat img = imread("/home/robot/Desktop/Homework/Camera/image.png");
    
    if(img.empty()){
        cout << "Could not read the image" << endl;
        return -1;
    }

    imshow("Image", img);
    waitKey(0); // Wait for a key press before closing the window
    return 0;
}
