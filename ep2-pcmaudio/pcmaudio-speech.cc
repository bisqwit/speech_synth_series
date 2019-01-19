#include <SDL.h>
#include <cstdio>
#include <cstring>

struct MyAudioDriver
{
    SDL_AudioSpec spec {};
    MyAudioDriver()
    {
        spec.freq     = 16000;
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

    std::FILE* fp = nullptr;
    void callback(float* target, int num_samples)
    {
        if(!fp)
        {
            fp = std::fopen("proverbs31.wav", "rb");
            std::fseek(fp, 0x355030, SEEK_SET);
        }
        std::memset(target, 0, sizeof(float) * num_samples);
        /* Read samples from a file */
        if(!std::fread(target, sizeof(float), num_samples, fp))
        {
            std::fclose(fp);
            fp = nullptr;
        }
    }
};

int main()
{
    // Initialize SDL audio.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    MyAudioDriver speaker;

    for(;;) { SDL_Delay(500); }
}
