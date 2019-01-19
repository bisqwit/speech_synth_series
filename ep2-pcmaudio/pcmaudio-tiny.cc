#include <SDL.h>

struct MyAudioEngine
{
    SDL_AudioSpec spec {};
    unsigned      pitch = 0, wave_counter = 0;

    MyAudioEngine()
    {
        spec.freq     = 96000;         // 96000 samples per second (96 kHz)
        spec.format   = AUDIO_F32SYS;  // Sample format: 32-bit float (system-defined byte order)
        spec.channels = 1;             // Number of channels: 1 (mono)
        spec.samples  = 4096;          // Buffer size in samples (chosen rather arbitrarily)
        spec.userdata = this;
        spec.callback = [](void* param, Uint8* stream, int len)
                        {
                            ((MyAudioEngine*)param)->callback((float*) stream, len / sizeof(float));
                        };
        auto dev = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
        SDL_PauseAudioDevice(dev, 0);
    }

    void callback(float* target, int num_samples)
    {
        // Generate "num_samples" samples of square-wave
        // that alternates between signal levels -0.5 and +0.5
        for(int position = 0; position < num_samples; ++position)
        {
            // Generate one sample
            float sample = ((wave_counter / spec.freq) % 2 == 0) ? -0.5f : 0.5f;
            wave_counter += pitch*2;
            // And put it into the buffer
            target[position] = sample;
        }
        // Note: This function generates aliased square-wave for simplicity.
        // To learn how to generate "perfect" noiseless band-limited square-wave
        // that sounds good even at low sampling rates, read this article: 
        // https://www.nayuki.io/page/band-limited-square-waves
    }
};

int main()
{
    // Initialize SDL audio.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    MyAudioEngine beeper;

    // Toggle the pitch between 440 Hz and 352 Hz every 0.5 seconds
    for(;;)
    {
        beeper.pitch = 440;
        SDL_Delay(500);

        beeper.pitch = 352;
        SDL_Delay(500);
    }
}




