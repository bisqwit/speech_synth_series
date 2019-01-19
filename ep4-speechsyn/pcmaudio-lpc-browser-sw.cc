#include <cmath>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <map>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics.hpp>

/*
g++ pcmaudio-lpc-browser-sw.cc -std=c++17 \
    $(pkg-config sfml-{audio,graphics} --libs --cflags) \
    imgui/imgui{,_draw,_widgets}.cpp imgui_software_renderer/src/imgui_sw.cpp \
    -I. -Iimgui -O3 -Wall -Wextra -pedantic -g
*/

namespace
{
    struct frame
    {
        std::vector<double> coefficients;
        double gain;
    };
    std::vector<frame> frames;
    int                frame_index = 10;
    float              volume      = 0.2;
    int                voicepitch  = 105;
    float              breath      = 0.5;
    float              buzz        = 1.0;
}

struct MyAudioDriver: public sf::SoundStream
{
    static constexpr unsigned MaxOrder = 420; // Nice integer, 44100/voicepitch

    MyAudioDriver(unsigned rate)
    {
        initialize(1, rate);
        play();
    }

    double bp[MaxOrder]={0};
    unsigned offset=0, count=0;
    unsigned broken_counter=0;

    template<typename T>
    static double get_with_hysteresis(T& value)
    {
        double newval = value;

        static std::map<T*, double> last;
        auto i = last.lower_bound(&value);
        if(i == last.end() || i->first != &value)
            { last.emplace_hint(i, &value, newval); return newval; }
        double& save = i->second;
        double newfac = std::exp2(-10);
        double oldfac = 1 - newfac;
        double oldval = save;
        return save = oldval*oldfac + newval*newfac;
    }

    static constexpr unsigned chunk_size = 4096;
    short sample_buffer[chunk_size];

    virtual bool onGetData(Chunk& data)
    {
        const auto& frame = frames[frame_index];
        assert(frame.coefficients.size() < MaxOrder);
        bool broken = false;

        unsigned num_samples = chunk_size, rate = getSampleRate();
        for(unsigned n=0; n<num_samples; ++n)
        {
            double pitch = get_with_hysteresis(voicepitch);
            // Generate buzz, at the specified voice pitch
            double w = (++count %= unsigned(rate/pitch)) / double(rate/pitch);
            double f = (-0.5 + drand48())*breath + (std::exp2(w) - 1/(1+w))*buzz;
            assert(std::isfinite(f));

            // Apply the filter (LPC coefficients)
            double sum = f;
            for(unsigned j=0; j<frame.coefficients.size(); ++j)
                sum -= frame.coefficients[j]
                       * bp[ (offset + MaxOrder - j) % MaxOrder ];

            if(!std::isfinite(sum) || std::fabs(sum)*std::sqrt(frame.gain) > 10
            || std::isnan(sum))
                { broken = true; }

            // Save it into a rolling buffer
            double r = bp[++offset %= MaxOrder] = sum;

            // And append the sample, multiplied by gain, into the PCM buffer
            r *= std::sqrt(frame.gain) * volume;
            sample_buffer[n] = std::max(std::min(r*32768, 32768.), -32767.);
        }
        if(broken) ++broken_counter;

        data.sampleCount = num_samples;
        data.samples     = sample_buffer;
        return true;
    }
    virtual void onSeek(sf::Time)
    {
    }
};

#include "imgui/imgui.h"
#include "imgui_software_renderer/src/imgui_sw.hpp"

int main(int argc, char** argv)
{
    // Parse commandline parameters
    const char* infn  = (argc>1 && argv[1][0] && (argv[1][1]||argv[1][0]!='-')) ? argv[1] : nullptr;

    // Read lines from the LPC file.
    double sampling_period = 1. / 8000.;
    {std::FILE* in = infn ? std::fopen(infn, "rt") : stdin; if(!in) { std::perror(infn); std::exit(1); }
    {frame cur_frame;
    double coefficient=0;
    for(char Buf[256]; std::fgets(Buf, sizeof(Buf), in); )
    {
        // Identify the line and parse contents
        if(std::sscanf(Buf, "samplingPeriod = %lf",    &sampling_period)==1) { }
        else if(std::sscanf(Buf, "%*s [%*d] = %lf",    &coefficient)==1)     { cur_frame.coefficients.push_back(coefficient); }
        else if(std::sscanf(Buf, "        gain = %lf", &cur_frame.gain)==1)  { frames.emplace_back(std::move(cur_frame));     }
    }}
    if(infn) std::fclose(in);}

    int width_pixels = 1280, height_pixels = 720;

    // Setup window
    sf::RenderWindow window(sf::VideoMode(width_pixels,height_pixels), "LPC sampler",
                            sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
    //window.setVerticalSyncEnabled(true);
    sf::Cursor mousecursors[ImGuiMouseCursor_COUNT];
    mousecursors[ImGuiMouseCursor_Arrow].loadFromSystem(sf::Cursor::Arrow);
    mousecursors[ImGuiMouseCursor_TextInput].loadFromSystem(sf::Cursor::Text);
    mousecursors[ImGuiMouseCursor_ResizeAll].loadFromSystem(sf::Cursor::SizeAll);
    mousecursors[ImGuiMouseCursor_ResizeNS].loadFromSystem(sf::Cursor::SizeVertical);
    mousecursors[ImGuiMouseCursor_ResizeEW].loadFromSystem(sf::Cursor::SizeHorizontal);
    mousecursors[ImGuiMouseCursor_ResizeNESW].loadFromSystem(sf::Cursor::SizeBottomLeftTopRight);
    mousecursors[ImGuiMouseCursor_ResizeNWSE].loadFromSystem(sf::Cursor::SizeTopLeftBottomRight);
    mousecursors[ImGuiMouseCursor_Hand].loadFromSystem(sf::Cursor::Hand);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;       // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "imgui_impl_sfml";
    io.KeyMap[ImGuiKey_Tab] = sf::Keyboard::Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = sf::Keyboard::Left;
    io.KeyMap[ImGuiKey_RightArrow] = sf::Keyboard::Right;
    io.KeyMap[ImGuiKey_UpArrow] = sf::Keyboard::Up;
    io.KeyMap[ImGuiKey_DownArrow] = sf::Keyboard::Down;
    io.KeyMap[ImGuiKey_PageUp] = sf::Keyboard::PageUp;
    io.KeyMap[ImGuiKey_PageDown] = sf::Keyboard::PageDown;
    io.KeyMap[ImGuiKey_Home] = sf::Keyboard::Home;
    io.KeyMap[ImGuiKey_End] = sf::Keyboard::End;
    io.KeyMap[ImGuiKey_Insert] = sf::Keyboard::Insert;
    io.KeyMap[ImGuiKey_Delete] = sf::Keyboard::Delete;
    io.KeyMap[ImGuiKey_Backspace] = sf::Keyboard::Backspace;
    io.KeyMap[ImGuiKey_Space] = sf::Keyboard::Space;
    io.KeyMap[ImGuiKey_Enter] = sf::Keyboard::Enter;
    io.KeyMap[ImGuiKey_Escape] = sf::Keyboard::Escape;
    io.KeyMap[ImGuiKey_A] = sf::Keyboard::A;
    io.KeyMap[ImGuiKey_C] = sf::Keyboard::C;
    io.KeyMap[ImGuiKey_V] = sf::Keyboard::V;
    io.KeyMap[ImGuiKey_X] = sf::Keyboard::X;
    io.KeyMap[ImGuiKey_Y] = sf::Keyboard::Y;
    io.KeyMap[ImGuiKey_Z] = sf::Keyboard::Z;

    auto bigfont   = io.Fonts->AddFontFromFileTTF("AnkaCoder-r.ttf",     26.0f);
    auto smallfont = io.Fonts->AddFontFromFileTTF("AnkaCoder-C87-r.ttf", 9.5f);
    auto medfont   = io.Fonts->AddFontFromFileTTF("AnkaCoder-C87-r.ttf", 14.f);
    io.FontAllowUserScaling = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
/*
    uint8_t* tex_data;
    int      font_width,font_height;
    io.Fonts->GetTexDataAsRGBA32(&tex_data,&font_width,&font_height);
    sf::Texture texture;
    texture.create(font_width, font_height);
    texture.update(tex_data);
    io.Fonts->TexID = 1;//don't care
*/

    imgui_sw::bind_imgui_painting();
    imgui_sw::make_style_fast();

    MyAudioDriver player(1 / sampling_period);

    for(bool running = true; running; )
    {
        // Poll and handle events (inputs, window resize, etc.)
        for(sf::Event event; window.pollEvent(event); )
            switch(event.type)
            {
                case sf::Event::Closed: { window.close(); running = false; break; }
                case sf::Event::MouseWheelScrolled:
                    { if(event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel)
                        io.MouseWheel  += event.mouseWheelScroll.delta;
                      else if(event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                        io.MouseWheelH += event.mouseWheelScroll.delta;
                      break; }
                case sf::Event::MouseMoved:
                    io.MousePos = {0.f+event.mouseMove.x, 0.f+event.mouseMove.y};
                    break;
                case sf::Event::MouseButtonPressed:
                    io.MouseDown[event.mouseButton.button] = true;
                    break;
                case sf::Event::MouseButtonReleased:
                    io.MouseDown[event.mouseButton.button] = false;
                    break;
                case sf::Event::MouseLeft:
                    io.MousePos = {-FLT_MAX, -FLT_MAX};
                    break;
                case sf::Event::KeyPressed:
                case sf::Event::KeyReleased:
                    io.KeyShift = event.key.shift;
                    io.KeyCtrl  = event.key.control;
                    io.KeyAlt   = event.key.alt;
                    io.KeySuper = event.key.system;
                    if(event.key.code >= 0 && event.key.code < IM_ARRAYSIZE(io.KeysDown))
                        io.KeysDown[event.key.code] = event.type == sf::Event::KeyPressed;
                    break;
                case sf::Event::TextEntered:
                    io.AddInputCharacter(event.text.unicode);
                    break;
                default: break;
            }

        // Start the ImGui frame
        auto [width_pixels,height_pixels] = window.getSize();
        io.DisplaySize             = {0.f+width_pixels, 0.f+height_pixels};
        io.DisplayFramebufferScale = {1.f,1.f};
        io.DeltaTime               = 0.01;
        //
        window.setMouseCursorGrabbed(ImGui::IsAnyMouseDown());
        if(!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
        {
            ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
            if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
                { window.setMouseCursorVisible(false); }
            else
                { window.setMouseCursor(mousecursors[imgui_cursor]);
                  window.setMouseCursorVisible(true); }
        }
        ImGui::NewFrame();

        // Create frame
        ImGui::PushFont(bigfont);
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize({0.f+width_pixels,height_pixels*.4f}, ImGuiCond_Once);
        ImGui::SetNextWindowFocus();
        ImGui::Begin("Controls", &running);

        ImGui::Text("Sample rate: %u", unsigned(1 / sampling_period));

        ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
        ImGui::SliderFloat("Breath", &breath, 0.0f, 1.0f);
        ImGui::SliderFloat("Buzz",   &buzz,   0.0f, 1.0f);
        ImGui::SliderInt("Pitch",    &voicepitch, 20, 400);
        ImGui::SliderInt("Frame",    &frame_index, 0, frames.size()-1);

        if(ImGui::Button("Reset broken audio"))    // Buttons return true when clicked
        {
            std::memset(&player.bp, 0, sizeof(player.bp));
            player.broken_counter = 0;
        }
        ImGui::SameLine();
        ImGui::Text(" (detected = %d)", player.broken_counter);
        ImGui::End();

        ImGui::SetNextWindowPos({0.f,height_pixels*.4f}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({0.f+width_pixels,0.f}, ImGuiCond_Once);
        ImGui::Begin("bp", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(smallfont);
        for(unsigned n=0; n< player.MaxOrder; ++n)
        {
            if(n%32) ImGui::SameLine();
            ImGui::Text("%7.3g", player.bp[n]);
        }
        ImGui::PopFont();
        ImGui::End();

        ImGui::SetNextWindowPos({0.f,height_pixels*.71f}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({0.f+width_pixels,0.f},  ImGuiCond_Once);
        ImGui::Begin("frame", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(medfont);

        const auto& f = frames[frame_index];
        ImGui::Text("gain: ");
        ImGui::SameLine();
        ImGui::Text("%.8g", f.gain);
        for(unsigned n=0; n< f.coefficients.size(); ++n)
        {
            if(n%16) ImGui::SameLine();
            ImGui::Text("%7.3g", f.coefficients[n]);
        }
        ImGui::PopFont();
        ImGui::End();

        ImGui::PopFont();

        // Render the frame
        ImGui::Render();

        std::vector<uint32_t> pixels(width_pixels*height_pixels);
        imgui_sw::paint_imgui(&pixels[0], width_pixels,height_pixels);

        sf::Texture texture;
        texture.create(width_pixels,height_pixels);
        texture.update((const unsigned char*)&pixels[0]);
        sf::Sprite sprite(texture);
        window.draw(sprite);
        window.display();
    }

    // Cleanup
    ImGui::DestroyContext();
    imgui_sw::unbind_imgui_painting();
}
