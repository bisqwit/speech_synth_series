#include <SFML/Audio/SoundStream.hpp>
#include <SFML/System/Sleep.hpp>
#include <cmath>

struct MyAudioDriver: public sf::SoundStream
{
    MyAudioDriver()
    {
        initialize(1, 48000);
        play();
    }

    int row=63,freq1,freq2,freq2b,count=0;

    static constexpr unsigned chunk_size = 2048;
    short sample_buffer[chunk_size];

    virtual bool onGetData(Chunk& data)
    {
        static const char song[] =
            "057+5420" "+%7%+%7%5%4%2%457%0%0%754%2%+%%%5%542%457%0%0%042%2#+%!#0%+%$%%%";

        unsigned num_samples = chunk_size;
        for(unsigned position=0; position<num_samples; --count)
        {
            if(!count)
            {
                row = (row+1)%64;
                freq1 = 17 * std::exp2(song[row+8] / 12.); if(song[row+8] == '%') freq1 = 0;
                freq2 = 17 * std::exp2(song[row/8] / 12.); freq2b = (row&2) ? (freq2/2) : 0;
                count = 5400 + 540*(row & 2);
            }

            sample_buffer[position++] = ( 8*(((count+85)*freq1   & 32767) - 16384)
                                        + 1*(((count+ 4)*freq1*8 & 32767) - 16384)
                                        + 3*(((count+57)*freq2/4 & 32767) - 16384)
                                        + 8*(((count+23)*freq2b  & 32767) - 16384) ) * count >> 16;
        }
        data.sampleCount = num_samples;
        data.samples     = sample_buffer;
        return true;
    }
    virtual void onSeek(sf::Time)
    {
    }
};

int main()
{
    // Initialize SFML audio.
    MyAudioDriver player;

    for(;;) { sf::sleep(sf::milliseconds(500)); }
}
