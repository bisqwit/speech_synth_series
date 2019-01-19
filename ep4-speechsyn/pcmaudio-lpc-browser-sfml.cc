#include <cmath>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <map>

#include <SFML/Audio/SoundStream.hpp>
#include <SFML/Graphics.hpp>
#include <imgui.h>

/*
Compilation:

sudo apt-get install libsfml-dev
git clone https://github.com/ocornut/imgui
g++ pcmaudio-lpc-browser-sfml.cc -std=c++17 -O3 -Wall -Wextra -pedantic -g \
    $(pkg-config sfml-audio sfml-graphics --libs --cflags) \
    -Iimgui imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp
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

    static constexpr unsigned chunk_size = 2048;
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
    window.setVerticalSyncEnabled(true);

    sf::Cursor mousecursors[ImGuiMouseCursor_COUNT];
    #define M(IM,SF) mousecursors[ImGuiMouseCursor_##IM].loadFromSystem(sf::Cursor::SF);
    M(Hand,Hand)         M(ResizeNS,SizeVertical)
    M(Arrow,Arrow)       M(ResizeEW,SizeHorizontal)
    M(TextInput,Text)    M(ResizeNESW,SizeBottomLeftTopRight)
    M(ResizeAll,SizeAll) M(ResizeNWSE,SizeTopLeftBottomRight)
    #undef M

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;  // We can honor GetMouseCursor() values (optional)
    io.BackendPlatformName = "imgui_impl_sfml";

    #define K(IM,SF) io.KeyMap[ImGuiKey_##IM] = sf::Keyboard::SF;
    K(Tab,Tab)     K(UpArrow,Up)    K(DownArrow,Down)
    K(End,End)     K(PageUp,PageUp) K(PageDown,PageDown)
    K(Space,Space) K(Insert,Insert) K(Backspace,Backspace)
    K(Enter,Enter) K(Delete,Delete) K(LeftArrow,Left)
    K(Home,Home)   K(Escape,Escape) K(RightArrow,Right)
    K(A,A) K(C,C) K(V,V) K(X,X) K(Y,Y) K(Z,Z)
    #undef K

    auto bigfont   = io.Fonts->AddFontFromFileTTF("AnkaCoder-r.ttf",     26.0f);
    auto smallfont = io.Fonts->AddFontFromFileTTF("AnkaCoder-C87-r.ttf",  9.5f);
    auto medfont   = io.Fonts->AddFontFromFileTTF("AnkaCoder-C87-r.ttf", 14.0f);
    io.FontAllowUserScaling = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    unsigned char* tex_data=nullptr;
    int            font_width=1,font_height=1;
    io.Fonts->GetTexDataAsRGBA32(&tex_data,&font_width,&font_height);
    sf::Texture texture;
    texture.create(font_width, font_height);
    texture.update(tex_data);
    //io.Fonts->TexID = nullptr; //irrelevant

    MyAudioDriver player(1 / sampling_period);
    sf::Clock clock;

    for(bool running = true; running; )
    {
        // Poll and handle events (inputs, window resize, etc.)
        for(sf::Event event; window.pollEvent(event); )
            switch(event.type)
            {
                case sf::Event::Closed: { window.close(); running = false; break; }
                case sf::Event::MouseWheelScrolled:
                    { if(event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel)
                        io.MouseWheelH += event.mouseWheelScroll.delta;
                      else if(event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                        io.MouseWheel  += event.mouseWheelScroll.delta;
                      break; }
                case sf::Event::MouseMoved:
                    io.MousePos = {0.f+event.mouseMove.x / io.DisplayFramebufferScale.x,
                                   0.f+event.mouseMove.y / io.DisplayFramebufferScale.y};
                    break;
                case sf::Event::MouseButtonPressed:
                    io.MouseDown[event.mouseButton.button] = true;
                    break;
                case sf::Event::MouseButtonReleased:
                    io.MouseDown[event.mouseButton.button] = false;
                    break;
                case sf::Event::MouseLeft:
                    io.MousePos = {-FLT_MAX, -FLT_MAX};
                    for(auto& b: io.MouseDown) b = false;
                    break;
                case sf::Event::KeyPressed:
                case sf::Event::KeyReleased:
                    if(event.key.code >= 0 && event.key.code < IM_ARRAYSIZE(io.KeysDown))
                        io.KeysDown[event.key.code] = event.type == sf::Event::KeyPressed;
                    io.KeyShift = io.KeysDown[sf::Keyboard::LShift]   || io.KeysDown[sf::Keyboard::RShift];
                    io.KeyCtrl  = io.KeysDown[sf::Keyboard::LControl] || io.KeysDown[sf::Keyboard::RControl];
                    io.KeyAlt   = io.KeysDown[sf::Keyboard::LAlt]     || io.KeysDown[sf::Keyboard::RAlt];
                    io.KeySuper = io.KeysDown[sf::Keyboard::LSystem]  || io.KeysDown[sf::Keyboard::RSystem];
                    break;
                case sf::Event::TextEntered:
                    io.AddInputCharacter(event.text.unicode);
                    break;
                default:break;
            }

        // Start the ImGui frame
        {auto size = window.getSize();
        io.DisplaySize             = {0.f+width_pixels, 0.f+height_pixels};
        io.DisplayFramebufferScale = {size.x / float(width_pixels), size.y / float(height_pixels)};
        texture.setSmooth( io.DisplayFramebufferScale.x!=1.f || io.DisplayFramebufferScale.y!=1.f );
        io.DeltaTime               = clock.restart().asSeconds();}
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
        window.clear({60,60,60});
        sf::VertexArray triangles(sf::Triangles);
        const ImDrawData* draw_data = ImGui::GetDrawData();
        for(int i = 0; i < draw_data->CmdListsCount; ++i)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[i];
            const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer[0];
            const ImDrawVert* vertices = cmd_list->VtxBuffer.Data;
            for(int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
            {
                const ImDrawCmd& pcmd = cmd_list->CmdBuffer[cmd_i];
                if (pcmd.UserCallback)
                    pcmd.UserCallback(cmd_list, &pcmd);
                else
                    for(unsigned i = 0; i < pcmd.ElemCount; ++i)
                    {
                        const ImDrawVert& v = vertices[idx_buffer[i]];
                        auto ByteSwap = [](unsigned color)
                        {
                            color = (color >> 16) | (color << 16);
                            color = ((color & 0xFF00FFu) << 8) | ((color & 0xFF00FF00u) >> 8);
                            return color;
                        };
                        // oddly enough the colors have to be byteswapped here,
                        // but not in the texture...
                        triangles.append(sf::Vertex{
                            {v.pos.x,v.pos.y}, sf::Color(ByteSwap(v.col)),
                            {v.uv.x*font_width, v.uv.y*font_height} });
                    }
                idx_buffer += pcmd.ElemCount;
            }
        }
        window.draw(triangles, &texture);
        window.display();
    }

    // Cleanup
    ImGui::DestroyContext();
    window.close();
}
