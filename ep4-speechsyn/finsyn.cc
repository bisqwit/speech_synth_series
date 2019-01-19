#include <map>
#include <vector>
#include <locale>
#include <codecvt>
#include <initializer_list>
#include <iostream>
#include <string>
#include <cmath>
#include <tuple>
#include <cassert>
#include <algorithm>

enum class record_modes { choose_randomly, trill, play_in_sequence };

static std::map<char32_t/*record identifier*/,
                std::tuple<record_modes,float/*voice level*/,
                           std::vector<std::tuple<float/*gain*/,std::vector<float/*coefficients*/>>>>> records
{
#include "records.inc"
};

struct prosody_element
{
    char32_t record;
    unsigned n_frames;
    float    relative_pitch;
};

std::vector<prosody_element> phonemize(const std::string& text)
{
    // Canonize the spelling: Convert to lowercase.
    std::locale locale("fi_FI.UTF-8");
    std::string input = text;
    std::use_facet<std::ctype<char>>(locale).tolower(&input[0],&input[0]+input.size());
    // Convert from utf8 to utf32 so we can index characters directly
    std::u32string wip = std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t>{}.from_bytes(input);
    // Canonize the spelling: Replace symbols with phonetic letters.
    static const std::initializer_list<std::pair<std::u32string_view,std::u32string_view>> canonizations
    {
        { U"0", U"nolla" }, { U"1", U"yksi" },    { U"xx",U"kx" },{ U"zz",U"tz" },{ U"cch",U"tch" },{ U"ñ", U"nj" },{ U"nŋ", U"ŋŋ" },
        { U"2", U"kaksi" }, { U"3", U"kolme" },   { U"sch",U"s" },{ U"ch",U"ts" },{ U"ng", U"ŋŋ" }, { U"nk",U"ŋk" },{ U"np", U"mp" },
        { U"4", U"neljä" }, { U"5", U"viisi" },   { U"x", U"ks" },{ U"z", U"ts" },{ U"w", U"v" },   { U"b", U"p" },
        { U"6", U"kuusi" }, { U"7", U"seitsemän"},{ U"c", U"s" }, { U"š", U"s" }, { U"ž", U"s" },   { U"f", U"v" },
        { U"8", U"kahdeksan"},{U"9",U"yhdeksän"}, { U"é", U"e" }, { U"q", U"k" }, { U"g", U"k" },   { U"+", U"plus"},{U"/",U"kautta"},
    };
    for(std::size_t pos = 0; pos < wip.size(); ++pos)
    r1: for(const auto& r: canonizations)
            if(wip.compare(pos, r.first.size(), r.first) == 0)
                { wip.replace(pos, r.first.size(), r.second); pos -= std::min(pos, std::size_t(3)); goto r1; }

    // Next, convert punctuation into pauses.
    wip.insert(0, U"q<"); // Add glottal stop in the beginning
    static const std::initializer_list<std::pair<std::u32string_view,std::u32string_view>> replacements
    {
        { U".", U">¯¯¯¯¯¯q<" }, { U"!", U">¯¯¯¯¯¯q<" }, { U"'", U"q" },  { U";", U"¯¯¯¯|q" },
        { U"?", U">¯¯¯¯¯¯q<" }, { U":", U">¯¯¯¯¯¯q<" }, { U"-", U"q" },  { U",", U"¯¯|q"   }
    };
    for(std::size_t pos = 0; pos < wip.size(); ++pos)
    r2: for(const auto& r: replacements)
            if(wip.compare(pos, r.first.size(), r.first) == 0)
                { wip.replace(pos, r.first.size(), r.second); pos -= std::min(pos, std::size_t(3)); goto r2; }

    auto vow   = [&](char32_t c){return c==U'a'||c==U'e'||c==U'i'||c==U'o'||c==U'u'||c==U'y'||c==U'ä'||c==U'ö';};
    auto alpha = [&](char32_t c){return (c>=U'a'&&c<=U'z')||vow(c); };
    auto cons  = [&](char32_t c){return alpha(c) && !vow(c); };
    auto endpu = [&](char32_t c){return c==U'>'||c==U'¯'||c==U'q'||c==U'"'; };

    // Create a pitch curve. First, divide the phrase into syllables (not perfect, but does not matter).
    std::vector<unsigned> syllable_id(wip.size());
    unsigned cur_syl = 1;
    for(std::size_t pos = 0; pos < wip.size(); )
    {
        while(pos < wip.size() && !alpha(wip[pos])) syllable_id[pos++] = cur_syl++;
        while(pos < wip.size() && cons(wip[pos]))   syllable_id[pos++] = cur_syl;
        while(pos < wip.size() && vow(wip[pos]))    syllable_id[pos++] = cur_syl;
        while(pos < wip.size() && cons(wip[pos]))
        {
            if(vow(wip[pos+1])) break;
            syllable_id[pos++] = cur_syl;
        }
        while(pos < wip.size() && endpu(wip[pos]))  syllable_id[pos++] = cur_syl;
        ++cur_syl;
    }

    std::vector<float> pitch_curve(cur_syl, 1.f);
    std::size_t begin=0, end=syllable_id.size()-1;
    float first=1.f, last=1.f, midpos = 0.8f;
    for(std::size_t pos = 0; pos < wip.size(); ++pos)
    {
        if(wip[pos] == U'<' || wip[pos] == U'|')
        {
            // Find the matching '>', or '|'
            begin  = pos;
            end = wip.find_first_of(U">|", pos+1);
            if(end == wip.npos) end = wip.size();
            first  = (wip[pos] == U'<' ? 1.4f : 1.25f);
            last   = (wip[end] == U'|' ? 1.0f: 0.86f);
            midpos = 0.7 + drand48()*0.1;
            --end;
        }
        // Interpolate pitch between the syllables that have '<' or '|' and those that have '>' or '|'
        float fltpos = midpos;
        if(syllable_id[pos] == syllable_id[begin]) fltpos = 0;
        if(syllable_id[pos] == syllable_id[end])   fltpos = 0.95 + drand48()*0.1;
        fltpos += drand48()*0.2;
        pitch_curve[syllable_id[pos]] = first + (last-first)*fltpos;
    }

    // Finally convert the phonemes into record mappings with timing and intonation information.
    std::vector<prosody_element> elements;
    std::map<char32_t, std::vector<std::tuple<char32_t,unsigned,unsigned,unsigned>>> maps;
    for(const char32_t* p = U"mm890M200:nn890N200:ŋŋ890Ŋ200:aa793:ee793:ii793:oo793:uu793:yy793:¯-700:"
      U"ää793:öö793:k-590k400:p-590p400:t-590t400:d-590d400:ll793:ss793:vv990:jj990:rr990:hh990:qq100:"; *p; ++p)
        for(auto& m = maps[*p++]; *p != U':'; ) { m.emplace_back(p[0],p[1]-U'0',p[2]-U'0',p[3]-U'0'); p += 4; }
    for(std::size_t pos = 0; pos < wip.size(); ++pos)
    {
        std::size_t endpos = pos;
        auto rep_len = [&]{ unsigned r=0; for(; wip[endpos+1] == wip[endpos]; ++r) ++endpos; return r; };
        bool surrounded_by_vowel = (wip[pos+1] != wip[pos] && vow(wip[pos+1])) || (pos>0 && vow(wip[pos-1]));
        if(auto i = maps.find(wip[pos]); i != maps.end())
            for(auto[ch,len,replen,slen]: i->second)
                elements.push_back(prosody_element{ch, len + replen*rep_len() + slen*surrounded_by_vowel, pitch_curve[syllable_id[pos]] });
        else
            switch(wip[pos])
            {
                case U' ':
                case U'\r': case U'\v':
                case U'\n': case U'\t':
                    // Whitespace. If the previous character was a vowel
                    // and the next one is a vowel as well, add a glottal stop.
                    if((pos > 0 && vow(wip[pos-1]) && vow(wip[pos+1])) || wip[pos] != U' ')
                    {
                        elements.push_back(prosody_element{ U'-', 3, pitch_curve[syllable_id[pos]] });
                        elements.push_back(prosody_element{ U'q', 1, pitch_curve[syllable_id[pos]] });
                    }
                    break;
                // Skip unknown characters
                default:
                    std::cout << "Skipped unknown char: '" << (char)wip[pos] << "' (" << wip[pos] << ")\n";
                case U'>': case U'<': case U'|':
                    break;
            }
        pos = endpos;
    }
    return elements;
}

#include <SFML/Audio/SoundStream.hpp>
#include <SFML/System/Sleep.hpp>
#include <mutex>

std::vector<short> audio;
std::mutex         audio_lock;
std::size_t        audio_pos = 0;

struct MyAudioDriver: public sf::SoundStream
{
    MyAudioDriver(unsigned rate)
    {
        initialize(1, rate);
        play();
    }

    static constexpr unsigned chunk_size = 2048;
    short       sample_buffer[chunk_size];

    virtual bool onGetData(Chunk& data)
    {
        unsigned num_samples = chunk_size;
        audio_lock.lock();
        for(unsigned n=0; n<num_samples; ++n)
            sample_buffer[n] = (audio_pos < audio.size()) ? audio[audio_pos++] : 0;
        audio_lock.unlock();

        data.sampleCount = num_samples;
        data.samples     = sample_buffer;
        return true;
    }
    virtual void onSeek(sf::Time)
    {
    }
};

static constexpr unsigned MaxOrder = 256;

template<typename T>
static double get_with_hysteresis(T& value, int speed)
{
    double newval = value;

    static std::map<T*, double> last;
    auto i = last.lower_bound(&value);
    if(i == last.end() || i->first != &value)
        { last.emplace_hint(i, &value, newval); return newval; }
    double& save = i->second;
    double newfac = std::exp2(speed);
    double oldfac = 1 - newfac;
    double oldval = save;
    return save = oldval*oldfac + newval*newfac;
}

static float bp[MaxOrder] = {0};
static unsigned offset=0, count=0;

static void synth(const std::tuple<float,std::vector<float>>& frame, float volume,
                  float breath, float buzz, float pitch, unsigned rate, unsigned n_samples)
{
    bool broken = false;
    static const void* prev = nullptr;
    if(&frame != prev) { prev=&frame; for(auto& f: bp) f=0; }

    static float coeff[MaxOrder]={0};
    const auto& src_coeff = std::get<1>(frame);
    for(unsigned j=0; j<src_coeff.size(); ++j) { coeff[j] = src_coeff[j]; }

    static float p_gain; p_gain     = std::get<float>(frame);
    static float p_pitch; p_pitch   = pitch;
    static float p_breath; p_breath = breath;
    static float p_buzz; p_buzz     = buzz;

    int hyst = -10;
    std::vector<float> slice;
    unsigned retries = 0, retries2 = 0;
    static float average = 0;
    float amplitude = 0, amplitude_limit = 7000;
retry:;
    for(unsigned n=0; n<n_samples; ++n)
    {
        // Generate buzz, at the specified voice pitch
        float pitch = get_with_hysteresis(p_pitch,-10);
        float gain  = get_with_hysteresis(p_gain, hyst);
        hyst = -std::clamp(8.f + std::log10(gain), 1.f, 6.f);

        float w = (++count %= unsigned(rate/pitch)) / float(rate/pitch);
        float f = float(-0.5 + drand48())*get_with_hysteresis(p_breath,-12)
                + (std::exp2(w) - 1/(1+w))*get_with_hysteresis(p_buzz,-12);
        assert(std::isfinite(f));

        // Apply the filter (LPC coefficients)
        float sum = f;
        for(unsigned j=0; j<src_coeff.size(); ++j)
            sum -= get_with_hysteresis(coeff[j], hyst) * bp[ (offset + MaxOrder - j) % MaxOrder ];

        if(!std::isfinite(sum) || std::isnan(sum)) { broken = true; sum = 0.f; }

        // Save it into a rolling buffer
        float r = bp[++offset %= MaxOrder] = sum;

        // And append the sample, multiplied by gain, into the PCM buffer
        float sample = r * std::sqrt(get_with_hysteresis(p_gain,hyst)) * 32768;
        // Click prevention
        average = average*0.9993f + sample*(1 - 0.9993f);
        sample -= average;
        amplitude = amplitude*0.99f + std::abs(sample)*0.01f; if(!n) amplitude=std::abs(sample);
        if(n == 100 && amplitude > amplitude_limit) { ++retries2; amplitude_limit += 500; slice.clear(); if(retries2 < 100) goto retry; }
        // Save sample
        slice.push_back( std::clamp(sample, -32768.f, 32767.f)*volume );
    }
    if(broken) { for(auto& f: bp) f=0; slice.clear(); ++retries;
                 hyst = -1;
                 average = 0;
                 if(retries < 10) goto retry; }
    audio_lock.lock();
    if(audio_pos >= 1048576)
    {
        audio.erase(audio.begin(), audio.begin()+audio_pos);
        audio_pos = 0;
    }
    audio.insert(audio.end(), slice.begin(), slice.end());
    audio_lock.unlock();
    //if(retries || retries2) std::cout << "Retries for broken=" << retries << ", clipping=" << retries2 << '\n';
}

int main()
{
    std::string line(std::istreambuf_iterator<char>(std::cin),
                     std::istreambuf_iterator<char>());
    auto phonemes = phonemize(line);

    const unsigned rate = 44100;
    MyAudioDriver player(rate);

    float frame_time_factor = 0.01f;
    float token_pitch       = 90;
    float volume            = 0.7f;
    float breath_base_level = 0.4f;
    float voice_base_level  = 1.0f;
    for(auto e: phonemes)
    {
        auto i = records.find(e.record);
        if(i == records.end() && ((i = records.find(U'-')) == records.end()))
            { std::cerr << "Missing record: " << e.record << '\n'; continue; }

        // Vary the voice randomly
        token_pitch       = std::clamp(token_pitch       + float(drand48() - 0.5), 50.f, 170.f);
        breath_base_level = std::clamp(breath_base_level + float(drand48() - 0.5) * 0.02f, 0.1f, 1.0f);
        voice_base_level  = std::clamp(voice_base_level  + float(drand48() - 0.5) * 0.02f, 0.7f, 1.0f);

        // Calculate length of time to play the record
        float    frame_time = rate * frame_time_factor;
        unsigned n_samples  = e.n_frames * frame_time;

        // Extract parameters from the record, and play it
        auto& [record_mode, voice_level, alts] = i->second;
        float breath = breath_base_level + (1.f-breath_base_level) * (1.f - voice_level);
        float buzz   = voice_level * voice_base_level;
        float pitch  = e.relative_pitch * token_pitch;

        auto do_synth = [&](const auto& f, unsigned n) { synth(f, volume, breath, buzz, pitch, rate, n); return n; };
        switch(record_mode)
        {
            case record_modes::choose_randomly:
                do_synth(alts[std::rand() % alts.size()], n_samples);
                break;
            case record_modes::play_in_sequence:
                for(std::size_t a=0; a<alts.size(); ++a)
                    do_synth(alts[a], (a+1)*n_samples/alts.size() - (a+0)*n_samples/alts.size());
                break;
            case record_modes::trill:
                for(unsigned n=0; n_samples > 0; ++n)
                    n_samples -= do_synth(alts[n % alts.size()], std::min(n_samples, unsigned(rate * 0.015f)));
        }
    }

    while(audio_pos < audio.size()) { sf::sleep(sf::milliseconds(100)); }
    sf::sleep(sf::milliseconds(1000));
}
