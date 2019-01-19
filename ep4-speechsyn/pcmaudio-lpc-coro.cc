#include <SDL.h>
#include <cmath>
#include <vector>
#include <cstdio>
#include <cassert>
#include <experimental/coroutine>

std::vector<float> data;

generator<float> generator()
{
    constexpr unsigned P = 256;
    constexpr unsigned voice_pitch = 110;

    double k[P]={0}, d[P]={0}, bp[P+1]={0};
    double factor=0, gain=0, frame_length = 0.02, sampling_period = 1. / 96000;
    unsigned offset=0, count=0, index=0, Num=P;
    for(;;)
    {
        char Buf[4096];
        if(!std::fgets(Buf, sizeof Buf, stdin)) break;
        std::sscanf(Buf, "        nCoefficients = %d", &Num);
        std::sscanf(Buf, "dx = %lf",             &frame_length);
        std::sscanf(Buf, "samplingPeriod = %lf", &sampling_period);
        assert(Num <= P);
        if(std::sscanf(Buf, "%*s [%d] = %lf\n", &index, &factor) == 2)
        {
            k[index-1] = factor;
        }
        if(std::sscanf(Buf, "        gain = %lf", &gain) == 1)
        {
            unsigned framemax = unsigned(frame_length/sampling_period);
            for(unsigned frame=0; frame < framemax; ++frame)
            {
                double w = count / double(1/sampling_period/voice_pitch);
                double f = (-0.3 + drand48()*0.6)*0.5 + std::exp2(w) - 1/(1+w);
                if(++count >= unsigned(1./sampling_period/voice_pitch)) { count=0; }

                d[offset%P] = f;

                double sum = f;
                for(unsigned j=0; j<Num; ++j)
                    sum -= bp[ unsigned(offset-j-1)%P ] * k[j];

                double r = bp[offset++ % P] = sum;
                co_yield r * std::sqrt(gain);
            }
        }
    }
}

struct MyAudioDriver
{
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

    std::size_t pos = 0;
    void callback(float* target, int num_samples)
    {
        for(int n=0; n<num_samples; ++n)
            target[n] = data[pos++ % data.size()];
    }
};

int main()
{

    // Initialize SDL audio.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    MyAudioDriver player(1. / sampling_period);

    for(;;) { SDL_Delay(500); }
}
