    // Save the PCM buffer into a well-formed WAV file.
    // The WAV header can be simplified into consisting of eleven 4-byte records, some of which
    // are text and some of which are integers. This put() writes an integer into the header.
    const char* outfn = "-";
    std::FILE* out = outfn ? std::fopen(outfn, "wb") : stdout; if(!out) { std::perror(outfn); std::exit(2); }
    char RIFF[] = "RIFF----WAVEfmt ____----____----____data---";
    auto put = [&](unsigned offset, unsigned value) { for(unsigned n=0; n<32; n+=8) RIFF[offset + n/8] = value >> n; };
    put( 1*4, 36);               // sizeof "RIFF" record
    put( 4*4, 16);               // sizeof "fmt " record
    put( 5*4, 1*65536+1);        // 1 channel, 3=float fmt
    put( 6*4,   44100); // Sample rate
    put( 7*4, 2*44100); // Data rate: sizeof(sample) * sample rate
    put( 8*4, (2*8)*65536+2);    // sizeof(sample)*8 bits per sample, alignment = sizeof(sample)
    put(10*4, 2*audio.size()); // Size of "data" record: sizeof(sample) * number of audio.
    std::fwrite(RIFF,      1, sizeof(RIFF),   out);
    std::fwrite(&audio[0], 2, audio.size(), out);
    if(outfn) {std::fclose(out);} return 0;
