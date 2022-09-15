/* Rコースのガレージダミー2 */
#define TR_BLOCK_D2_R \

/* Lコースのガレージダミー2 */
#define TR_BLOCK_D2_L \
    .composite<BrainTree::MemSequence>() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<SetArmPosition>(10, 40) \
            .leaf<IsTimeEarned>(500000) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<TraceLine>(0, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
            .leaf<IsTimeEarned>(10000000) \
        .end() \
        .leaf<StopNow>() \
        .leaf<IsTimeEarned>(30000000) /* wait 3 seconds */ \
    .end() \