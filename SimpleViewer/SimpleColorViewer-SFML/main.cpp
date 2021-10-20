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
#include <astra_core/astra_core.hpp>
#include <astra/astra.hpp>

#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <key_handler.h>


#include <astra/astra.hpp>
#include <cstdio>
#include <iostream>
#include <key_handler.h>
#include <astra/capi/astra.h>
#include <opencv.hpp>
#include <typeinfo>
#include <vector>
#include "../../../../../../../Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/include/vector"
class SampleFrameListener : public astra::FrameListener   
{//就是读rgb的 按帧来读的
private:
    using buffer_ptr = std::unique_ptr<astra::RgbPixel[]>;
    buffer_ptr buffer_;
    unsigned int lastWidth_;
    unsigned int lastHeight_;

public:
    virtual void on_frame_ready(astra::StreamReader& reader,
        astra::Frame& frame) override
    {
        const astra::ColorFrame colorFrame = frame.get<astra::ColorFrame>();

        if (colorFrame.is_valid())
        {
            print_color(colorFrame);
        }
    }

    void print_color(const astra::ColorFrame& colorFrame)
    {
        if (colorFrame.is_valid())
        {
            int width = colorFrame.width();
            int height = colorFrame.height();
            int frameIndex = colorFrame.frame_index();

            if (width != lastWidth_ || height != lastHeight_) {
                buffer_ = buffer_ptr(new astra::RgbPixel[colorFrame.length()]);
                lastWidth_ = width;
                lastHeight_ = height;
            }
            colorFrame.copy_to(buffer_.get());

            size_t index = ((width * (height / 2.0f)) + (width / 2.0f));
            astra::RgbPixel middle = buffer_[index];
            //for(int i=0;i<buff)
            //std::cout << buffer_[index] << std::endl;
            std::cout << index << std::endl;


            std::cout << "color frameIndex: " << frameIndex
                << " r: " << static_cast<int>(middle.r)
                << " g: " << static_cast<int>(middle.g)
                << " b: " << static_cast<int>(middle.b)
                << std::endl;
        }
    }
    
    //void findStraberry(const astra::ColorFrame& colorFrame)
    //{

    //}
    //void find_strawberry_red(const astra::ColorFrame& colorFrame) {
    //    cv::cvtColor(colorFrame, colorFrame, cv::COLOR_RGB2BGR);
    //}
};




void simple_rgba2rgb(const uint8_t* rgba_img, uint8_t* rgb_img,
    int height, int width)
{
    const int total_pixels = height * width;
    for (int i = 0; i < total_pixels; i++)
    {
        const int src_idx = i * 4;
        const int dst_idx = i * 3;
        *(rgb_img + dst_idx) = *(rgba_img + src_idx);
        *(rgb_img + dst_idx + 1) = *(rgba_img + src_idx + 1);
        *(rgb_img + dst_idx + 2) = *(rgba_img + src_idx + 2);
    }
}


using namespace std;
using namespace cv;
class ColorFrameListener : public astra::FrameListener
{
public:
    ColorFrameListener()
    {
        prev_ = ClockType::now();
        font_.loadFromFile("Inconsolata.otf");
    }

    void init_texture(int width, int height)
    {
        if (displayBuffer_ == nullptr || width != displayWidth_ || height != displayHeight_)
        {
            displayWidth_ = width;
            displayHeight_ = height;

            // texture is RGBA
            int byteLength = displayWidth_ * displayHeight_ * 4;

            displayBuffer_ = BufferPtr(new uint8_t[byteLength]);
            std::memset(displayBuffer_.get(), 0, byteLength);

            texture_.create(displayWidth_, displayHeight_);
            sprite_.setTexture(texture_, true);
            sprite_.setPosition(0, 0);
        }
    }

    void check_fps()//打印东西的方法！！！！！
    {
        const float frameWeight = .2f;

        const ClockType::time_point now = ClockType::now();
        std::cout << now.max<<" "<<now.min << std::endl;//什么玩意？？？？
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

    virtual void on_frame_ready(astra::StreamReader& reader, astra::Frame& frame) override
    {
        const astra::ColorFrame colorFrame = frame.get<astra::ColorFrame>();

        int width = colorFrame.width();
        int height = colorFrame.height();

        init_texture(width, height);

        check_fps();

        if (isPaused_)
        {
            return;
        }

        const astra::RgbPixel* colorData = colorFrame.data();

        for (int i = 0; i < width * height; i++)
        {
            int rgbaOffset = i * 4;
            displayBuffer_[rgbaOffset] = colorData[i].r;
            displayBuffer_[rgbaOffset + 1] = colorData[i].g;
            displayBuffer_[rgbaOffset + 2] = colorData[i].b;
            displayBuffer_[rgbaOffset + 3] = 255;
        }
        texture_.update(displayBuffer_.get());

        //这个是读图像的
        Mat M(height, width, CV_8UC3, Scalar(0, 0, 255));
        int num = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                M.at<Vec3b>(i, j)[0] = colorData[num].b;
                M.at<Vec3b>(i, j)[1] = colorData[num].g;
                M.at<Vec3b>(i, j)[2] = colorData[num].r;
                num++;
            }
        }
        imshow("jjj", M);



        //uint8_t* temp = new uint8_t[width*height];
        //for (int i = 0; i < width * height; i++)
        //{
        //    const int rgbaOffset = i * 3;
        //    temp[rgbaOffset] = colorData[i].r;
        //    temp[rgbaOffset + 1] = colorData[i].b;
        //    temp[rgbaOffset + 2] = colorData[i].g;
        //}
        
        const int byteLength = displayWidth_ * displayHeight_ * 3;
        
    }

    void drawTo(sf::RenderWindow& window)
    {
        if (displayBuffer_ != nullptr)
        {
            float imageScale = window.getView().getSize().x / displayWidth_;
            sprite_.setScale(imageScale, imageScale);
            window.draw(sprite_);
            draw_help_message(window);
        }
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
    //新加的
    void findStraberry(const astra::ColorFrame& colorFrame)
    {

    }
    void find_strawberry_red(const astra::ColorFrame& colorFrame) {
        //cv::cvtColor(colorFrame, colorFrame, cv::COLOR_RGB2BGR);
    }

private:
    using DurationType = std::chrono::milliseconds;
    using ClockType = std::chrono::high_resolution_clock;

    ClockType::time_point prev_;
    float elapsedMillis_{.0f};

    sf::Texture texture_;//图像！！！！！！
    sf::Sprite sprite_;
    sf::Font font_;

    using BufferPtr = std::unique_ptr<uint8_t[]>;
    BufferPtr displayBuffer_{nullptr};
    BufferPtr temp{ nullptr };


    int displayWidth_{0};
    int displayHeight_{0};

    bool isPaused_{false};
    bool isMouseOverlayEnabled_{true};
    bool isFullHelpEnabled_{false};
    const char* helpMessage_{nullptr};
};

int main(int argc, char** argv)
{
    astra::initialize();

    set_key_handler();

    sf::RenderWindow window(sf::VideoMode(1280, 960), "Color Viewer");//打开摄像头的显示

    astra::StreamSet streamSet;
    astra::StreamReader reader = streamSet.create_reader();
    SampleFrameListener listener_Sample;

    reader.stream<astra::ColorStream>().start();

    ColorFrameListener listener;
    //cv::imshow(listener.get_latest_)

    const char* helpMessage =
        "keyboard shortcut:\n"
        "H      show/hide this message\n"
        "P      enable/disable drawing texture\n"
        "SPACE  show/hide all text\n"
        "Esc    exit";
    listener.set_help_message(helpMessage);

    reader.add_listener(listener);
    reader.add_listener(listener_Sample);

    while (window.isOpen())
    {
        astra_update();

        sf::Event event;
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
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                case sf::Keyboard::H:
                    listener.toggle_help();
                    break;
                case sf::Keyboard::P:
                    listener.toggle_paused();
                    break;
                case sf::Keyboard::Space:
                    listener.toggle_overlay();
                    break;
                }
                break;
            }
            default:
                break;
            }
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        listener.drawTo(window);
        window.display();

        if (!shouldContinue)
        {
            window.close();
        }
    }

    astra::terminate();
    return 0;
}

//using namespace cv;
//int main(int argc, char** argv)
//{
//    astra::initialize();
//    astra::StreamSet streamSet;
//    astra::StreamReader reader = streamSet.create_reader();
//    reader.stream<astra::DepthStream>().start();
//    //Stores the maximum number of frames we're going to process in the loop
//    const int maxFramesToProcess = 100;
//    //Sentinel to count the number of frames that we've processed
//    int count = 0;
//    //The frame processing loop
//    do {
//        astra::Frame frame = reader.get_latest_frame();
//        //imshow("woshisb", frame);
//        const auto depthFrame = frame.get<astra::DepthFrame>();
//        const int frameIndex = depthFrame.frame_index();
//        const short pixelValue = depthFrame.data()[0];
//        std::cout << std::endl
//            << "Depth frameIndex: " << frameIndex
//            << " pixelValue: " << pixelValue
//            << std::endl
//            << std::endl;
//        count++;
//    } while (count < maxFramesToProcess);
//    std::cout << "Press any key to continue...";
//    std::cin.get();
//    astra::terminate();
//    std::cout << "hit enter to exit program" << std::endl;
//    std::cin.get();
//    return 0;
//}