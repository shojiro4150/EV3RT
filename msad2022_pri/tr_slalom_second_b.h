/* スラローム後半B */
/* スラローム後半Bは左右で違いなし。モーター左右反転で対応可能 */
#define TR_SLALOM_SECOND_B \
    .composite<BrainTree::MemSequence>() \
    /* 後半第一スラローム開始 */ \
        .composite<BrainTree::ParallelSequence>(1,2)  \
            .leaf<IsDistanceEarned>(50) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            /* param SJ:100 */ \
            .leaf<IsDistanceEarned>(110) \
            /* param SJ:20,50 */ \
            .leaf<RunAsInstructed>(20, 50, 0.0)  \
        .end() \
        /* 後半第二スラローム開始 */ \
        .composite<BrainTree::ParallelSequence>(1,2)  \
            .leaf<IsDistanceEarned>(20) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
        /* カーブを分割すると綺麗になる */ \
        .composite<BrainTree::ParallelSequence>(1,2)  \
            .leaf<IsDistanceEarned>(40) \
            .leaf<RunAsInstructed>(20, 50, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(250) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
        /* param IS:150 */ \
            .leaf<IsDistanceEarned>(120) \
            .leaf<RunAsInstructed>(50, 20, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(30) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
        /* param IS:150 */ \
            .leaf<IsDistanceEarned>(100)     \
            .leaf<RunAsInstructed>(50, 20, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(100) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
    .end()