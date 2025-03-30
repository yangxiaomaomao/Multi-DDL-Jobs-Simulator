#ifndef DDL_STATE_H
#define DDL_STATE_H
enum class JobState {
    UNARRIVED,  // 任务未到达
    ARRIVED,
    PENDING,    // 任务挂起等待
    RUNNING,    // 任务正在运行
    FINISH      // 任务完成
};
enum class GpuState {
    FREE,  // GPU空闲
    BUSY   // GPU忙碌
};
#endif
