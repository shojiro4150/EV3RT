/* Rコースのスラローム後半A */
#define TR_SLALOM_SECOND_A_R \

/* Lコースのスラローム後半A */
#define TR_SLALOM_SECOND_A_L \
    .composite<BrainTree::MemSequence>() \
    /* 後半第一スラローム開始 */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(30) \
            .leaf<RunAsInstructed>(-50, 0, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(50) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(20) \
            .leaf<RunAsInstructed>(-40, 0, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(50) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
        /* ここでペットボトル4,5 の間に進入する直前 */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(100) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(30) \
            .leaf<RunAsInstructed>(50, 15, 0.0) \
        .end() \
        /* 後半第二スラローム開始 */ \
        /* able to see end */ \
        .composite<BrainTree::MemSequence>() \
            .leaf<StopNow>() \
            /* wait 0.2 sec */ \
            .leaf<IsTimeEarned>(200000)  \
        .end() \
    .end() \