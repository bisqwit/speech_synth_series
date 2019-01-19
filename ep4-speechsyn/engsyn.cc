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
#include <array>

enum class record_modes { choose_randomly, trill, play_in_sequence };

static std::map<char32_t, std::tuple<record_modes,float/*voice*/,std::vector<std::tuple<float,std::vector<float>>>>> records
{
#include "records.inc"
};

struct prosody_element
{
    char32_t record;
    unsigned n_frames;
    float    relative_pitch;
};

static std::string to8(const std::u32string& s)
{
    return std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t>{}.to_bytes(s);
}

std::u32string english_convert(const std::u32string& text)
{
    // Based on: Automatic translation of English text to phonetics,
    //           NRL Report 7948 (AD/A021 929), on 1976-01-21
static char32_t patterns[] = U".|'s||z,#:.e|'s||z,#|'s||z,|'||,|a| |@, |are| |0r, |ar|o|@r,|ar|#|er,^|as|#|eIs,|a|wa|@,|aw||O, :|any||eni,|a|^+#|eI,#:|ally||@li, |al|#|@l,|aga"
U"in||@gen,#:|ag|e|IdZ,|a|^+:#|&, :|a|^+ |eI,|a|^%|eI, |arr||@r,|arr||&r, :|ar| |0r,|ar| |3,|ar||0r,|air||e3,|ai||eI,|ay||eI,|au||O,#:|al| |@l,#:|als| |@lz,|alk||Ok,|al|^|Ol, :|able||eIb@l,|able||@b@l,|ang|+|eI"
U"ndZ,^|a|^#|eI,|a||&, |be|^#|bI,|being||biIN, |both| |b@UT, |bus|#|bIz,|buil||bIl,|b||b, |ch|^|k,^e|ch||k,|ch||tS, s|ci|#|saI,|ci|a|S,|ci|o|S,|ci|en|S,|c|+|s,|ck||k,|com|%|kVm,|c||k,#:|ded| |dId,.e|d| |d,#:^e|"
U"d| |t, |de|^#|dI, |do| |du, |does||dVz, |doing||duIN, |dow||daU,|du|a|dZu,|d||d,#:|e| |,':^|e| |, :|e| |i,|ery||erI,#|ed| |d,#:|e|d |,|ev|er|ev,|e|n+r|e,|e|^%|i,|eri|#|iri,|eri||erI,#:|er|#|3,|er|#|er,|er||3,"
U" |even||iven,#:|e|w|,t|ew||u,s|ew||u,r|ew||u,d|ew||u,l|ew||u,z|ew||u,n|ew||u,j|ew||u,th|ew||u,ch|ew||u,sh|ew||u,|ew||ju,|e|o|i,#:s|es| |Iz,#:c|es| |Iz,#:g|es| |Iz,#:z|es| |Iz,#:x|es| |Iz,#:j|es| |Iz,#:ch|es| "
U"|Iz,#:sh|es| |Iz,#:|e|s |,#:|ely| |li,#:|ement||ment,|eful||fUl,|ee||i,|earn||3n, |ear|^|3,|ead||ed,#:|ea| |i@,|ea|su|e,|ea||i,|eigh||eI,|ei||i, |eye||aI,|ey||i,|eu||ju,|e||e,|ful||fUl,|f||f,|giv||gIv, |g|i^|"
U"g,|ge|t|ge,su|gges||gdZes,|gg||g,#|gn|%|n,#|gn| |n, b#|g||g,|g|+|dZ,|great||greIt,#|gh||,|g||g, |hav||h&v, |here||hir, |hour||aU3,|how||haU,|h|#|h,|h||, |iain| |I@n, |ing| |IN, |in||In, |i| |aI,|i'm||aIm,|in|"
U"d|aIn,|ier||i3,#:r|ied||id,|ied| |aId,|ien||ien,|ie|t|aIe, :|i|%|aI,|i|%|i,|ie||i,|i|^+:#|I,|ir|#|aIr,|iz|%|aIz,|is|%|aIz,|i|d%|aI,+^|i|^+|I,|i|t%|aI,#:^|i|^+|I,#|i|g|aI,|i|^+|aI,|ir||3,|igh||aI,|ild||aIld,|i"
U"que||ik,^|i|^#|aI,|i||I,|j||dZ, |k|n|,|k||k,|lo|c#|l@U,l|l||,#:^|l|%|@l,|lead||lid,|l||l,|mov||muv,#|mm|#|m,|m||m,e|ng|+|ndZ,|ng|r|Ng,|ng|#|Ng,|ngl|%|Ng@l,|ng||N,|nk||Nk, |now| |naU,#|ng| |Ng,|n||n,|of| |@v,|"
U"orough||3@U,#:|or| |3,#:|ors| |3z,|or||Or, |one||wVn,|ow||@U, |over||@Uv3,|ov||Vv,|o|^%|@U,|o|^en|@U,|o|^i#|@U,|ol|d|@Ul,|ought||Ot,|ough||Vf, |ou||aU,h|ou|s#|aU,|ous||@s,|our||Or,|ould||Ud,^|ou|^l|V,|oup||up"
U",|ou||aU,|oy||oI,|oing||@UIN,|oi||oI,|oor||Or,|ook||Uk,|ood||Ud,|oo||u,|o|e|@U,|o| |@U,|oa||@U, |only||@Unli, |once||wVns,|on't||@Unt,c|o|n|0,|o|ng|O, :^|o|n|V,i|on||@n,#:|on| |@n,#^|on||@n,|o|st |@U,|of|^|Of"
U",|other||VD3,|oss| |Os,#:^|om||Vm,|o||0,|ph||f,|peop||piip,|pow||paU,|put| |pUt,|p||p,|quar||kwOr,|qu||kw,|q||k, |re|^#|ri,|r||r,|sh||S,#|sion||Z@n,|some||sVm,#|sur|#|Z3,|sur|#|S3,#|su|#|Zu,#|ssu|#|Su,#|sed| "
U"|zd,#|s|#|z,|said||sed,^|sion||S@n,|s|s|,.|s| |z,#:.e|s| |z,#:^##|s| |z,#:^#|s| |s,u|s| |s, :#|s| |z, |sch||sk,|s|c+|,#|sm||zm,#|sn|'|z@n,|s||s, |the| |D@,|to| |tu,|that| |D&t, |this| |DIs, |they||DeI, |there"
U"||Der,|ther||D3,|their||Der, |than| |D&n, |them| |Dem,|these| |Diz, |then||Den,|through||Tru,|those||D@Uz,|though| |D@U, |thus||DVs,|th||T,#:|ted| |tId,s|ti|#n|tS,|ti|o|S,|ti|a|S,|tien||S@n,|tur|#|tS3,|tu|a|t"
U"Su, |two||tu,|t||t, |un|i|jun, |un||Vn, |upon||@pOn,t|ur|#|Ur,s|ur|#|Ur,r|ur|#|Ur,d|ur|#|Ur,l|ur|#|Ur,z|ur|#|Ur,n|ur|#|Ur,j|ur|#|Ur,th|ur|#|Ur,ch|ur|#|Ur,sh|ur|#|Ur,|ur|#|jUr,|ur||3,|u|^ |V,|u|^^|V,|uy||aI, g"
U"|u|#|,g|u|%|,g|u|#|w,#n|u||ju,t|u||u,s|u||u,r|u||u,d|u||u,l|u||u,z|u||u,n|u||u,j|u||u,th|u||u,ch|u||u,sh|u||u,|u||ju,|view||vju,|v||v, |were||w3,|wa|s|w0,|wa|t|w0,|where||wer,|what||w0t,|whol||h@Ul,|who||hu,|"
U"wh||w,|war||wOr,|wor|^|w3,|wr||r,|w||w,|x||ks,|young||jVN, |you||ju, |yes||jes, |y||j,#:^|y| |i,#:^|y|i|i, :|y| |aI, :|y|#|aI, :|y|^+:#|I, :|y|^#|aI,|y||I,|z||z,";
    std::array<std::vector<std::array<const char32_t*,4>>,26+1> rules;
    for(char32_t* s = patterns; *s; )
    {
        std::array<const char32_t*,4> row;
        for(unsigned n=0; n<4; ++n, *s++ = U'\0')
           for(row[n] = s; *s != U"|||,"[n]; ++s)
                {}
        rules[(row[1][0] >= U'a' && row[1][0] <= U'z') ? 1 + (row[1][0] - U'a') : 0].emplace_back(std::move(row));
    }
    auto match = [&](auto p, auto g)
    {
        auto in   = [&](char32_t c, std::u32string_view s) { return s.find(c) != std::string_view::npos; };
        auto cons = [&](char32_t c) { return in(c, U"bcdfghjklmnpqrstvwxyz"); };
        for(char32_t pat; (pat = p()) != U'\0'; )
            switch(pat)
            {
                case U'#': {unsigned n=0; for(; in(g(0), U"aeiou"); ++n) g(1); if(!n) return false; else break;}
                case U':': while(cons(g(0))) g(1); break;
                case U'^': if(!cons(g(1))) return false; else break;
                case U'.': if(!in(g(1), U"bdvgjlmnrwz")) return false; else break;
                case U'+': if(!in(g(1), U"eyi"        )) return false; else break;
                case U'%': // ER,E,ES,ED,ING,ELY (suffix)
                    if(char32_t c = g(1); c==U'i') { if(g(1)!=U'n' || g(1)!=U'g') return false; }
                    else if(c!=U'e') return false;
                    else if(char32_t c2 = g(1); c2==U'l') { if(g(1)!=U'y') return false; }
                    else if(c2!=U'r' && c2!=U's' && c2!=U'd') return false;
                    else break;
                case U' ': if(char32_t c = g(1); (c>=U'a'&&c<=U'z') || c==U'\'') return false; else break;
                default:   if(pat != g(1)) return false; else break;
            }
        return true;
    };
    std::u32string result;
    for(std::size_t position = 1; position < text.size(); )
    {
        for(const auto& rule: rules[(text[position] >= U'a' && text[position] <= U'z') ? 1 + (text[position] - U'a') : 0])
        {
            const char32_t* r_pat = rule[2], *l_pat = rule[0], *m_pat = rule[1];
            std::size_t m_size = std::u32string_view(m_pat).size(), r_pos = position+m_size;
            std::size_t l_size = std::u32string_view(l_pat).size(), l_pos = position-1;
            // Match middle, right and left parts
            if(text.compare(position, m_size, m_pat) == 0
            && match([&]{return *r_pat++;},                             [&](int n){ char32_t c = text[r_pos]; r_pos+=n; return c; })
            && match([&]{return l_size==0 ? U'\0' : l_pat[--l_size]; }, [&](int n){ char32_t c = text[l_pos]; l_pos-=n; return c; }))
            {
                result   += rule[3];
                position += m_size;
                goto r3;
            }
        }
        result += text[position++]; r3:;
    }
    // At this point, the result string is pretty much an ASCII representation of IPA.
    // Now just touch up it a bit to convert it into typical Finnish pronunciation.
    static const char32_t tab[][2][5] = {
    {U"@U", U"öy"}   /* OW |goat,shOW,nO|                               */
   ,{ U"@", U"ö" }   /* AX |About,commA,commOn|                         */
   ,{ U"A", U"aa"}   /* AA |stARt,fAther|                               */
   ,{ U"&", U"ä" }   /* AE |trAp,bAd|                                   */
   ,{ U"V", U"a" }   /* AH |strUt,bUd,lOve|                             */
   ,{U"Or", U"oo"}   /* AR |wARp| */
   ,{ U"O", U"oo"}   /* AO |thOUGHt,lAW|                                */
   ,{U"aU", U"au"}   /* AW |mOUth,nOW|                                  */
   ,{U"aI", U"ai"}   /* AY |prIce,hIGH,trY|                             */
   ,{U"e@", U"eö"}   /* EA |squARE,fAIR|                                */
   ,{U"eI", U"ei"}   /* EY |fAce,dAy,stEAk|                             */
   ,{ U"e", U"e" }   /* EH |drEss,bEd|                                  */
   ,{ U"3", U"ör"}   /* ER |nURse,stIR,cOURage|                         */
   ,{U"ir", U"iö"}   /* IA |nEAR,hERE,sErious|                          */
   ,{U"i@", U"ia"}   /* EO |mEOw|                                       */
   ,{ U"I", U"i" }   /* IH |Intend,basIc,kIt,bId,hYmn|                  */
   ,{ U"i", U"i" }   /* IY |happY,radIation,glorIous,flEEce,sEA,machIne|*/
   ,{U"0r", U"aa"}   /* ar |ARe */
   ,{ U"0", U"o" }   /* OH |lOt,Odd,wAsh|                               */
   ,{U"oI", U"oi"}   /* OY |chOIce,bOY|                                 */
   ,{U"U@", U"ue"}   /* UA |inflUence,sitUation,annUal,cURE,pOOR,jUry|  */
   ,{ U"U", U"u" }   /* UH |fOOt,gOOd,pUt,stimUlus,edUcate|             */
   ,{ U"u", U"uu"}   /* UW |gOOse,twO,blUE|                             */
   ,{U"Z@", U"sö"}   /* viSIOn  */
   ,{ U"b", U"p" }   /* B |Back,BuBBle,joB|                             */
   ,{U"dZ", U"ts"}   /* JH |JuDGE,aGe,solDIer|                          */
   ,{ U"d", U"d" }   /* D |Day,laDDer,oDD|                              */
   ,{ U"D", U"t" }   /* DH |THis,oTHer,smooTH|                          */
   ,{ U"f", U"v" }   /* F |Fat,coFFee,rouGH,PHysics|                    */
   ,{ U"g", U"k" }   /* G |Get,GiGGle,GHost|                            */
   ,{ U"h", U"h" }   /* HH |Hot,WHole,beHind|                           */
   ,{U"k&", U"khä"}  /* CA |CAt,CAptain| */
   ,{ U"k", U"k" }   /* K |Key,CoCK,sCHool|                             */
   ,{ U"l", U"l" }   /* L |Light,vaLLey,feeL|                           */
   ,{ U"m", U"m" }   /* M |More,haMMer,suM|                             */
   ,{ U"n", U"n" }   /* N |Nice,KNow,fuNNy,suN|                         */
   ,{U"Ng", U"ŋŋ"}   /* NG "coNGratulations" */
   ,{ U"N", U"ŋŋ"}   /* NG |riNG,loNG,thaNks,suNG|                      */
   ,{ U"p", U"p" }   /* P |Pen,coPy,haPPen|                             */
   ,{ U"r", U"r" }   /* R |Right,soRRy,aRRange|                         */
   ,{U"sw", U"sv"}   /* SW |SWap| */
   ,{ U"s", U"s" }   /* S |Soon,CeaSe,SiSter|                           */
   ,{ U"S", U"s" }   /* SH |SHip,Sure,staTIon|                          */
   ,{U"tS", U"ts"}   /* CH |CHurCH,maTCH,naTUre|                        */
   ,{U"ts", U"ts"}   /* TS |TSai|                                       */
   ,{U"tu", U"ty"}   /* TU |TUlip| */
   ,{ U"t", U"t" }   /* T |Tea,TighT,buTTon|                            */
   ,{ U"T", U"t" }   /* TH |THing,auTHor,paTH|                          */
   ,{ U"v", U"v" }   /* V |View,heaVy,moVe|                             */
   ,{ U"w", U"v" }   /* W |Wet,One,When,qUeen|                          */
   ,{ U"j", U"j" }   /* Y |Yet,Use,bEAuty|                              */
   ,{ U"z", U"s" }   /* Z |Zero,Zone,roSeS,buZZ|                        */
   ,{ U"Z", U"s" }   /* ZH |pleaSure,viSIon|                            */
};
    std::cout << "Before: " << to8(result) << '\n';
    for(std::size_t pos=0; pos<result.size(); ++pos)
        for(const auto& row: tab)
        {
            std::u32string_view l = row[0], r = row[1];
            if(result.compare(pos, l.size(), l.data()) == 0)
                { result.replace(pos, l.size(), r); pos += r.size()-1; break; }
        }
    std::cout << "After: " << to8(result) << '\n';
    return result;
}

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
        { U"0", U"zero" },{ U"1", U"one" },
        { U"2", U"two" }, { U"3", U"three" },
        { U"4", U"four" },{ U"5", U"five" },
        { U"6", U"six" }, { U"7", U"seven"},
        { U"8", U"eight"},{ U"9", U"nine"}, { U"c+", U"ceeplus"},
        { U"+", U"plus"}, { U"/", U"slash"},
    };
    for(std::size_t pos = 0; pos < wip.size(); ++pos)
    r1: for(const auto& r: canonizations)
            if(wip.compare(pos, r.first.size(), r.first) == 0)
                { wip.replace(pos, r.first.size(), r.second); pos -= std::min(pos, std::size_t(3)); goto r1; }
    wip = english_convert(U" " + wip + U" ");
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
    for(std::size_t pos = 0; pos < wip.size(); ++pos)
    {
        std::size_t endpos = pos;
        auto rep_len = [&]{ unsigned r=0; for(; wip[endpos+1] == wip[endpos]; ++r) ++endpos; return r; };
        bool surrounded_by_vowel = (wip[pos+1] != wip[pos] && vow(wip[pos+1])) || (pos>0 && vow(wip[pos-1]));
        static const std::map<char32_t, std::initializer_list<std::tuple<char32_t,unsigned,unsigned,unsigned>>> maps
        {
            {U'm', {{U'm',8,9,0},{U'M',2,0,0}}}, {U'a', {{U'a',7,9,3}}}, {U'e', {{U'e',7,9,3}}}, {U'l', {{U'l',7,9,3}}},
            {U'n', {{U'n',8,9,0},{U'N',2,0,0}}}, {U'u', {{U'u',7,9,3}}}, {U'y', {{U'y',7,9,3}}}, {U's', {{U's',7,9,3}}},
            {U'ŋ', {{U'ŋ',8,9,0},{U'Ŋ',2,0,0}}}, {U'v', {{U'v',9,9,0}}}, {U'j', {{U'j',9,9,0}}},
            {U'k', {{U'-',5,9,0},{U'k',4,0,0}}}, {U'i', {{U'i',7,9,3}}}, {U'o', {{U'o',7,9,3}}},
            {U'p', {{U'-',5,9,0},{U'p',4,0,0}}}, {U'ä', {{U'ä',7,9,3}}}, {U'ö', {{U'ö',7,9,3}}},
            {U't', {{U'-',5,9,0},{U't',4,0,0}}}, {U'r', {{U'r',9,9,0}}}, {U'h', {{U'h',9,9,0}}},
            {U'd', {{U'-',5,9,0},{U'd',4,0,0}}}, {U'¯', {{U'-',7,0,0}}}, {U'q', {{U'q',1,0,0}}},
        };
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

    static float coeff[MaxOrder]={0};
    const auto& src_coeff = std::get<1>(frame);
    for(unsigned j=0; j<src_coeff.size(); ++j)
    {
        coeff[j] = src_coeff[j];
    }

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
        if(i == records.end())
            if(i = records.find(U'-'); i == records.end())
                { fprintf(stderr, "Didn't find %d\n", e.record); continue; }

        token_pitch       = std::clamp(token_pitch       + float(drand48() - 0.5), 50.f, 170.f);
        breath_base_level = std::clamp(breath_base_level + float(drand48() - 0.5) * 0.02f, 0.1f, 1.0f);
        voice_base_level  = std::clamp(voice_base_level  + float(drand48() - 0.5) * 0.02f, 0.7f, 1.0f);

        float    frame_time = rate * frame_time_factor;
        const auto& alts    = std::get<2>(i->second);
        unsigned n_samples  = e.n_frames * frame_time;

        float breath = breath_base_level + (1.f-breath_base_level) * (1.f - std::get<1>(i->second));
        float buzz   = std::get<1>(i->second) * voice_base_level;
        float pitch  = e.relative_pitch * token_pitch;

        auto do_synth = [&](const auto& f, unsigned n) { synth(f, volume, breath, buzz, pitch, rate, n); return n; };
        switch(std::get<0>(i->second))
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
                    n_samples -= do_synth(alts[n % alts.size()], std::min(n_samples, unsigned(frame_time * 1.5f)));
        }
    }

    while(audio_pos < audio.size()) { sf::sleep(sf::milliseconds(100)); }
    sf::sleep(sf::milliseconds(1000));
}
