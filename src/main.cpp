// NHC - NAPOLEON HORORSS-CENTER: AML plugin entry point (arm64-v8a).
#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>

#include "nhc/core.h"
#include "nhc/platform.h"

// Only this translation unit may include the AML headers: interface.h defines free functions, and a
// second TU including it could cause duplicate symbols at link time. platform.cpp is therefore pulled
// in here instead of being compiled on its own (it is not listed in Android.mk).
#include "platform.cpp"

#if !defined(IAML_VER) || IAML_VER < 01040000
#error "NHC needs the AML 1.4.0 mod headers (the GitHub Actions workflow copies them into include/mod/)."
#endif

// The config file is nhc.ini in AML's config directory; it holds [Settings], [Build] and [Offsets_*].
MYMODCFGNAME(net.nhc.napoleonhororsscenter, NHC, 1.0.0, NHC, nhc)
NEEDGAME(com.rockstargames.gtasa)

BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.4.0)
END_DEPLIST()

ON_MOD_PRELOAD()
{
    logger->SetTag("NHC");
    nhc::plat::BindAml(aml, cfg, logger);
}

ON_MOD_LOAD()
{
    nhc::Start();
}

ON_MOD_UNLOAD()
{
    nhc::Stop();
}
