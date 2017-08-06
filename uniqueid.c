#include "redismodule.h"
#include <time.h>
#include <stddef.h>

size_t machineIdBits = 10;
size_t seqBits = 12;

long long lastTs;
long long seq = 0;
long long machineId = 0;

long long getTimestampMsec(void) {
    long long sec, msec;
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    sec = (long long)spec.tv_sec;
    msec = (long long)spec.tv_nsec / 1000000;

    return sec * 1000 + msec;
}

long long generateUniqueId(void) {

    long long ts = getTimestampMsec();

    if (ts == lastTs) {
        seq = (seq + 1) & ((1 << seqBits) - 1); // TODO: pre-calculate mask
    } else {
        // TODO: error when ts < lastTs
        seq = 0;
    }

    lastTs = ts;

    return (ts << (machineIdBits + seqBits)) | (seq << machineId) | (machineId);
}

int UniqueIdGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);

    if (argc != 1) return RedisModule_WrongArity(ctx);

    long long uniqueId = generateUniqueId();

    RedisModule_ReplyWithLongLong(ctx, uniqueId);

    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (RedisModule_Init(ctx,"uniqueid",1,REDISMODULE_APIVER_1)
        == REDISMODULE_ERR) return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"uniqueid.get",
        UniqueIdGet_RedisCommand,"readonly random fast",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
