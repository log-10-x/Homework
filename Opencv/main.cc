#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    cv::Mat image = cv::imread("image.png", cv::IMREAD_ANYCOLOR);


    if (image.empty()) {
        std::cerr << "Could not open or find the image!" << std::endl;
        return -1;
    }

    cv::namedWindow("Display window", cv::WINDOW_FREERATIO);
    cv::imshow("Display window", image);
    int height = image.rows;
    int width = image.cols;
    std::cout << "Image dimensions: " << width << " x " << height << std::endl;
    int channels = image.channels();
    std::cout << "Number of channels: " << channels << std::endl;

    if (cv::waitKey(0) == 27) { // Wait for ESC key press to exit
        std::cout << "ESC key pressed. Exiting..." << std::endl;
    }

    return 0;
}
