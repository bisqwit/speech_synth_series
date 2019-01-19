#include <SDL.h>
#include <cmath>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <map>

/*
g++ pcmaudio-lpc-browser.cc -std=c++17 $(pkg-config sdl2 gl --libs --cflags) \
    imgui/{imgui{,_draw,_widgets},examples/imgui_impl_{sdl,opengl2}}.cpp \
    -Iimgui -O3 -Wall -Wextra -pedantic -g
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

struct MyAudioDriver
{
    static constexpr unsigned MaxOrder = 420; // Nice integer, 44100/voicepitch

    SDL_AudioSpec spec {};

    MyAudioDriver(unsigned rate)
    {
        spec.freq     = rate;
        spec.format   = AUDIO_F32SYS;
        spec.channels = 1;
        spec.samples  = 4096;
        spec.userdata = this;
        spec.callback = [](void* param, Uint8* stream, int len)
                        {
                            ((MyAudioDriver*)param)->callback((float*)stream, len/sizeof(float));
                        };
        auto dev = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, 0);
        SDL_PauseAudioDevice(dev, 0);
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

    void callback(float* target, int num_samples)
    {
        const auto& frame = frames[frame_index];
        assert(frame.coefficients.size() < MaxOrder);
        bool broken = false;

        /*static double coeff[MaxOrder];
        for(unsigned d=0; d<frame.coefficients.size(); ++d)
            coeff[d] = frame.coefficients[d];*/

        for(int n=0; n<num_samples; ++n)
        {
            double pitch = get_with_hysteresis(voicepitch);
            // Generate buzz, at the specified voice pitch
            double w = (++count %= unsigned(spec.freq/pitch)) / double(spec.freq/pitch);
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
            target[n] = r * std::sqrt(frame.gain) * volume;
        }

        if(broken) ++broken_counter;
    }
};

#include "imgui.h"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl2.h"
#include <SDL_opengl.h>

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

    // Initialize SDL audio and video.
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,  1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,  8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    // Setup window
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window* window = SDL_CreateWindow(
        "LPC sampler",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1280,720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    auto bigfont   = io.Fonts->AddFontFromFileTTF("AnkaCoder-r.ttf",     26.0f);
    auto smallfont = io.Fonts->AddFontFromFileTTF("AnkaCoder-C87-r.ttf", 9.5f);
    auto medfont   = io.Fonts->AddFontFromFileTTF("AnkaCoder-C87-r.ttf", 14.f);
    io.FontAllowUserScaling = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    MyAudioDriver player(1 / sampling_period);

    for(bool running = true; running; )
    {
        // Poll and handle events (inputs, window resize, etc.)
        for(SDL_Event event; SDL_PollEvent(&event); ImGui_ImplSDL2_ProcessEvent(&event))
        {
            if (event.type == SDL_QUIT
            || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE
             && event.window.windowID == SDL_GetWindowID(window)))
                running = false;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGui::PushFont(bigfont);
        int width=64,height=64;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize({0.f+width,height*.4f}, ImGuiCond_Once);
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

        ImGui::SetNextWindowPos({0.f,height*.4f}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({0.f+width,0.f}, ImGuiCond_Once);
        ImGui::Begin("bp", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(smallfont);
        for(unsigned n=0; n< player.MaxOrder; ++n)
        {
            if(n%32) ImGui::SameLine();
            ImGui::Text("%7.3g", player.bp[n]);
        }
        ImGui::PopFont();
        ImGui::End();

        ImGui::SetNextWindowPos({0.f,height*.71f}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({0.f+width,0.f},  ImGuiCond_Once);
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

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
