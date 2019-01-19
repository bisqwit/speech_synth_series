#include <string>
#include <iostream>
#include <vector>
#include <initializer_list>
#include <cstdlib>

int main(int argc, char** argv)
{
    const std::string f    = argv[1];
    const long double fval = std::stold(f);
    const unsigned maxlen  = std::stoi(argv[2]);

    /*
    Try:
        n - val
        n - val*m
        n - val/m
        n - m/val
        val
        val*m
        val/m
        m/val
        val - n
        val*m - n
        val/m - n
        m/val - n
        val + n
        val*m + n
        val/m + n
        m/val + n
    */
    struct result_type
    {
        std::string result_string;
        double      error;
        unsigned    num_operators;
        unsigned    operand_magnitude;

        bool operator< (const result_type& b) const
        {
            if(error != b.error) return error < b.error;
            if(result_string.size() != b.result_string.size()) return result_string.size() < b.result_string.size();
            if(num_operators != b.num_operators) return num_operators < b.num_operators;
            return operand_magnitude < b.operand_magnitude;
        }
    };

    std::vector<long double> muldiv_options;
    for(int n=1; n<1000; ++n)
    {
        std::string mvalue = std::to_string(n);
        if(n > 1 && mvalue.size()+1 > maxlen) break;
        muldiv_options.push_back(n);
    }
    for(int n=1; n<1000; ++n)
    {
        long double m = 1000.l / n;
        long double test = 1.0l / m; if(test==(int)test && test>=1 && test<1000) continue;
        char Buf[64]; std::sprintf(Buf, "%Lg", m); std::string mvalue = Buf;
        if(mvalue.size()+1 > maxlen) continue;
        muldiv_options.push_back(m);
    }

    result_type best = { f, 1e99, 99, 99 };

    #pragma omp declare reduction(isbetter:result_type: \
            (omp_in < omp_out ? omp_out = omp_in : omp_out) \
                                 ) initializer(omp_priv = result_type{"xxxxxxx",1e99,99,99})

    #pragma omp parallel for schedule(dynamic) collapse(4) reduction(isbetter:best)
    for(int arith_mode=0; arith_mode<4; ++arith_mode) // n, n-, -n, +n
    for(int n=0; n<100; ++n)
    for(unsigned digits=0; digits<=maxlen; ++digits)
    for(char fmtchar='d'; fmtchar<='f'; ++fmtchar)
    {
        if((arith_mode==0) != (n==0)) continue;

        std::string arith_string;
        switch(arith_mode)
        {
            case 0: break;
            case 1: arith_string = std::to_string(n) + '-'; break;
            case 2: arith_string = '-' + std::to_string(n); break;
            case 3: arith_string = '+' + std::to_string(n); break;
        }

        if(digits + arith_string.size() > maxlen) continue;

        for(long double m: muldiv_options)
        {
            char Buf[64]; std::sprintf(Buf, "%Lg", m); std::string mvalue = Buf;

            for(int muldiv_mode=0; muldiv_mode<4; ++muldiv_mode)
            {
                if( (muldiv_mode==0) != (m==1)) continue;

                std::string muldiv_string;
                switch(muldiv_mode)
                {
                    case 0: break;
                    case 1: muldiv_string = '*' + mvalue; break;
                    case 2: muldiv_string = '/' + mvalue; break;
                    case 3: muldiv_string = mvalue + '/'; break;
                }

                if(digits + arith_string.size() + muldiv_string.size() > maxlen) continue;

                long double testval = fval;
                switch(arith_mode*4 + muldiv_mode)
                {
                    case  0: testval = fval;   break;  // val  =x    val=x
                    case  1: testval = fval/m; break;  // val*m=x    val=x/m
                    case  2: testval = fval*m; break;  // val/m=x    val=x*m
                    case  3: testval = m/fval; break;  // m/val=x    val=m/x
                    case  4: testval = n-fval; break;  // n-val  =x     val=n-x
                    case  5: testval = (n-fval)/m; break; // n-val*m=x     val=(n-x)/m
                    case  6: testval = (n-fval)*m; break; // n-val/m=x     val=(n-x)*m
                    case  7: testval = m/(n-fval); break; // n-m/val=x     n=x+m/val   n-x=m/val val*(n-x)=m  val=m/(n-x)
                    case  8: testval = fval+n; break; // val-n  =x   val=x+n
                    case  9: testval = (fval+n)/m; break; // val*m-n=x   val=(x+n)/m
                    case 10: testval = (fval+n)*m; break; // val/m-n=x   val=(x+n)*m
                    case 11: testval = m/(fval+n); break; // m/val-n=x   m/val = x+n   m = (x+n)*val  val = m/(x+n)
                    case 12: testval = fval-n; break; // val+n  =x
                    case 13: testval = (fval-n)/m; break; // val*m+n=x
                    case 14: testval = (fval-n)*m; break; // val/m+n=x
                    case 15: testval = m/(fval-n); break; // m/val+n=x
                }
                char Buf[64], fmt[64];
                std::sprintf(fmt, "%%.%uL%c", digits, fmtchar=='d'?'a':fmtchar);
                std::sprintf(Buf, fmt,      testval);
                std::string elem = Buf;
                //if(!n && m==1) fprintf(stderr, "tried %s\n", Buf);
                // Change 0. into .
                if(elem[0] == '0' && elem[1] == '.') elem.erase(0,1);
                else if(elem[0] == '-' && elem[1] == '0' && elem[2] == '.') elem.erase(1,1);
                // Change E+ or e+ into e
                bool integer = true;
                std::size_t point_pos = std::string::npos;
                for(std::size_t p=0; p<elem.size(); ++p)
                {
                    if(elem[p]=='.') { integer = false; point_pos = p; }
                    if(elem[p]=='e' || elem[p]=='E')
                    {
                        integer = false;
                        if(elem[p+1]=='+')      { if(elem[p+2]=='0' && elem[p+3]>='0' && elem[p+3]<='9') elem.erase(p+1, 2); else elem.erase(p+1,1); }
                        else if(elem[p+1]=='-') { if(elem[p+2]=='0' && elem[p+3]>='0' && elem[p+3]<='9') elem.erase(p+2, 1); }

                        int exponent = std::atoi(elem.c_str()+p+1);
                        if(point_pos != std::string::npos)
                        {
                            // Change 4.4e-6 into 44e-7
                            int num_decimals = p - point_pos - 1;
                            std::string next_exponent = std::to_string(exponent-1);
                            while(num_decimals > 0 && (elem.size()-(p+1)) == next_exponent.size())
                            {
                                //fprintf(stderr, "Changed '%s' ", elem.c_str());
                                std::swap(elem[point_pos], elem[point_pos+1]);
                                elem.replace(p+1, elem.size()-(p+1), next_exponent);
                                --exponent;
                                ++point_pos;
                                --num_decimals;
                                //fprintf(stderr, " to '%s', decimals remain %d, exponent=%d\n", elem.c_str(), num_decimals,exponent);
                                next_exponent = std::to_string(exponent-1);
                            }
                            // Delete decimal point right before 'e'
                            if(!num_decimals) elem.erase(p-1,1);
                        }
                        break;
                    }
                    if(elem[p]=='p' || elem[p]=='P')
                    {
                        integer = false;
                        if(elem[p+1]=='+')      { elem.erase(p+1,1); }
                        break;
                    }
                }
                if(elem.size() > maxlen) continue;
                //fprintf(stderr, "elem=%s with muldiv=%u %g, fval=%Lg, testval=%Lg\n", elem.c_str(), muldiv_mode,m, fval, testval);
                // Now create the formula.
                double evaluated = std::stod(elem), mm = m;
                if(muldiv_mode >= 2) // divisions
                {
                    // make sure either m or elem contains decimal point
                    if(integer && mvalue.find('.')==mvalue.npos) elem += '.';
                }
                switch(muldiv_mode)
                {
                    case 0: evaluated = evaluated;    break;
                    case 1: evaluated = evaluated*mm; elem += muldiv_string; break;
                    case 2: evaluated = evaluated/mm; elem += muldiv_string; break;
                    case 3: evaluated = mm/evaluated; elem.insert(0, muldiv_string); break;
                }
                switch(arith_mode)
                {
                    case 0: evaluated = evaluated; break;
                    case 1: evaluated = n-evaluated; elem.insert(0, arith_string); break;
                    case 2: evaluated = evaluated-n; elem += arith_string; break;
                    case 3: evaluated = evaluated+n; elem += arith_string; break;
                }
                /*fprintf(stderr, "Testing %s = %g (fmt=%s, testval=%Lg, arith=%u:%d, mul=%u:%g)\n",
                    elem.c_str(), evaluated,
                    fmt, testval,
                    arith_mode,n,
                    muldiv_mode,m);*/
                if(elem.size() > maxlen) continue;
                // Compare error
                result_type r{ elem, std::abs(evaluated - double(fval)),
                               unsigned(arith_mode!=0) + unsigned(muldiv_mode!=0),
                               unsigned(n+10*m) };
                if(r < best) best = r;
            }
        }
    }
    std::cout << best.result_string;
}

