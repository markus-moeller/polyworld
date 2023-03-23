
// Self
#include "grect.h"

// Local
#include "utils/misc.h"

//===========================================================================
// grect
//===========================================================================

grect::grect()
{
	init();
}


grect::grect(char* n)
	:	gobject(n)
{
	init();
}


grect::grect(float lx, float ly)
{
	setrect(lx,ly);
}


grect::grect(char* n, float lx, float ly)
	:	gobject(n)
{
	setrect(lx,ly);
}


grect::grect(float xa, float ya, float xb, float yb)
{
	setrect(xa,ya,xb,yb);
}
 
 
grect::grect(char* n, float xa, float ya, float xb, float yb)
	:	gobject(n)
{
	setrect(xa, ya, xb, yb);
}


void grect::init()
{
	fLengthX = fLengthY = 1.0;
	fRadiusFixed = false;
	fRadiusScale = 1.0;
}


void grect::setrect(float lx, float ly)
{
	fLengthX = lx;
	fLengthY = ly;
	setradius();
}


void grect::setrect(float xa, float ya, float xb, float yb)
{
	float xp;
	float yp;
	
	if (xa < xb)
	{
		xp = xa;
		fLengthX = xb - xa;
	}
	else
	{
		xp = xb;
		fLengthX = xa - xb;
	}
	
	if (ya < yb)
	{
		yp = ya;
		fLengthY = yb - ya;
	}
	else
	{
		yp = yb;
		fLengthY = ya - yb;
	}
	
	settranslation(xp,yp);
	setradius();
}
    
    
void grect::draw()
{
	glColor3fv(&fColor[0]);
	
    glPushMatrix();
		position();		
		glScalef(fScale, fScale, fScale);
      	if (fFilled)
			glRectf(0., 0., fLengthX, fLengthY); // [TODO] fill it
      	else
			glRectf(0., 0., fLengthX, fLengthY);
    glPopMatrix();
}


void grect::print()
{
    gobject::print();
    std::cout << "  fLengthX = " << fLengthX nl;
    std::cout << "  fLengthY = " << fLengthY nl;
    if (fFilled)
        std::cout << "  it is fFilled" nl;
    else
        std::cout << "  it is not fFilled" nl;
    std::cout.flush();
}

