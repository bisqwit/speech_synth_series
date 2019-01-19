#include <SDL.h>
#include <cmath>

struct MyAudioDriver
{
    SDL_AudioSpec spec {};
    MyAudioDriver()
    {
        spec.freq     = 48000;
        spec.format   = AUDIO_S16SYS;
        spec.channels = 1;
        spec.samples  = 4096;
        spec.userdata = this;
        spec.callback = [](void* param, Uint8* stream, int len)
                        {
                            ((MyAudioDriver*)param)->callback((short*)stream, len/2);
                        };
        auto dev = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, 0);
        SDL_PauseAudioDevice(dev, 0);
    }

    int row=63,freq1,freq2,freq2b,count=0;
    void callback(short* target, int num_samples)
    {
        static const char song[] =
            "057+5420" "+%7%+%7%5%4%2%457%0%0%754%2%+%%%5%542%457%0%0%042%2#+%!#0%+%$%%%";

        for(int position=0; position < num_samples; --count)
        {
            if(!count)
            {
                row = (row+1)%64;
                freq1 = 17 * std::exp2(song[row+8] / 12.); if(song[row+8] == '%') freq1 = 0;
                freq2 = 17 * std::exp2(song[row/8] / 12.); freq2b = (row&2) ? (freq2/2) : 0;
                count = 5400 + 540*(row & 2);
            }

            target[position++] = ( 8*(((count+85)*freq1   & 32767) - 16384)
                                 + 1*(((count+ 4)*freq1*8 & 32767) - 16384)
                                 + 3*(((count+57)*freq2/4 & 32767) - 16384)
                                 + 8*(((count+23)*freq2b  & 32767) - 16384) ) * count >> 16;
        }
    }
};

int main()
{
    // Initialize SDL audio.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    MyAudioDriver player;

    for(;;) { SDL_Delay(500); }
}
