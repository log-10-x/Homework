#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main(){
    Mat image = imread("image.jpg");
    if(image.empty()){
        cout << "Could not read the image: image.jpg" << endl;
        return 1;
    }
    return 0;
}
