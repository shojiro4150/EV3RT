/* ガレージ赤 */
/* ガレージは左右で違いなし。モーター左右反転で対応可能 */
#define TR_BLOCK_R \
    .composite<BrainTree::MemSequence>() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<SetArmPosition>(10, 40) \
            .leaf<IsTimeEarned>(500000) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<TraceLine>(38, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
            .leaf<IsColorDetected>(CL_BLUE) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(975000) /* 後ろ向き走行。狙いは黒線。 */ \
            .leaf<RunAsInstructed>(-30, -80, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(1700000) /* 後ろ向き走行。狙いは黒線。 */ \
            .leaf<RunAsInstructed>(-50,-50,  0.0) \
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
            .leaf<TraceLine>(40, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
            .leaf<IsColorDetected>(CL_GRAY) /* グレー検知までライントレース */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .composite<BrainTree::MemSequence>() \
                .leaf<IsColorDetected>(CL_GRAY) /* グレー検知までライントレース */ \
                .leaf<IsColorDetected>(CL_WHITE) /* グレー検知までライントレース */ \
                .leaf<IsColorDetected>(CL_GRAY) /* グレー検知までライントレース */ \
            .end() \
            .leaf<IsTimeEarned>(1000000) /* break after 10 seconds */ \
            .leaf<RunAsInstructed>(46, 46, 0.0)   /* グレー検知後、丸穴あき部分があるため少し前進 */ \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(1075000)  /* 本線ラインに戻ってくる */ \
            .leaf<RunAsInstructed>(75, 40, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(1000000)  /* 本線ラインに戻ってくる */ \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
            .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(5000000) /* 本線ラインに戻ってくる。黒ラインか青ライン検知 */ \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
            .leaf<IsColorDetected>(CL_BLACK) \
            .leaf<IsColorDetected>(CL_BLUE2) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(800000) /* 検知後、斜め右前まで回転(ブロックを離さないように) */ \
            .leaf<RunAsInstructed>(-55, 60, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(1500000) \
            .leaf<TraceLine>(35,GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsColorDetected>(CL_WHITE) \
            .leaf<TraceLine>(35,GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(475000) /* 全身しながら大きく左に向けて旋回。黄色を目指す。 */ \
            .leaf<RunAsInstructed>(-55, 55, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsColorDetected>(CL_YELLOW)    /* 黄色検知後、方向立て直す。 */ \
            .leaf<RunAsInstructed>(45, 45, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsTimeEarned>(750000) \
            .leaf<RunAsInstructed>(30, 75, 0.0) \
            .leaf<IsColorDetected>(CL_RED) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,3) \
            .leaf<IsColorDetected>(CL_RED)  /* 赤検知までまっすぐ進む。 */ \
            .leaf<RunAsInstructed>(40, 40, 0.0) \
        .end() \
        .leaf<StopNow>() \
        .leaf<IsTimeEarned>(30000000) /* wait 3 seconds */ \
    .end()