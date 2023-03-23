#include "Resources.h"

#include <string.h>

#include "error.h"
#include "misc.h"
#include "graphics/gpolygon.h"
#include "proplib/proplib.h"

#if !__WIN64__ && !__WIN32__
static const char *RPATH[] = {"./Polyworld.app/Contents/Resources/",
							  "./etc/objects/",
							  "./",
							  NULL};
#else
static const char *RPATH[] = {"./etc/objects/",
                              "./",
                              NULL};
#endif

//===========================================================================
// Resources
//===========================================================================

//---------------------------------------------------------------------------
// Resources::loadPolygons()
//---------------------------------------------------------------------------

bool Resources::loadPolygons( gpolyobj *poly,
                              std::string name )
{
    std::string path = find(name + ".obj");
	if( path == "" )
	{
		error(1, "Failed finding polygon object ", name.c_str());
		return false;
	}

	path.c_str() >> *poly;

	return true;
}

//---------------------------------------------------------------------------
// Resources::getInterpreterScript()
//---------------------------------------------------------------------------

std::string Resources::getInterpreterScript() {
#if __WIN32__ || __WIN64__
    return find("src/library/proplib/interpreter.py");
#else
    return find("src/library/proplib/interpreter.py");
#endif
}

//---------------------------------------------------------------------------
// Resources::find()
//---------------------------------------------------------------------------

std::string Resources::find( std::string name ) {
    for ( const char **path = RPATH; *path; path++ ) {

        char buf[1024];

		strcpy(buf, *path);
		strcat(buf, name.c_str());

        if( exists(buf) ) {
			return buf;
		}
	}

	return "";
}
