/* Rコースのスラローム前半 */
#define TR_SLALOM_FIRST_R \

/* Lコースのスラローム後半 */
#define TR_SLALOM_FIRST_L \
    .composite<BrainTree::MemSequence>() \
        /* ライントレースから引継ぎして、直前の青線まで走る */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsTimeEarned>(1000000) \
            .leaf<TraceLine>(45, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE) \
        .end() \
        .composite<BrainTree::ParallelSequence>(2,3) \
            .leaf<SetArmPosition>(0, 40) \
            .composite<BrainTree::MemSequence>() \
                .leaf<IsColorDetected>(CL_BLACK) \
                .leaf<IsColorDetected>(CL_BLUE) \
            .end() \
            .leaf<TraceLine>(35, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE) \
        .end() \
        /* 台にのる　勢いが必要 */ \
        /* 青検知の後、台乗上前にギリギリまでトレース */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(360) \
            .leaf<TraceLine>(45, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
        /* 段差ストップ 3.0 sec */ \
            .leaf<IsTimeEarned>(1000000)  \
            .leaf<TraceLine>(35, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_CENTER) \
        .end() \
        /* 遠藤追加（疑似台形駆動） */ \
        .composite<BrainTree::ParallelSequence>(1,2)  \
            .leaf<IsTimeEarned>(3000000)  \
            .leaf<TraceLine>(25, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_CENTER) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(150) \
            .leaf<TraceLine>(80, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_CENTER) \
        .end() \
        /* 0916 遠藤追加 */ \
        /* 第一スラローム開始 */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(70) \
            .leaf<RunAsInstructed>(0, 50, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(50) \
            .leaf<RunAsInstructed>(30, 30, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(40) \
            .leaf<RunAsInstructed>(50, 0, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(150) \
            .leaf<RunAsInstructed>(30, 30, 0.0) \
        .end() \
        /* テスト用停止 */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(10000000) \
            .leaf<RunAsInstructed>(0, 0, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(150) \
            .leaf<RunAsInstructed>(20, 50, 0.0) \
        .end() \
        /* 以下、変更前の決め打ち */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(30) \
            .leaf<RunAsInstructed>(30, 30, 0.0) \
        .end() \
        /* 第二スラローム開始 */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(120) \
            .leaf<RunAsInstructed>(60, 15, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(30) \
            .leaf<RunAsInstructed>(15, 40, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(10) \
            .leaf<RunAsInstructed>(30, 30, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(40) \
            .leaf<RunAsInstructed>(40, 15, 0.0) \
        .end() \
        /* 黒検知したらライントレース */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsColorDetected>(CL_BLACK) \
            .leaf<RunAsInstructed>(30, 20, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(160) \
            .leaf<TraceLine>(30, 47, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE) \
        .end() \
        /* ライントレースおもてなし */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(20) \
            .leaf<RunAsInstructed>(15, 50, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(20) \
            .leaf<RunAsInstructed>(50, 15, 0.0) \
        .end() \
        /* 第三スラローム開始 ライントレース */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(160) \
            .leaf<TraceLine>(30, 47, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
        .end() \
        /* 第三スラローム開始 ライントレース */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            /* .leaf<IsDistanceEarned>(50) */ \
            /* 超音波センサー＆ライトレースによるチェックポイント */ \
            .leaf<IsSonarOn>(450) \
            .leaf<TraceLine>(30, 47, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL) \
        .end() \
        /* ガレージカードスラローム開始 */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(90) \
            .leaf<RunAsInstructed>(50, 15, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(80) \
            .leaf<RunAsInstructed>(30, 30, 0.0) \
        .end() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsDistanceEarned>(60) \
            .leaf<RunAsInstructed>(15, 50, 0.0) \
        .end() \
        /* 色検知 */ \
        .composite<BrainTree::ParallelSequence>(1,2) \
            /* .leaf<IsDistanceEarned>(50) */ \
            .leaf<IsColorDetected>(CL_BLUE_SL) \
            .leaf<IsColorDetected>(CL_RED_SL) \
            .leaf<IsColorDetected>(CL_YELLOW_SL) \
            .leaf<IsColorDetected>(CL_GREEN_SL) \
            .leaf<RunAsInstructed>(30, 30, 0.0) \
        .end() \
    .end()