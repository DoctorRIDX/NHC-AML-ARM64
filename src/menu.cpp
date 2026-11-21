#include "nhc/menu.h"
#include "nhc/core.h"
#include "nhc/features.h"
#include "nhc/platform.h"
#include "nhc/ui_dex.h"

#include <jni.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace nhc {
namespace menu {
namespace {

std::atomic<bool> g_started{false};

jstring JStr(JNIEnv* env, const std::string& s) { return env->NewStringUTF(s.c_str()); }

bool ValidFeature(jint f) { return f >= 0 && f < F_COUNT; }

// ---- natives called by com.nhc.menu.NHCMenu (always on the Android UI thread) ----

jstring JNICALL N_About(JNIEnv* env, jclass) {
    std::string s = "NHC - NAPOLEON HORORSS-CENTER\nVersion 1.0.0\n\n";
    s += Compat().Summary();
    s += "\nFeature status:\n";
    static const char* const names[F_COUNT] = {"Infinite Money", "Invincibility", "Jetpack", "Vehicle Spawn", "Weapon Selector"};
    for (int f = 0; f < F_COUNT; f++) {
        s += std::string("- ") + names[f] + ": ";
        s += FeatureAvailable(static_cast<Feature>(f)) ? "ready (not tested in game)" : "NOT IMPLEMENTED";
        s += "\n";
    }
    s += "\nFull report: NHC_diagnostics.txt in the game's Android/data folder (files/).\n";
    return JStr(env, s);
}

jboolean JNICALL N_Available(JNIEnv*, jclass, jint f) {
    return ValidFeature(f) && FeatureAvailable(static_cast<Feature>(f)) ? JNI_TRUE : JNI_FALSE;
}

jstring JNICALL N_Reason(JNIEnv* env, jclass, jint f) {
    return JStr(env, ValidFeature(f) ? FeatureReason(static_cast<Feature>(f)) : std::string());
}

jboolean JNICALL N_Enabled(JNIEnv*, jclass, jint f) {
    return ValidFeature(f) && FeatureEnabled(static_cast<Feature>(f)) ? JNI_TRUE : JNI_FALSE;
}

void JNICALL N_SetEnabled(JNIEnv*, jclass, jint f, jboolean on) {
    if (ValidFeature(f)) SetFeatureEnabled(static_cast<Feature>(f), on == JNI_TRUE);
}

void JNICALL N_AddMoneyOnce(JNIEnv*, jclass) {
    Command c; c.type = Command::AddMoney; Enqueue(c);
}

void JNICALL N_Jetpack(JNIEnv*, jclass) {
    Command c; c.type = Command::Jetpack; Enqueue(c);
}

void JNICALL N_SpawnVehicle(JNIEnv*, jclass, jint model) {
    Command c; c.type = Command::SpawnVehicle; c.a = model; Enqueue(c);
}

void JNICALL N_GiveWeapon(JNIEnv*, jclass, jint weapon, jint ammo) {
    Command c; c.type = Command::GiveWeapon; c.a = weapon; c.b = ammo; Enqueue(c);
}

jobjectArray MakeStringArray(JNIEnv* env, const std::vector<std::string>& items) {
    jclass strCls = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(static_cast<jsize>(items.size()), strCls, nullptr);
    for (size_t i = 0; i < items.size(); i++) {
        jstring s = JStr(env, items[i]);
        env->SetObjectArrayElement(arr, static_cast<jsize>(i), s);
        env->DeleteLocalRef(s);
    }
    env->DeleteLocalRef(strCls);
    return arr;
}

jobjectArray JNICALL N_Vehicles(JNIEnv* env, jclass) {
    std::vector<std::string> items;
    for (const CatalogItem& it : VehicleCatalog()) items.push_back(std::to_string(it.id) + "|" + it.name);
    return MakeStringArray(env, items);
}

jobjectArray JNICALL N_Weapons(JNIEnv* env, jclass) {
    std::vector<std::string> items;
    for (const CatalogItem& it : WeaponCatalog())
        items.push_back(std::to_string(it.id) + "|" + it.name + "|" + std::to_string(it.defaultAmmo));
    return MakeStringArray(env, items);
}

jint JNICALL N_GetSetting(JNIEnv* env, jclass, jstring key, jint def) {
    const char* k = env->GetStringUTFChars(key, nullptr);
    if (!k) return def;
    jint v = settings::Get(k, def);
    env->ReleaseStringUTFChars(key, k);
    return v;
}

void JNICALL N_SetSetting(JNIEnv* env, jclass, jstring key, jint value) {
    const char* k = env->GetStringUTFChars(key, nullptr);
    if (!k) return;
    settings::Set(k, value);
    env->ReleaseStringUTFChars(key, k);
}

jstring JNICALL N_LastResult(JNIEnv* env, jclass) { return JStr(env, LastResult()); }

const JNINativeMethod kNatives[] = {
    {"nAbout", "()Ljava/lang/String;", reinterpret_cast<void*>(N_About)},
    {"nAvailable", "(I)Z", reinterpret_cast<void*>(N_Available)},
    {"nReason", "(I)Ljava/lang/String;", reinterpret_cast<void*>(N_Reason)},
    {"nEnabled", "(I)Z", reinterpret_cast<void*>(N_Enabled)},
    {"nSetEnabled", "(IZ)V", reinterpret_cast<void*>(N_SetEnabled)},
    {"nAddMoneyOnce", "()V", reinterpret_cast<void*>(N_AddMoneyOnce)},
    {"nActivateJetpack", "()V", reinterpret_cast<void*>(N_Jetpack)},
    {"nSpawnVehicle", "(I)V", reinterpret_cast<void*>(N_SpawnVehicle)},
    {"nGiveWeapon", "(II)V", reinterpret_cast<void*>(N_GiveWeapon)},
    {"nVehicles", "()[Ljava/lang/String;", reinterpret_cast<void*>(N_Vehicles)},
    {"nWeapons", "()[Ljava/lang/String;", reinterpret_cast<void*>(N_Weapons)},
    {"nGetSetting", "(Ljava/lang/String;I)I", reinterpret_cast<void*>(N_GetSetting)},
    {"nSetSetting", "(Ljava/lang/String;I)V", reinterpret_cast<void*>(N_SetSetting)},
    {"nLastResult", "()Ljava/lang/String;", reinterpret_cast<void*>(N_LastResult)},
};

// Returns true when no Java exception is pending; otherwise logs and clears it.
bool Ok(JNIEnv* env, const char* step) {
    if (!env->ExceptionCheck()) return true;
    plat::Log(plat::LOG_ERROR, "Menu: Java exception during '%s'", step);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
}

bool LoadAndStart(JNIEnv* env) {
    jobject activity = static_cast<jobject>(plat::ActivityObject());
    if (!activity) { plat::Log(plat::LOG_ERROR, "Menu: no Activity"); return false; }

    jclass actCls = env->GetObjectClass(activity);
    jmethodID getLoader = env->GetMethodID(actCls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject parent = env->CallObjectMethod(activity, getLoader);
    if (!Ok(env, "getClassLoader")) return false;

    // The DEX lives in this library's read-only data; InMemoryDexClassLoader (API 26+) reads it directly.
    jobject buffer = env->NewDirectByteBuffer(const_cast<unsigned char*>(UiDexData()), static_cast<jlong>(UiDexSize()));
    jclass imCls = env->FindClass("dalvik/system/InMemoryDexClassLoader");
    if (!Ok(env, "find InMemoryDexClassLoader")) return false;
    jmethodID ctor = env->GetMethodID(imCls, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    jobject loader = env->NewObject(imCls, ctor, buffer, parent);
    if (!Ok(env, "new InMemoryDexClassLoader") || !loader) return false;

    jclass clCls = env->FindClass("java/lang/ClassLoader");
    jmethodID loadClass = env->GetMethodID(clCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring name = env->NewStringUTF("com.nhc.menu.NHCMenu");
    jclass menuCls = static_cast<jclass>(env->CallObjectMethod(loader, loadClass, name));
    if (!Ok(env, "loadClass com.nhc.menu.NHCMenu") || !menuCls) return false;

    if (env->RegisterNatives(menuCls, kNatives, sizeof(kNatives) / sizeof(kNatives[0])) != JNI_OK) {
        Ok(env, "RegisterNatives");
        plat::Log(plat::LOG_ERROR, "Menu: RegisterNatives failed");
        return false;
    }
    jmethodID start = env->GetStaticMethodID(menuCls, "start", "(Landroid/app/Activity;)V");
    if (!Ok(env, "find NHCMenu.start") || !start) return false;
    env->CallStaticVoidMethod(menuCls, start, activity);
    if (!Ok(env, "NHCMenu.start")) return false;

    plat::Log(plat::LOG_INFO, "Menu: UI started");
    return true;
}

void UiThreadBootstrap(void*) {
    JavaVM* vm = static_cast<JavaVM*>(plat::JavaVmPtr());
    if (!vm) { plat::Log(plat::LOG_ERROR, "Menu: no JavaVM"); return; }
    JNIEnv* env = nullptr;
    bool attached = false;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            plat::Log(plat::LOG_ERROR, "Menu: cannot attach UI thread");
            return;
        }
        attached = true;
    }
    LoadAndStart(env);
    if (attached) vm->DetachCurrentThread();
}

} // namespace

void StartAsync() {
    if (g_started.exchange(true)) return;
    std::thread([] {
        for (int i = 0; i < 180; i++) {   // wait up to ~90 s for the Activity
            if (plat::JavaVmPtr() && plat::ActivityObject() && plat::RunOnJavaUiThread(&UiThreadBootstrap, nullptr))
                return;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        plat::Log(plat::LOG_ERROR, "Menu: Activity not available after 90 s, UI disabled");
    }).detach();
}

} // namespace menu
} // namespace nhc
