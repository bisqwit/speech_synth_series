#include <SDL.h>
#include <cmath>
#include <vector>
#include <cstdio>

std::vector<float> data;

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
    constexpr unsigned P = 256;
    constexpr unsigned F = 110;

    double k[P]={0};
    double prevk[P]={0}, prevgain=0;
    double bp[P+1]={0};
    double d[P]={0};
    unsigned offset=0;
    double frame_length = 0.02, sampling_period = 1. / 96000;

    //for(auto& d: bp) d=1;

    unsigned count=0;
    for(;;)
    {
        char Buf[4096];
        if(!std::fgets(Buf, sizeof Buf, stdin)) break;
        int index=0, Num=P;
        double factor=0, gain=0;
        std::sscanf(Buf, "        nCoefficients = %d", &Num);
        std::sscanf(Buf, "dx = %lf", &frame_length);
        std::sscanf(Buf, "samplingPeriod = %lf", &sampling_period);

        int n = std::sscanf(Buf, "            a [%d] = %lf\n", &index, &factor);
        int m = std::sscanf(Buf, "        gain = %lf", &gain);

        if(n == 2)
        {
            /*constexpr int ratio = 2873/2; // 2873
            int intfac = factor*ratio;
            factor = intfac / (double)ratio;
            static int minfac=0, maxfac=0; bool c=false;
            if(intfac < minfac) {c=true; minfac=intfac; }
            if(intfac > maxfac) {c=true; maxfac=intfac; }
            static double minfa=0, maxfa=0;
            if(factor < minfa) {c=true; minfa=factor; }
            if(factor > maxfa) {c=true; maxfa=factor; }
            printf("min=%d max=%d orig min=%g max=%g\n", minfac,maxfac, minfa,maxfa);
            */
            k[index-1] = factor;
        }
        if(m == 1)
        {
            //printf("gain=%g\n", gain);
            // process frame
            printf("%g\t", d[offset%P]);
            unsigned framemax = unsigned(frame_length/sampling_period);
            for(unsigned frame=0; frame < framemax; ++frame)
            {
                double w  = count / double(1/sampling_period/F);
                double f = (-0.1 + drand48()*0.6)*0.1
                         + (1 - 2*(60.0 / ((w *30+20)*(w *40+3))));

                //double point = frame / double(framemax);
                double tempk[P], tempgain;
                for(unsigned n=0; n<P; ++n)
                    tempk[n] = prevk[n];// + (k[n] - prevk[n]) * point;
                tempgain = prevgain;// + gain*point;

                ++count;
                if(count >= unsigned(1./sampling_period/F)) { count=0; }
                d[offset%P] = f;

                double sum = f;
                for(unsigned j=0; j< unsigned(Num); ++j)
                    sum -= bp[ unsigned(offset-j-1)%P ] * tempk[j];
                //sum=f;gain=1;
                //if(std::abs(sum) > 1e4) sum = 0;

                double r = bp[offset%P] = sum;
                data.push_back(r * std::sqrt(tempgain));

                ++offset;
            }
            for(unsigned n=0; n<P; ++n) prevk[n] = k[n];
            prevgain = gain;
        }
    }

    // Initialize SDL audio.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    MyAudioDriver player(1. / sampling_period);

    for(;;) { SDL_Delay(500); }
}
