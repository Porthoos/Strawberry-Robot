#include <SFML/Graphics.hpp>
#include <astra/astra.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <opencv.hpp>
#include <vector>
#include <cstring>
#include<fstream>
#include<iostream>
#include<ctime>
float X = 0;
float Y = 0;
class DepthFrameListener : public astra::FrameListener
{
public:
    DepthFrameListener()
    {
        prev_ = ClockType::now();
        font_.loadFromFile("Inconsolata.otf");
    }

    void init_texture(int width, int height)
    {
        if (!displayBuffer_ ||
            width != displayWidth_ ||
            height != displayHeight_)
        {
            displayWidth_ = width;
            displayHeight_ = height;

            // texture is RGBA
            const int byteLength = displayWidth_ * displayHeight_ * 4;

            displayBuffer_ = BufferPtr(new uint8_t[byteLength]);
            std::fill(&displayBuffer_[0], &displayBuffer_[0] + byteLength, 0);

            texture_.create(displayWidth_, displayHeight_);
            sprite_.setTexture(texture_, true);
            sprite_.setPosition(0, 0);
        }
    }

    void check_fps()
    {
        const float frameWeight = .2f;

        const ClockType::time_point now = ClockType::now();
        const float elapsedMillis = std::chrono::duration_cast<DurationType>(now - prev_).count();

        elapsedMillis_ = elapsedMillis * frameWeight + elapsedMillis_ * (1.f - frameWeight);
        prev_ = now;

        const float fps = 1000.f / elapsedMillis;

        const auto precision = std::cout.precision();

        std::cout << std::fixed
            << std::setprecision(1)
            << fps << " fps ("
            << std::setprecision(1)
            << elapsedMillis_ << " ms)"
            << std::setprecision(precision)
            << std::endl;
    }

    void on_frame_ready(astra::StreamReader& reader,
        astra::Frame& frame) override
    {
        cv::RotatedRect ellipsemege;
        bool judge = false;
        try {
            const astra::ColorFrame colorFrame = frame.get<astra::ColorFrame>();//加的
            int width1 = colorFrame.width();
            int height1 = colorFrame.height();
            init_texture(width1, height1);//width height有关

            check_fps();
            if (isPaused_) { return; }
            const astra::RgbPixel* colorData = colorFrame.data();
            cv::Mat M(height1, width1, CV_8UC3, cv::Scalar(0, 0, 255));
            int num = 0;
            for (int i = 0; i < height1; i++) {
                for (int j = 0; j < width1; j++) {
                    M.at<cv::Vec3b>(i, j)[0] = colorData[num].b;
                    M.at<cv::Vec3b>(i, j)[1] = colorData[num].g;
                    M.at<cv::Vec3b>(i, j)[2] = colorData[num].r;
                    num++;
                }
            }
            try {

                ellipsemege = find_strawberry_red(M, M.rows, M.cols);
                if (ellipsemege.center.x == -10000) {
                    //std::cout << "找到个假的" << std::endl;
                    return;
                }
                //std::cout << "找到了:" << ellipsemege.center.x << " and " << ellipsemege.center.y << std::endl;
                judge = true;
                //
            }
            catch (const char*& e) {
            }
        }
        catch (const char*& e) {
            std::cout << "这里报错了" << std::endl;
        }
        if (judge) {
            std::cout << "真的找到了:" << ellipsemege.center.x << " and " << ellipsemege.center.y << std::endl;

            X = ellipsemege.center.x;
            Y = ellipsemege.center.y;
        }



        const astra::PointFrame pointFrame = frame.get<astra::PointFrame>();
        const int width = pointFrame.width();
        const int height = pointFrame.height();

        init_texture(width, height);//width height有关

        check_fps();

        if (isPaused_) { return; }

        copy_depth_data(frame);


    }

    std::vector<std::vector<cv::Point>> find_biggest_contour(cv::Mat M) {
        //std::cout << "222222222222" << std::endl;
        cv::Mat image = M.clone();
        //Mat contours;
        //Mat hierarchy;
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        findContours(image, contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

        double maxarea = 0;
        int maxAreaIdx = 0;
        for (int index = contours.size() - 1; index >= 0; index--)
        {
            double tmparea = fabs(contourArea(contours[index]));
            if (tmparea > maxarea)
            {
                maxarea = tmparea;
                maxAreaIdx = index;//记录最大轮廓的索引号
            }
        }
        std::vector<std::vector<cv::Point>> contourlist;
        //std::cout << "33333333333" << std::endl;
        if (contours.size() == 0) {
            return contourlist;
        }
        contourlist.push_back(contours[maxAreaIdx]);
        //std::cout << "333333333334" << std::endl;

        return contourlist;
    }

    cv::Mat overlay_mask(cv::Mat mask, cv::Mat image) {
        cv::Mat rgb_mask;
        cvtColor(mask, rgb_mask, cv::COLOR_GRAY2RGB);
        cv::Mat result;
        addWeighted(rgb_mask, 0.5, image, 0.5, 0, result);
        return result;

    }

    cv::RotatedRect find_strawberry_red(cv::Mat M, int height, int width)
    {
        //std::cout << "1111111" << std::endl;
        int max_dimension = std::max(height, width);
        int scale = 700 / max_dimension;
        cv::Size dsize = cv::Size(M.cols * scale, M.rows * scale);
        cv::Mat image = cv::Mat(dsize, CV_32S);
        resize(M, image, dsize);
        cv::Mat image_blur;
        cv::GaussianBlur(image, image_blur, cv::Size(7, 7), 0);
        cv::Mat image_blur_hsv;
        cvtColor(image_blur, image_blur_hsv, cv::COLOR_BGR2HSV, 0);
        cv::InputArray min_red = (0, 43, 46);
        cv::InputArray max_red = (10, 255, 255);
        cv::Mat mask1;
        inRange(image_blur_hsv, cv::Scalar(0, 100, 80), cv::Scalar(10, 256, 256), mask1);
        cv::InputArray min_red2 = (156, 43, 46);
        cv::InputArray max_red2 = (180, 255, 255);
        cv::Mat mask2;
        inRange(image_blur_hsv, min_red2, max_red2, mask2);
        cv::Mat mask = mask1 + mask2;
        cv::Size ksize = cv::Size(15, 15);
        cv::Mat kernel = getStructuringElement(cv::MORPH_ELLIPSE, ksize);
        cv::Mat mask_closed;
        morphologyEx(mask, mask_closed, cv::MORPH_CLOSE, kernel);
        cv::Mat mask_clean;
        morphologyEx(mask_closed, mask_clean, cv::MORPH_OPEN, kernel);
        //try {
        std::vector<std::vector<cv::Point>> biggest_contour;
        biggest_contour = find_biggest_contour(mask_clean);
        if (biggest_contour.size() == 0) {
            //std::cout << "没找到" << std::endl;
            cv::RotatedRect ellipsemege1;
            ellipsemege1.center.x = -10000;
            return ellipsemege1;
        }
        else {
            cv::RNG& rng = cv::theRNG();
            cv::RotatedRect ellipsemege = cv::fitEllipse(biggest_contour[0]);
            cv::Mat M_copy = M.clone();
            ellipse(M_copy, ellipsemege, cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255)), 3);
            //cv::imshow("oo", M_copy);
            cv::waitKey(0);
            //std::cout << "坐标IN找草莓: " << ellipsemege.center.x << std::endl;
            cv::Mat result;
            cvtColor(M_copy, result, cv::COLOR_RGB2BGR);
            return ellipsemege;
        }

    }

    void copy_depth_data(astra::Frame& frame)
    {
        const astra::DepthFrame depthFrame = frame.get<astra::DepthFrame>();

        if (depthFrame.is_valid())
        {
            const int width = depthFrame.width();
            const int height = depthFrame.height();
            if (!depthData_ || width != depthWidth_ || height != depthHeight_)
            {
                depthWidth_ = width;
                depthHeight_ = height;

                // texture is RGBA
                const int byteLength = depthWidth_ * depthHeight_ * sizeof(uint16_t);

                depthData_ = DepthPtr(new int16_t[byteLength]);
            }

            depthFrame.copy_to(&depthData_[0]);
        }
    }


    void my_position2(
        const astra::CoordinateMapper& coordinateMapper)
    {

        std::cout << "start program" << std::endl;

        const size_t index = (depthWidth_ * Y + X);//根据x和y来计算z。
        const short z = depthData_[index];
        std::cout << "z: " << z << std::endl;//z是什么呀???

        coordinateMapper.convert_depth_to_world(float(X),
            float(Y),
            float(z),
            mouseWorldX_,
            mouseWorldY_,
            mouseWorldZ_);
        //将深度坐标转换为世界坐标 单位？ 哪是0？？？
        std::cout << "真实世界坐标： " << mouseWorldX_ << " " << mouseWorldY_ << " " << mouseWorldZ_ << std::endl;
        std::cout << X << " " << Y << " " << z << std::endl;

        std::ofstream ofs;
        ofs.open("C:/Users/Lenovo/Desktop/test.txt", std::ios::out);
        ofs << "catch " << mouseWorldX_ << " " << mouseWorldY_ << " " << mouseWorldZ_ << std::endl;
    }




    void toggle_paused()
    {
        isPaused_ = !isPaused_;
    }

    bool is_paused() const
    {
        return isPaused_;
    }

    void toggle_overlay()
    {
        isMouseOverlayEnabled_ = !isMouseOverlayEnabled_;
    }

    bool overlay_enabled() const
    {
        return isMouseOverlayEnabled_;
    }

    void toggle_help()
    {
        isFullHelpEnabled_ = !isFullHelpEnabled_;
    }

    void set_help_message(const char* msg)
    {
        helpMessage_ = msg;
    }

private:

    using DurationType = std::chrono::milliseconds;
    using ClockType = std::chrono::high_resolution_clock;

    ClockType::time_point prev_;
    float elapsedMillis_{ .0f };

    sf::Texture texture_;
    sf::Sprite sprite_;
    sf::Font font_;

    int displayWidth_{ 0 };
    int displayHeight_{ 0 };

    using BufferPtr = std::unique_ptr<uint8_t[]>;
    BufferPtr displayBuffer_{ nullptr };

    int depthWidth_{ 0 };
    int depthHeight_{ 0 };

    using DepthPtr = std::unique_ptr<int16_t[]>;
    DepthPtr depthData_{ nullptr };

    int mouseX_{ 0 };
    int mouseY_{ 0 };
    float mouseWorldX_{ 0 };
    float mouseWorldY_{ 0 };
    float mouseWorldZ_{ 0 };
    bool isPaused_{ false };
    bool isMouseOverlayEnabled_{ true };
    bool isFullHelpEnabled_{ false };
    const char* helpMessage_{ nullptr };
};

astra::DepthStream configure_depth(astra::StreamReader& reader)
{
    auto depthStream = reader.stream<astra::DepthStream>();

    auto oldMode = depthStream.mode();

    //We don't have to set the mode to start the stream, but if you want to here is how:
    astra::ImageStreamMode depthMode;

    depthMode.set_width(640);
    depthMode.set_height(480);
    depthMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_DEPTH_MM);
    depthMode.set_fps(30);

    depthStream.set_mode(depthMode);

    auto newMode = depthStream.mode();
    printf("Changed depth mode: %dx%d @ %d -> %dx%d @ %d\n",
        oldMode.width(), oldMode.height(), oldMode.fps(),
        newMode.width(), newMode.height(), newMode.fps());

    return depthStream;
}

std::string getTimeString()
{
    time_t time = std::time(0);
    char buf[50];
    std::strftime(buf, 50, "%H-%M-%S", std::localtime(&time));
    return std::string(buf) + ".oni";
}

int main(int argc, char** argv)
{
    astra::initialize();

    //set_key_handler();

    //sf::RenderWindow window(sf::VideoMode(1280, 960), "Depth Viewer");

#ifdef _WIN32
    auto fullscreenStyle = sf::Style::None;
#else
    auto fullscreenStyle = sf::Style::Fullscreen;
#endif

    const sf::VideoMode fullScreenMode = sf::VideoMode::getFullscreenModes()[0];
    const sf::VideoMode windowedMode(1280, 1024);
    bool isFullScreen = false;

    astra::StreamSet streamSet;
    astra::StreamReader reader = streamSet.create_reader();
    reader.stream<astra::ColorStream>().start();

    reader.stream<astra::PointStream>().start();

    auto depthStream = configure_depth(reader);
    depthStream.start();
    bool isRecording = false;

    DepthFrameListener listener;

    reader.add_listener(listener);

    std::cout << "X: " << X << " Y: " << Y << std::endl;
    while (1) {
        astra_update();
        sf::Event event;
        if (X != 0 && Y != 0) {
            std::cout << "X: " << int(X) << " Y: " << int(Y) << std::endl;
            auto coordinateMapper = depthStream.coordinateMapper();
            std::cout << "start" << std::endl;
            listener.my_position2(coordinateMapper);

            clock_t start;
            start = clock();
            while (clock() - start < 5000) {}
            std::cout << "end program" << std::endl;
        }
    }
}
  
