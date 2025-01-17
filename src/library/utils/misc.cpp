/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// misc.cp: miscellaneous useful short procedures

// Self
#include "misc.h"

// System

#ifdef __APPLE__
	#include <mach/mach_time.h>
#endif

#include <math.h>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#if !__WIN64__ && !__WIN32__
#include <sys/resource.h>
#else
#include "resource.h"
#endif
#include <set>
#include <string>
#include <vector>
#include <iostream>

#ifdef CORE_UTILS
#define UTILS_PATH CORE_UTILS"\\"
#else
#define UTILS_PATH ""
#endif

// https://en.wikipedia.org/wiki/Marsaglia_polar_method
double nrand()
{
    static bool spare = false;
    static double u, v, s, c;
    if (spare)
    {
        spare = false;
        return c * v;
    }
    do
    {
        u = 2.0 * randpw() - 1.0;
        v = 2.0 * randpw() - 1.0;
        s = u * u + v * v;
    } while (s == 0.0 || s >= 1.0);
    c = sqrt(-2.0 * log(s) / s);
    spare = true;
    return c * u;
}

double nrand(double mean, double stdev)
{
    return mean + nrand() * stdev;
}

double trand(double min, double max)
{
    double range = max - min;
    return min + sqrt(randpw() * range * range);
}

char* concat(const char* s1, const char* s2)
{
    char* s = new char[strlen(s1)+strlen(s2)+1];
    strcpy(s,s1);
    strcat(s,s2);
    return s;
}


char* concat(const char* s1, const char* s2, const char* s3)
{
    char* s = new char[strlen(s1)+strlen(s2)+strlen(s3)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    return s;
}


char* concat(const char* s1, const char* s2, const char* s3, const char* s4)
{
    char* s = new char[strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    strcat(s,s4);
    return s;
}


char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5)
{
    char* s= new char[strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+strlen(s5)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    strcat(s,s4);
    strcat(s,s5);
    return s;
}

char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5, const char* s6)
{
    char* s= new char[strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+strlen(s5)+
                      strlen(s6)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    strcat(s,s4);
    strcat(s,s5);
    strcat(s,s6);
    return s;
}


char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5, const char* s6, const char* s7)
{
    char* s= new char[strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+strlen(s5)+
                      strlen(s6)+strlen(s7)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    strcat(s,s4);
    strcat(s,s5);
    strcat(s,s6);
    strcat(s,s7);
    return s;
}


std::string operator+( const char *a, const std::string &b )
{
    return std::string(a) + b;
}

char* itoa(long i)
{
    char* b = new char[256];
    sprintf(b,"%ld", i);
    char* a = new char[strlen(b)+1];
    strcpy(a, b);
    delete[] b;
    return a;
}


char* ftoa(float f)
{
    char* b = new char[256];
    sprintf(b,"%g", f);
    char* a = new char[strlen(b)+1];
    strcpy(a ,b);
    delete[] b;
    return a;
}


double logistic(double x, double slope)
{
    return (1.0 / (1.0 + exp(-1 * x * slope)));
}

double biasedLogistic(double x, double midpoint, double slope)
{
    return (1.0 / (1.0 + exp(-1 * (x-midpoint) * slope)));
}

double generalLogistic(double x, double midpoint, double slope, double yneg, double ypos)
{
    return yneg + (ypos - yneg) * biasedLogistic(x, midpoint, slope);
}


double gaussian( double x, double mean, double variance )
{
    return( exp( -(x-mean)*(x-mean) / variance ) );
}

bool exists( const std::string &path )
{
	struct stat _stat;
	return 0 == stat(path.c_str(), &_stat);
}

// TODO: Pfadtrenner beachten
std::string dirname( const std::string &path )
{
    size_t end = path.length() - 1;

    if( path[end] == '/' )
        end--;

    end = path.rfind( '/', end );

    return path.substr( 0, end );
}

void makeDirs( const std::string &path )
{
    static std::set<std::string> alreadyMade;
    static std::mutex alreadyMade_mutex;

	{
        std::lock_guard<std::mutex> lock(alreadyMade_mutex);

		if( alreadyMade.find(path) == alreadyMade.end() )
		{
			char cmd[1024];
            sprintf(cmd, "%smkdir -p %s", UTILS_PATH, path.c_str());
            if (system(cmd) != 0) {
                fprintf(stderr, "Failed executing command '%s'\n", cmd);
                exit(1);
            }

            alreadyMade.insert(path);
		}
	}
}

void makeParentDir( const std::string &path )
{
	makeDirs( dirname(path) );
}

int SetMaximumFiles( long filecount )
{
    struct rlimit lim;

	lim.rlim_cur = lim.rlim_max = (rlim_t) filecount;
	if( setrlimit( RLIMIT_NOFILE, &lim ) == 0 )
		return 0;
	else
		return errno;
}

int GetMaximumFiles( long *filecount )
{
	struct rlimit lim;

	if( getrlimit( RLIMIT_NOFILE, &lim ) == 0 )
	{
		*filecount = (long) lim.rlim_max;
		return 0;
	}
	else
		return errno;
}


std::vector<std::string> split( const std::string& str, const std::string& delimiters )
{
    std::vector<std::string> parts;

    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of( delimiters, 0 );
    // Find first delimiter after non-delimiters.
    std::string::size_type pos     = str.find_first_of( delimiters, lastPos );

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        parts.push_back( str.substr( lastPos, pos - lastPos ) );
        // Skip delimiters.
        lastPos = str.find_first_not_of( delimiters, pos );
        // Find next delimiter after non-delimiters
        pos = str.find_first_of( delimiters, lastPos );
    }

    return( parts );
}

// int main( int argc, char** argv )
// {
// 	string sentence( "Now is the time for all good men" );
//
// 	vector<string> parts = split( sentence );
//
// 	itfor( vector<string>, parts, it )
// 	{
// 		cout << *it << endl;
// 	}
// }
