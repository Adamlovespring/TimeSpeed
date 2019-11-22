#include <stdio.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <android/log.h>
#include <jni.h>
#include <sys/time.h>
#include <stdlib.h>


#define log(...) __android_log_print(ANDROID_LOG_INFO, "Inject Log", __VA_ARGS__);

extern "C" {
//这两个函数用于存放获取的真实的函数指针
int (*gettimeofday_orig)(struct timeval *tv, struct timezone *tz);
int (*clock_gettime_orig)(clockid_t clk_id, struct timespec *tp);

//UTC时间加速速率
float utc_time_velocity = 1;
//开机时间加速速率
float clock_time_velocity = 1;

//加速后的时间
uint64_t g_gettimeofday_saved_usecs = 0;
//真实的时间
uint64_t g_gettimeofday_last_real_usecs = 0;

uint64_t g_clock_gettime_saved_usecs = 0;
uint64_t g_clock_gettime_last_real_usecs = 0;

//local function
int gettimeofday_local(struct timeval *tv, struct timezone *tz) {
    int64_t diff;
    uint64_t cur_usecs;
    uint64_t ret_usecs;
    int ret = gettimeofday_orig(tv, tz);
    if (0 != ret) {
        return ret;
    }
    if (g_gettimeofday_saved_usecs == 0) {
        g_gettimeofday_saved_usecs = (uint64_t) ((tv->tv_sec * 1000000LL) + tv->tv_usec);
    } else {
        cur_usecs = (uint64_t) ((tv->tv_sec * 1000000LL) + tv->tv_usec);
        diff = cur_usecs - g_gettimeofday_last_real_usecs;
        diff = (int64_t) (diff * utc_time_velocity);
        ret_usecs = g_gettimeofday_saved_usecs + diff;
        tv->tv_sec = (time_t) (ret_usecs / 1000000LL);
        tv->tv_usec = (suseconds_t) (ret_usecs % 1000000LL);

        g_gettimeofday_last_real_usecs = cur_usecs;
        g_gettimeofday_saved_usecs = ret_usecs;
    }
    return ret;
}

int clock_gettime_local(clockid_t clk_id, struct timespec *tp) {
    int ret = clock_gettime_orig(clk_id, tp);

    if (ret != 0 || clk_id != CLOCK_MONOTONIC) {
        return ret;
    }

    int64_t diff;
    uint64_t cur_usecs;
    uint64_t ret_nsec;
    if (g_clock_gettime_saved_usecs == 0 && g_clock_gettime_last_real_usecs == 0) {
        g_clock_gettime_saved_usecs = (uint64_t) ((tp->tv_sec * 1000000000LL) + tp->tv_nsec);
        g_clock_gettime_last_real_usecs = g_clock_gettime_saved_usecs;
    } else {
        cur_usecs = (uint64_t) ((tp->tv_sec * 1000000000LL) + tp->tv_nsec);
        diff = cur_usecs - g_clock_gettime_last_real_usecs;
        diff = (int64_t) (diff * clock_time_velocity);
        ret_nsec = g_clock_gettime_saved_usecs + diff;

        tp->tv_sec = (time_t) (ret_nsec / 1000000000LL);
        tp->tv_nsec = (suseconds_t) (ret_nsec % 1000000000LL);

        g_clock_gettime_last_real_usecs = cur_usecs;
        g_clock_gettime_saved_usecs = ret_nsec;
    }
    return ret;
}

void *hook_params_utc_time[4] = {(void *) "/system/lib/libc.so", (void *) "gettimeofday",
                                 (void *) gettimeofday_local, &gettimeofday_orig};
void *hook_params_clock_time[4] = {(void *) "/system/lib/libc.so", (void *) "clock_gettime",
                                   (void *) clock_gettime_local, &clock_gettime_orig};

int is_hooked_utc_time_fun = 0;
int is_hooked_clock_time_fun = 0;


void hookSubstrate(const char *dlLocation, void *hook_params[]) {
    void (*MSHookFunction)(void *symbol, void *replace, void **result);

    log("hookSubstrate:%s\n", dlLocation);
    void *substrate_sub = dlopen(dlLocation, RTLD_NOW);
    if (!substrate_sub) {
        log("open libsubstrate.so fail:%s\n", dlLocation);
        return;
    }

    MSHookFunction = (void (*)(void *, void *, void **)) dlsym(substrate_sub, "MSHookFunction");
    if (nullptr == MSHookFunction) {
        log("can't find MSHookFunction\n");
        return;
    }

    char *target_lib = (char *) hook_params[0];
    char *target_func = (char *) hook_params[1];
    void *local_func = hook_params[2];
    void *orig_func = hook_params[3];
    log("start hook func(%s) in lib(%s), local(%d), orig(%d)", target_func, target_lib,
        local_func, orig_func);
    void *target_lib_sub = dlopen(target_lib, RTLD_NOW);
    if (!target_lib_sub) {
        log("can't find lib(%s) to hook\n", target_lib);
        return;
    }

    void *target_func_symbol = dlsym(target_lib_sub, target_func);
    if (!target_func_symbol) {
        log("can't find func(%s) in (%s)", target_func, target_lib);
        dlclose(target_lib_sub);
        return;
    }

    MSHookFunction(target_func_symbol, local_func, (void **) orig_func);
    dlclose(target_lib_sub);

    dlclose(substrate_sub);
}

char *jstringTostring(JNIEnv *env, jstring jstr) {
    char *rtn = nullptr;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char *) malloc(alen + 1);

        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

JNIEXPORT void JNICALL Java_com_timespeed_lib_TimeSpeed_speedUTCTime(
        JNIEnv *env, jobject /* this */, jstring substrate_path, jfloat velocity) {
    if (is_hooked_utc_time_fun == 0) {
        hookSubstrate(jstringTostring(env, substrate_path), hook_params_utc_time);
        is_hooked_utc_time_fun = 1;
    }

    if (velocity <= 0) {
        return;
    }
    utc_time_velocity = velocity;

    log("utc_time_velocity [%f]", utc_time_velocity);
}

JNIEXPORT void JNICALL Java_com_timespeed_lib_TimeSpeed_speedClockTime(
        JNIEnv *env, jobject /* this */, jstring substrate_path, jfloat velocity) {
    if (is_hooked_clock_time_fun == 0) {
        hookSubstrate(jstringTostring(env, substrate_path), hook_params_clock_time);
        is_hooked_clock_time_fun = 1;
    }

    if (velocity <= 0) {
        return;
    }
    clock_time_velocity = velocity;

    log("clock_time_velocity [%f]", clock_time_velocity);
}


JNIEXPORT jlong JNICALL Java_com_timespeed_lib_TimeSpeed_getRealClockTime(
        JNIEnv *env, jobject /* this */) {
    return g_clock_gettime_last_real_usecs / 1000000LL;
}

JNIEXPORT jlong JNICALL Java_com_timespeed_lib_TimeSpeed_getRealUTCTime(
        JNIEnv *env, jobject /* this */) {
    return g_gettimeofday_last_real_usecs / 1000000LL;
}
}