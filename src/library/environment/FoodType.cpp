#include "FoodType.h"

#include <assert.h>

#include "utils/misc.h"

std::map<std::string, const FoodType *> FoodType::foodTypes;
std::vector<FoodType *> FoodType::foodTypesVector;


FoodType::FoodType( int _index,
                    std::string _name,
					Color _color,
					EnergyPolarity _energyPolarity,
					EnergyMultiplier _eatMultiplier,
					Energy _depletionThreshold )
	: index( _index)
	, name( _name )
	, color( _color )
	, energyPolarity( _energyPolarity )
	, eatMultiplier( _eatMultiplier )
	, depletionThreshold( _depletionThreshold )
{
}

void FoodType::define( std::string _name,
					   Color _color,
					   EnergyPolarity _energyPolarity,
					   EnergyMultiplier _eatMultiplier,
					   Energy _depletionThreshold )
{
	FoodType *foodType = new FoodType( foodTypesVector.size(),
									   _name,
									   _color,
									   _energyPolarity,
									   _eatMultiplier,
									   _depletionThreshold );
	foodTypes[_name] = foodType;
	foodTypesVector.push_back( foodType );
}

const FoodType *FoodType::lookup( std::string name )
{
	return foodTypes[name];
}

const FoodType *FoodType::find( const EnergyPolarity &polarity )
{
	itfor( FoodTypeMap, foodTypes, it )
	{
		if( it->second->energyPolarity == polarity )
		{
			return it->second;
		}
	}

	return NULL;
}

FoodType *FoodType::get( int index )
{
	return foodTypesVector[index];
}

int FoodType::getNumberDefinitions() {
	return (int)foodTypes.size();
}
