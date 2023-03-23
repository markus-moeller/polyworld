#pragma once

#include <string>
#include <vector>

#include "environment/Energy.h"
#include "library_global.h"

// forward decls
class FoodType;

class LIBRARY_SHARED Metabolism {
public:
    static void define(const std::string &name, const EnergyPolarity &energyPolarity,
                       EnergyMultiplier eatMultiplier, const Energy &energyDelta,
                       float minEatAge, const FoodType *carcassFoodType);
	static int getNumberOfDefinitions();
    static Metabolism LIBRARY_SHARED *get(int index);

    enum SelectionMode {
		Gene,
		Random
	};

	static SelectionMode selectionMode;

	const int index;
	const std::string name;
	const EnergyPolarity energyPolarity;
	EnergyMultiplier eatMultiplier;
	Energy energyDelta;
	float minEatAge;
	const FoodType *carcassFoodType;

private:
    Metabolism(int _index,
               const std::string &_name,
               const EnergyPolarity &_energyPolarity,
               EnergyMultiplier _eatMultiplier,
               const Energy &energyDelta,
               float _minEatAge,
               const FoodType *_carcassFoodType);

	static std::vector<Metabolism *> metabolisms;
};
