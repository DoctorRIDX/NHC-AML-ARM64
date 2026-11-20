#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

MYMODCFG(net.napoleonhorosscentral.nhc, NHC, 0.1.0, NapoleonHorossCentral)
NEEDGAME(com.rockstargames.gtasa)

ON_MOD_PRELOAD()
{
    logger->SetTag("NHC");
}

ON_MOD_LOAD()
{
    logger->Info("NHC plugin loaded. Runtime compatibility check passed only if this message appears.");
    logger->Info("Player: Infinite Money, Invincibility, Jetpack - planned, not active.");
    logger->Info("Vehicles: Vehicle Spawn - planned, not active.");
    logger->Info("No game memory offsets or unsupported function addresses are assumed.");
}
