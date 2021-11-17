// This file is part of the Orbbec Astra SDK [https://orbbec3d.com]
// Copyright (c) 2015-2017 Orbbec 3D
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Be excellent to each other.
#include <SFML/Graphics.hpp>
#include <astra/astra.hpp>
#include "LitDepthVisualizer.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <key_handler.h>
#include <sstream>
#include <opencv.hpp>
#include <core/mat.hpp>
#include <vector>
#include <cstring>
#include<fstream>
#include<iostream>
#include<ctime>
//using namespace std;
//using namespace cv;
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
            cv::imshow("jjj", M);
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
        //const astra::ColorFrame colorFrame = frame.get<astra::ColorFrame>();//加的
        //std::cout<<colorFrame.data()<<std::endl;




        const astra::PointFrame pointFrame = frame.get<astra::PointFrame>();
        const int width = pointFrame.width();
        const int height = pointFrame.height();
        //std::cout << "woshisbbbbbbbbbbbbbbbbbbbbbb" << std::endl;

        init_texture(width, height);//width height有关

        check_fps();

        if (isPaused_) { return; }

        copy_depth_data(frame);

        visualizer_.update(pointFrame);

        const astra::RgbPixel* vizBuffer = visualizer_.get_output();
        for (int i = 0; i < width * height; i++)
        {
            const int rgbaOffset = i * 4;
            displayBuffer_[rgbaOffset] = vizBuffer[i].r;
            displayBuffer_[rgbaOffset + 1] = vizBuffer[i].b;
            displayBuffer_[rgbaOffset + 2] = vizBuffer[i].g;
            displayBuffer_[rgbaOffset + 3] = 255;
        }




        texture_.update(displayBuffer_.get());
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
        //RNG& rng = theRNG();
        //RotatedRect ellipsemege = fitEllipse(contours[maxAreaIdx]);
        //Mat M_copy = M.clone();
        //ellipse(M_copy, ellipsemege, Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255)), 3);
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
        cv::InputArray min_red = (0, 43, 46);//不知道这样行不行？？？
        cv::InputArray max_red = (10, 255, 255);//不知道这样行不行？？？
        cv::Mat mask1;
        inRange(image_blur_hsv, cv::Scalar(0, 100, 80), cv::Scalar(10, 256, 256), mask1);
        cv::InputArray min_red2 = (156, 43, 46);//不知道这样行不行？？？
        cv::InputArray max_red2 = (180, 255, 255);//不知道这样行不行？？？
        cv::Mat mask2;
        inRange(image_blur_hsv, min_red2, max_red2, mask2);
        cv::Mat mask = mask1 + mask2;//mask是什么格式的？？？？？
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
            cv::imshow("oo", M_copy);
            cv::waitKey(0);
            //std::cout << "坐标IN找草莓: " << ellipsemege.center.x << std::endl;
            cv::Mat result;
            cvtColor(M_copy, result, cv::COLOR_RGB2BGR);
            return ellipsemege;
        }
        // }
         //catch (const char*& e) {

         //}

         //return ellipsemege;

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

    void my_position(const astra::CoordinateMapper& coordinateMapper)
    {
        const size_t index = (depthWidth_ * int(Y) + int(X));
        const short z = depthData_[index];
        std::cout << "z: " << z << std::endl;
        coordinateMapper.convert_depth_to_world(float(X),
            float(Y),
            float(z),
            mouseWorldX_,
            mouseWorldY_,
            mouseWorldZ_);
        std::cout << mouseWorldX_ << ", " << mouseWorldX_ << ", " << mouseWorldZ_ << std::endl;
    }

    void my_position1(sf::RenderWindow& window,
        const astra::CoordinateMapper& coordinateMapper)
    {
        const sf::Vector2i position = sf::Mouse::getPosition(window);
        const sf::Vector2u windowSize = window.getSize();

        std::cout << "start program" << std::endl;


        //float mouseNormX = position.x / float(windowSize.x);
        //float mouseNormY = position.y / float(windowSize.y);

        //mouseX_ = depthWidth_ * mouseNormX;
        //mouseY_ = depthHeight_ * mouseNormY;

        //if (mouseX_ >= depthWidth_ ||
        //    mouseY_ >= depthHeight_ ||
        //    mouseX_ < 0 ||
        //    mouseY_ < 0) {
        //    return;
        //}

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

    void update_mouse_position(sf::RenderWindow& window,
        const astra::CoordinateMapper& coordinateMapper)
    {
        const sf::Vector2i position = sf::Mouse::getPosition(window);
        const sf::Vector2u windowSize = window.getSize();




        float mouseNormX = position.x / float(windowSize.x);
        float mouseNormY = position.y / float(windowSize.y);

        mouseX_ = depthWidth_ * mouseNormX;
        mouseY_ = depthHeight_ * mouseNormY;

        if (mouseX_ >= depthWidth_ ||
            mouseY_ >= depthHeight_ ||
            mouseX_ < 0 ||
            mouseY_ < 0) {
            return;
        }

        const size_t index = (depthWidth_ * mouseY_ + mouseX_);//根据x和y来计算z。
        const short z = depthData_[index];
        std::cout << "z: " << z << std::endl;//z是什么呀???

        coordinateMapper.convert_depth_to_world(float(mouseX_),
            float(mouseY_),
            float(z),
            mouseWorldX_,
            mouseWorldY_,
            mouseWorldZ_);
        //将深度坐标转换为世界坐标 单位？ 哪是0？？？
    }

    void draw_text(sf::RenderWindow& window,
        sf::Text& text,
        sf::Color color,
        const int x,
        const int y) const
    {
        text.setColor(sf::Color::Black);
        text.setPosition(x + 5, y + 5);
        window.draw(text);

        text.setColor(color);
        text.setPosition(x, y);
        window.draw(text);
    }

    void draw_mouse_overlay(sf::RenderWindow& window,
        const float depthWScale,
        const float depthHScale) const
    {
        if (!isMouseOverlayEnabled_ || !depthData_) { return; }

        std::stringstream str;
        str << std::fixed
            << std::setprecision(0)
            << "(" << mouseX_ << ", " << mouseY_ << ") "
            << "X: " << mouseWorldX_ << " Y: " << mouseWorldY_ << " Z: " << mouseWorldZ_;

        const int characterSize = 40;
        sf::Text text(str.str(), font_);
        text.setCharacterSize(characterSize);
        text.setStyle(sf::Text::Bold);

        const float displayX = 10.f;
        const float margin = 10.f;
        const float displayY = window.getView().getSize().y - (margin + characterSize);

        draw_text(window, text, sf::Color::White, displayX, displayY);
    }

    void draw_help_message(sf::RenderWindow& window) const
    {
        if (!isMouseOverlayEnabled_) {
            return;
        }

        std::stringstream str;
        str << "press h to toggle help message";

        if (isFullHelpEnabled_ && helpMessage_ != nullptr)
        {
            str << "\n" << helpMessage_;
        }

        const int characterSize = 30;
        sf::Text text(str.str(), font_);
        text.setCharacterSize(characterSize);
        text.setStyle(sf::Text::Bold);

        const float displayX = 0.f;
        const float displayY = 0;

        draw_text(window, text, sf::Color::White, displayX, displayY);
    }

    void draw_to(sf::RenderWindow& window)//核心？？？
    {
        if (displayBuffer_ != nullptr)
        {
            const float depthWScale = window.getView().getSize().x / displayWidth_;//都是2的缩放比？
            const float depthHScale = window.getView().getSize().y / displayHeight_;
            std::cout << "depthWScale:" << depthWScale << " depthHScale: " << depthHScale << std::endl;
            sprite_.setScale(depthWScale, depthHScale);
            window.draw(sprite_);

            draw_mouse_overlay(window, depthWScale, depthHScale);
            draw_help_message(window);
        }
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
    samples::common::LitDepthVisualizer visualizer_;

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

    set_key_handler();

    sf::RenderWindow window(sf::VideoMode(1280, 960), "Depth Viewer");

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

    const char* helpMessage =
        "keyboard shortcut:\n"
        "D      use 640x400 depth resolution\n"
        "F      toggle between fullscreen and windowed mode\n"
        "H      show/hide this message\n"
        "M      enable/disable depth mirroring\n"
        "P      enable/disable drawing texture\n"
        "R      enable/disable depth registration\n"
        "S      start/stop depth record\n"
        "SPACE  show/hide all text\n"
        "Esc    exit";
    listener.set_help_message(helpMessage);

    reader.add_listener(listener);

    std::cout << "X: " << X << " Y: " << Y << std::endl;
    //auto coordinateMapper = depthStream.coordinateMapper();
    //listener.my_position(coordinateMapper);
    //listener.update_mouse_position(window, coordinateMapper);

    while (window.isOpen())
    {
        astra_update();
        sf::Event event;
        if (X!=0 && Y!= 0) {
            std::cout << "X: " << int(X) << " Y: " << int(Y) << std::endl;
            auto coordinateMapper = depthStream.coordinateMapper();
            std::cout << "start" << std::endl;
            listener.my_position1(window, coordinateMapper);

            clock_t start;
            start = clock();
            while (clock() - start < 5000) {}
            std::cout << "end program" << std::endl;
        }

        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
            {
                if (event.key.code == sf::Keyboard::C && event.key.control)
                {
                    window.close();
                }
                switch (event.key.code)
                {
                case sf::Keyboard::D:
                {
                    auto oldMode = depthStream.mode();
                    astra::ImageStreamMode depthMode;

                    depthMode.set_width(640);
                    depthMode.set_height(400);
                    depthMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_DEPTH_MM);
                    depthMode.set_fps(30);

                    depthStream.set_mode(depthMode);
                    auto newMode = depthStream.mode();
                    printf("Changed depth mode: %dx%d @ %d -> %dx%d @ %d\n",
                        oldMode.width(), oldMode.height(), oldMode.fps(),
                        newMode.width(), newMode.height(), newMode.fps());
                    break;
                }
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                case sf::Keyboard::F:
                    if (isFullScreen)
                    {
                        window.create(windowedMode, "Depth Viewer", sf::Style::Default);
                    }
                    else
                    {
                        window.create(fullScreenMode, "Depth Viewer", fullscreenStyle);
                    }
                    isFullScreen = !isFullScreen;
                    break;
                case sf::Keyboard::H:
                    listener.toggle_help();
                    break;
                case sf::Keyboard::R:
                    depthStream.enable_registration(!depthStream.registration_enabled());
                    break;
                case sf::Keyboard::M:
                    depthStream.enable_mirroring(!depthStream.mirroring_enabled());
                    break;
                case sf::Keyboard::P:
                    listener.toggle_paused();
                    break;
                case sf::Keyboard::S:
                    if (isRecording)
                    {
                        depthStream.stop_record();
                        isRecording = false;
                    }
                    else
                    {
                        std::string filename = getTimeString();
                        printf("record depth to oni file %s", filename.c_str());
                        depthStream.start_record(filename);
                        isRecording = true;
                    }
                    break;
                case sf::Keyboard::Space:
                    listener.toggle_overlay();
                    break;
                default:
                    break;
                }
                break;
            }
            case sf::Event::MouseMoved:
            {
                //std::cout << "X: " << int(X) << " Y: " << int(Y) << std::endl;
                //auto coordinateMapper = depthStream.coordinateMapper();
                //listener.my_position1(window,coordinateMapper);
                // 
                //listener.my_position(coordinateMapper);
                //listener.update_mouse_position(window, coordinateMapper);
                break;
            }
            default:
                break;
            }
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        listener.draw_to(window);//！！！！！！！！！！！
        window.display();

        if (!shouldContinue)
        {
            window.close();
        }
    }

    astra::terminate();

    return 0;
}
