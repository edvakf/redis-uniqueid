#include "redismodule.h"
#include <time.h>
#include <stddef.h>

size_t workerIdBits = 10;
size_t seqBits = 12;

long long lastTs;
long long seq = 0;
long long workerId = 0;

const int ERR_OK = 0;
const int ERR_TIME_ROLLBACK = 1;

long long getTimestampMsec(void) {
    long long sec, msec;
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    sec = (long long)spec.tv_sec;
    msec = (long long)spec.tv_nsec / 1000000;

    return sec * 1000 + msec;
}

int generateUniqueId(long long *uniqueId) {

    long long ts = getTimestampMsec();

    if (ts < lastTs) {
        return ERR_TIME_ROLLBACK;
    } else if (ts == lastTs) {
        seq = (seq + 1) & ((1 << seqBits) - 1); // TODO: pre-calculate mask
    } else {
        seq = 0;
    }

    lastTs = ts;

    *uniqueId = (ts << (workerIdBits + seqBits)) | (seq << workerId) | (workerId);

    return ERR_OK;
}

int UniqueIdGetWorkerId_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);

    if (argc != 1) return RedisModule_WrongArity(ctx);

    RedisModule_ReplyWithLongLong(ctx, workerId);

    return REDISMODULE_OK;
}

int UniqueIdSetWorkerId_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) return RedisModule_WrongArity(ctx);

    long long mid;
    int err = RedisModule_StringToLongLong(argv[1], &mid);
    if (err == REDISMODULE_ERR) {
        RedisModule_ReplyWithError(ctx, "Machine ID must be an integer");
    } else {
        workerId = mid;
        RedisModule_ReplyWithLongLong(ctx, workerId);
    }

    return REDISMODULE_OK;
}

int UniqueIdGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);

    if (argc != 1) return RedisModule_WrongArity(ctx);

    long long uniqueId;

    int err = generateUniqueId(&uniqueId);
    if (err == ERR_OK) {
        RedisModule_ReplyWithLongLong(ctx, uniqueId);
    } else if (err == ERR_TIME_ROLLBACK) {
        RedisModule_ReplyWithError(ctx, "Server time has rolled back");
    } else {
        RedisModule_ReplyWithError(ctx, "Unexpected error");
    }

    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (RedisModule_Init(ctx,"uniqueid",1,REDISMODULE_APIVER_1)
        == REDISMODULE_ERR) return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"uniqueid.getworkerid",
        UniqueIdGetWorkerId_RedisCommand,"readonly random fast",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"uniqueid.setworkerid",
        UniqueIdSetWorkerId_RedisCommand,"write random fast",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"uniqueid.get",
        UniqueIdGet_RedisCommand,"readonly random fast",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
