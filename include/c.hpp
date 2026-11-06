#pragma once
#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig) {
    CONFIG_VALUE(EnableMenuMusic, bool, "Enable Menu Music", true);
    CONFIG_VALUE(EnableTurkeyGobbles, bool, "Enable Turkey Gobbles", true);
    CONFIG_VALUE(EnableAutumnLeaves, bool, "Enable Falling Autumn Leaves", true);
    CONFIG_VALUE(EnableHauntedEnvColors, bool, "Enable Thanksgiving Harvest Lighting", true);
    CONFIG_VALUE(EnablePumpkinAccents, bool, "Enable Thanksgiving Pumpkin Accents", true);
    // CONFIG_VALUE(EnableBlocks, bool, "Enable Thanksgiving blocks", true);

};