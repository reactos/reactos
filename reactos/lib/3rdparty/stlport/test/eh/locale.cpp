#include <exception>
#include <iostream>
#include <locale>
#include <ctime>

using namespace std;

void main()
{
  try
  {
    locale c_loc;
    //locale sys(c_loc, "LC_TIME=UKR_UKR.OCP;LC_NUMERIC=RUS_RUS.OCP;LC_CTYPE=ukr_ukr.ocp;", locale::numeric | locale::time | locale::ctype);
    locale sys(".ocp");
    locale::global(sys);
    cin.imbue(sys);
    cout.imbue(sys);

    cout<<"Locale name is: "<<sys.name().c_str()<<'\n';

    cout<<"Enter real number:";
    double value;
    cin>>value;
    cout<<value<<'\n';

        // Time test.
        long lcur_time;
        time(&lcur_time);
        struct tm* cur_time=localtime(&lcur_time);

        const numpunct<char>& num_punct=use_facet<numpunct<char> >(cout.getloc());
        cout << num_punct.decimal_point() << '\n';
        const time_put<char, ostreambuf_iterator<char, char_traits<char> > >& time_fac=
        use_facet<time_put<char, ostreambuf_iterator<char, char_traits<char> > > >(cout.getloc());
        time_fac.put(cout, cout, NULL, cur_time, 'x'); cout<<'\n';
        time_fac.put(cout, cout, NULL, cur_time, 'x', '#'); cout<<'\n';
        time_fac.put(cout, cout, NULL, cur_time, 'X'); cout<<'\n';
        time_fac.put(cout, cout, NULL, cur_time, 'c'); cout<<'\n';
        time_fac.put(cout, cout, NULL, cur_time, 'c', '#'); cout<<'\n';
        time_fac.put(cout, cout, NULL, cur_time, 'I'); cout<<'\n';

        const ctype<char>& char_type=use_facet<ctype<char> >(cout.getloc());
        if(char_type.is(ctype_base::upper, 'è')) puts("Upper");
        if(char_type.is(ctype_base::lower, 'Ø')) puts("Lower");
        puts("Next");
        if(isupper('è', cout.getloc())) puts("Upper");
        if(islower('Ø', cout.getloc())) puts("Lower");
        /*for(int ch=128; ch<256; ch++)
          printf("Character %c (%d) - upper %c, lower %c\n",(char)ch, ch,toupper((char)ch, cout.getloc()), tolower((char)ch, cout.getloc()));*/
  }
  catch(exception &e)
  {
    cout<<"Exception fired:\n"<<e.what()<<'\n';
  }
  catch(...)
  {
    cout<<"Unknown exception throwed!\n";
  }
  cout.flush();
}
