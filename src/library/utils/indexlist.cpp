/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// indexlist.cp: declaration & implementation of indexlist classes

// System
#include <stdio.h>

// Local
#include "error.h"
#include "misc.h"

// Self
#include "indexlist.h"

indexlist::indexlist(long l, long h)
{
    if (l < h)
    {
    	lo = l;
    	hi = h;
	}
	else
	{
		lo = h;
		hi = l;
	}
    
    numbytes = (hi - lo + 8) >> 3;  // >> 3 == / 8
    pind = new unsigned char[numbytes];
    if (pind == NULL)
    	error(2, "Unable to allocate memory for new indexlist; program terminating");
    	
    for (long i = 0; i < numbytes; i++)
        pind[i] = 0;
        
    next = lo;
}


indexlist::~indexlist()
{
    if (pind != NULL)
    	delete pind;
}


void indexlist::dump(std::ostream& out)
{
    out << lo sp hi sp next sp numbytes nl;
    
    if (pind == NULL)
    {
        error(1,"dumping an indexlist with no index storage");
        return;
    }
    
    for (int i = 0; i < numbytes; i++)
		out << (int)(pind[i]) nl;
}


void indexlist::load(std::istream& in)
{
    long newbytes;
    in >> lo >> hi >> next >> newbytes;
    if (pind != NULL && (newbytes != numbytes))
    {
        char msg[256];
        sprintf(msg,
        		"numbytes in indexlist load (%ld) not equal to existing numbytes (%ld)",
           		newbytes,
           		numbytes);
        error(1,msg);
        delete pind;
        pind = NULL;
    }
    
    if (pind == NULL)
    {
        numbytes = newbytes;
        pind = new unsigned char[numbytes];
        if (pind == NULL)
			error(2,"Insufficient memory for indexlist during load");
    }
    
    int num = 0;
    for (int i = 0; i < numbytes; i++)
    {
        in >> num;
        pind[i] = (unsigned char)num;
    }
}


long indexlist::getindex()
{
    if (next < lo)  // no more slots left
        return next;
        
    long xbyte = (next - lo) >> 3;  // 0-based  ( >> 3 == / 8 )
    unsigned char bit = (unsigned char)((next - lo) % 8);//0-based,from left
    pind[xbyte] |= 1 << (7 - bit);
    long curr = next;
    
    // following is < hi, not <= hi, because we don't want to wrap back
    // around to curr.
    for (long i = lo; i < hi; i++)  // try next, but search entire list
    {
        next++;
        if (next > hi)
        	next = lo;
        	
        long xbyte = (next - lo) >> 3;  // 0-based  ( >> 3 == / 8 )
        unsigned char bit = (unsigned char)((next - lo) % 8);//0-based,from left
        if (!((pind[xbyte] >> (7 - bit)) & 1))  // found an available slot
        {
            return curr;
        }
    }
    
    next = lo - 1; // search failed
    return curr;
}


void indexlist::freeindex(long i)
{
    if ((i < lo) || (i > hi))
    {
        char msg[256];
        sprintf(msg, "Attempt to free out-of-range index: %ld not in [%ld, %ld]", i, lo, hi);
        error(1, msg);
    }
    else
    {
        long xbyte = (i - lo) >> 3;  // 0-based  ( >> 3 == / 8 )
        unsigned char bit = (unsigned char)((i - lo) % 8); //0-based, from left
        pind[xbyte] &= ~(1 << (7 - bit));
        if ( (i < next) || (next < lo) )
            next = i;
    }
}


bool indexlist::isone(long i)
{
    if ((i < lo) || (i > hi))
    {
        char msg[256];
        sprintf(msg, "isone request for out-of-range index: %ld not in [%ld, %ld]", i, lo, hi);
        error(1, msg);
    }
    else
    {
        long xbyte = (i - lo) >> 3;  // 0-based  ( >> 3 == / 8 )
        unsigned char bit = (unsigned char)((i - lo) % 8); //0-based, from left
        return (bool)((pind[xbyte] >> (7 - bit)) & 1);
    }
    
    return false;
}


void indexlist::print(long loindex, long hiindex)
{
    std::cout << "indexlist bits " << loindex << " through " << hiindex << " =" nl;
    for (long i = loindex; i <= hiindex; i++)
    {
        long xbyte = (i-lo) >> 3; // 0-based
        long bit = (i-lo) % 8; // 0-based,from left
        std::cout << ((pind[xbyte] >> (7-bit)) & 1);
    }
    std::cout nlf;
}

