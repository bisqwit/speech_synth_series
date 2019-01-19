#include <cmath>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>

int main(int argc, char** argv)
{
    constexpr unsigned MaxOrder = 256, VoicePitch = 105, DefaultRate = 8000;
    std::vector<float> samples;
    double k[MaxOrder]={0}, bp[MaxOrder]={0};
    double coefficient=0, gain=0, frame_length = 0.02, sampling_period = 1. / DefaultRate;
    unsigned offset=0, count=0, index=0, Order=MaxOrder;

    // Parse commandline parameters
    const char* infn  = (argc>1 && argv[1][0] && (argv[1][1]||argv[1][0]!='-')) ? argv[1] : nullptr;
    const char* outfn = (argc>2 && argv[2][0] && (argv[2][1]||argv[2][0]!='-')) ? argv[2] : nullptr;

    // Read lines from the LPC file.
    std::FILE* in = infn ? std::fopen(infn, "rt") : stdin; if(!in) { std::perror(infn); std::exit(1); }
    for(char Buf[256]; std::fgets(Buf, sizeof(Buf), in); )
    {
        // Identify the line and parse contents
        if(std::sscanf(Buf, "        nCoefficients = %d", &Order) == 1)          { assert(Order <= MaxOrder); }
        else if(std::sscanf(Buf, "dx = %lf",             &frame_length) == 1)    { }
        else if(std::sscanf(Buf, "samplingPeriod = %lf", &sampling_period) == 1) { }
        else if(std::sscanf(Buf, "%*s [%d] = %lf\n", &index, &coefficient) == 2) { assert(index <= Order); k[index-1] = coefficient; }
        else if(std::sscanf(Buf, "        gain = %lf", &gain) == 1)
        {
            unsigned rate = 1/sampling_period, samples_per_frame = unsigned(frame_length/sampling_period);
            for(unsigned sample=0; sample < samples_per_frame; ++sample)
            {
                // Generate buzz, at the specified voice pitch
                double w = (++count %= (rate/VoicePitch)) / double(rate/VoicePitch);
                double f = (-0.5 + drand48())*0.5 + std::exp2(w) - 1/(1+w);

                // Apply the filter (LPC coefficients)
                double sum = f;
                for(unsigned j=0; j<Order; ++j)
                    sum -= k[j] * bp[ (offset+MaxOrder-j) % MaxOrder ];

                // Save it into a rolling buffer
                double r = bp[++offset %= MaxOrder] = sum;

                // And append the sample, multiplied by gain, into the PCM buffer
                samples.push_back(r * std::sqrt(gain));
            }
        }
    }
    if(infn) std::fclose(in);

    // Save the PCM buffer into a well-formed WAV file.
    // The WAV header can be simplified into consisting of eleven 4-byte records, some of which
    // are text and some of which are integers. This put() writes an integer into the header.
    std::FILE* out = outfn ? std::fopen(outfn, "wb") : stdout; if(!out) { std::perror(outfn); std::exit(2); }
    char RIFF[] = "RIFF----WAVEfmt ____----____----____data---";
    auto put = [&](unsigned offset, unsigned value) { for(unsigned n=0; n<32; n+=8) RIFF[offset + n/8] = value >> n; };
    put( 1*4, 36);               // sizeof "RIFF" record
    put( 4*4, 16);               // sizeof "fmt " record
    put( 5*4, 1*65536+3);        // 1 channel, 3=float fmt
    put( 6*4,   unsigned(1/sampling_period)); // Sample rate
    put( 7*4, 4*unsigned(1/sampling_period)); // Data rate: sizeof(sample) * sample rate
    put( 8*4, (4*8)*65536+4);    // sizeof(sample)*8 bits per sample, alignment = sizeof(sample)
    put(10*4, 4*samples.size()); // Size of "data" record: sizeof(sample) * number of samples.
    std::fwrite(RIFF,        1, sizeof(RIFF),   out);
    std::fwrite(&samples[0], 4, samples.size(), out);
    if(outfn) std::fclose(out);
}
