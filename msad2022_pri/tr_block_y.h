/* ガレージ黄 */
/* ガレージは左右で違いなし。モーター左右反転で対応可能 */
#define TR_BLOCK_Y \
    .composite<BrainTree::MemSequence>() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<SetArmPosition>(10, 40) \
            .leaf<IsTimeEarned>(500000) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<TraceLine>(40, \
                                GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
            .leaf<IsColorDetected>(CL_BLUE) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(975000) /* 後ろ向き走行。狙いは黒線。 */ \
            .leaf<RunAsInstructed>(-30, -80, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(1700000) /* 後ろ向き走行。狙いは黒線。 */ \
            .leaf<RunAsInstructed>(-50, -50, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(4000000) /* 後ろ向き走行。狙いは黒線。 */ \
            .leaf<RunAsInstructed>(-35, -35, 0.0) \
            .leaf<IsColorDetected>(CL_BLACK) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(900000) /* 黒線検知後、ライントレース準備 */ \
            .leaf<RunAsInstructed>(-30, 55, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<TraceLine>(35, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
            .leaf<IsTimeEarned>(1500000) /* 黒線検知後、ライントレース準備 */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<TraceLine>(40, S_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
            .leaf<IsColorDetected>(CL_GRAY) /* グレー検知までライントレース */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .composite<BrainTree::MemSequence>() \
                .leaf<IsColorDetected>(CL_GRAY) /* グレー検知までライントレース */ \
                .leaf<IsColorDetected>(CL_WHITE) /* グレー検知までライントレース */ \
                .leaf<IsColorDetected>(CL_GRAY) /* グレー検知までライントレース */ \
            .end() \
            .leaf<IsTimeEarned>(1000000) /* break after 10 seconds */ \
            .leaf<RunAsInstructed>(45, 45, 0.0)   /* グレー検知後、丸穴あき部分があるため少し前進 */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(600000) /* break after 10 seconds */ \
            .leaf<RunAsInstructed>(-50, 50, 0.0) /* 左に旋回。ライントレース準備。 */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(1000000) /* 少し前進。ライントレース準備。 */ \
            .leaf<RunAsInstructed>(40, 42, 0.0) \
                .leaf<IsColorDetected>(CL_BLACK) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(5000000) \
            .leaf<TraceLine>(40, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE) \
            .leaf<IsColorDetected>(CL_BLUE2)  /* 純粋な青検知までライントレース */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(300000) /* break after 10 seconds */ \
            .leaf<RunAsInstructed>(44, -44, 0.0)   /* 青検知後は大きく右に旋回 */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(1000000) /* break after 10 seconds */ \
            .leaf<RunAsInstructed>(35, 55, 0.0)  /* 前進。次の青検知を目指す。 */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(5000000) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
            .leaf<IsColorDetected>(CL_BLUE2)  /* 前進。次の青検知を目指す。 */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(890000) /* 青検知後、大きく右旋回。向きを整える。 */ \
            .leaf<RunAsInstructed>(55, -50, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsColorDetected>(CL_WHITE) \
            .leaf<TraceLine>(37, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(2000000) \
            .leaf<RunAsInstructed>(40, 40, 0.0)  /* 目的の色検知まで前進 */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(5000000) \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
            .leaf<IsColorDetected>(CL_YELLOW) \
        .end() \
        .leaf<StopNow>() \
        .leaf<IsTimeEarned>(30000000) /* wait 3 seconds */ \
        .leaf<SetArmPosition>(10, 40) \
    .end()