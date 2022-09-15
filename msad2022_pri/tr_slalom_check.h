/* Rコースのスラローム転回 */
#define TR_SLALOM_CHECK_R \

/* Lコースのスラローム転回 */
#define TR_SLALOM_CHECK_L \
    /* 台上転回後、センサーでコースパターン判定 */ \
    .composite<BrainTree::MemSequence>() \
        /* 色検知 for test */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsColorDetected>(CL_BLUE_SL) \
            .leaf<IsColorDetected>(CL_RED_SL) \
            .leaf<IsColorDetected>(CL_YELLOW_SL) \
            .leaf<IsColorDetected>(CL_GREEN_SL) \
        .end() \
        /* move back */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsTimeEarned>(600000) \
            .leaf<RunAsInstructed>(-40, -40, 3.0) \
        .end() \
        /* rotate left with left wheel */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsTimeEarned>(300000) \
            .leaf<RunAsInstructed>(-40, 0, 0.0)  \
        .end() \
        /* move foward */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsTimeEarned>(400000) \
            .leaf<RunAsInstructed>(50, 50, 0.0) \
        .end() \
        /* turn left with right wheel */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsTimeEarned>(760000) \
            .leaf<RunAsInstructed>(0, 50, 0.0) \
        .end() \
        /* detect the distance between the robot and plastic bottle using ultrasonic sensor */ \
        /* determine the arrangement pattern of plastic bottles from the distance */ \
        /* rotate right until sensor detects the distance or 2 second pass */ \
        .composite<BrainTree::MemSequence>() \
            .leaf<StopNow>() \
            .composite<BrainTree::ParallelSequence>(1,2) \
            /* sonar 0.2 sec */ \
                .leaf<IsTimeEarned>(200000)  \
                .leaf<DetectSlalomPattern>() \
                .leaf<RunAsInstructed>(0, 35, 0.0) \
            .end() \
        .end() \
        /* able to see detected */ \
        .composite<BrainTree::MemSequence>() \
            .leaf<StopNow>() \
        /* wait 0.2 sec */ \
            .leaf<IsTimeEarned>(200000) \
        .end() \
    .end()