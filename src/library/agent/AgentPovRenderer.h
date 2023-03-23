#pragma once

#include "utils/Signal.h"
#include "library_global.h"

class LIBRARY_SHARED AgentPovRenderer
{
 protected:
    AgentPovRenderer() {}

 public:
    static AgentPovRenderer *create( int maxAgents,
                                     int retinaWidth,
                                     int retinaHeight );
    virtual ~AgentPovRenderer() {}

	virtual void add( class agent *a ) = 0;
	virtual void remove( class agent *a ) = 0;

	virtual void beginStep() = 0;
	virtual void render( class agent *a ) = 0;
	virtual void endStep() = 0;

    util::Signal<> renderComplete;
};
