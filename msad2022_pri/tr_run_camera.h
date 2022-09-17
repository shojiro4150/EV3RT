#define TR_RUN_CAMERA_R \
    /* RUN: a regression test case to ensure sensor trace works */ \
    .composite<BrainTree::MemSequence>() \
        .composite<BrainTree::ParallelSequence>(1, 2) \
        .leaf<TraceLine>(prof->getValueAsNum("CAM_SPEED"), \
                         prof->getValueAsNum("CAM_TARGET"), \
                         prof->getValueAsNum("CAM_P_CONST"), \
                         prof->getValueAsNum("CAM_I_CONST"), \
                         prof->getValueAsNum("CAM_D_CONST"), 0.0, \
                         (TraceSide)prof->getValueAsNum("TR_TS")) \
        .leaf<IsDistanceEarned>(prof->getValueAsNum("TR_DIST")) \
        .end() \
    .end()

#define TR_RUN_CAMERA_L \
    /* RUN: until detecting BLUE while executing MemSequence in parallel */ \
    .composite<BrainTree::ParallelSequence>(1, 2) \
        .leaf<IsDistanceEarned>(prof->getValueAsNum("RUNx_DIST")) /* temp stopper */  \
        .composite<BrainTree::MemSequence>() \
            /* RUN1: until passing Gate 3 */ \
            .composite<BrainTree::ParallelSequence>(1, 2) \
                .leaf<TraceLineCam>(prof->getValueAsNum("RUN1_SPEED"), \
                                    prof->getValueAsNum("RUNx_P_CONST"), \
                                    prof->getValueAsNum("RUNx_I_CONST"), \
                                    prof->getValueAsNum("RUNx_D_CONST"), \
                                    prof->getValueAsNum("RUNx_GS_MIN"), \
                                    prof->getValueAsNum("RUNx_GS_MAX"), 0.0, TS_OPPOSITE) \
                .leaf<IsDistanceEarned>(prof->getValueAsNum("RUN1_DIST")) \
            .end() \
            /* RUN2: until the intersection between Gate 3 and 4 */ \
            .composite<BrainTree::ParallelSequence>(1, 2) \
                .leaf<TraceLineCam>(prof->getValueAsNum("RUN2_SPEED"), \
                                    prof->getValueAsNum("RUNx_P_CONST"), \
                                    prof->getValueAsNum("RUNx_I_CONST"), \
                                    prof->getValueAsNum("RUNx_D_CONST"), \
                                    prof->getValueAsNum("RUNx_GS_MIN"), \
                                    prof->getValueAsNum("RUNx_GS_MAX"), 0.0, TS_NORMAL) \
                .leaf<IsJunction>(JST_JOINED) \
            .end() \
            /* RUN3: until passing the intersection */ \
            .composite<BrainTree::ParallelSequence>(1, 2) \
                .leaf<RunAsInstructed>(prof->getValueAsNum("RUN3_PWML"), \
                                    prof->getValueAsNum("RUN3_PWMR"), 0.0) \
                .leaf<IsDistanceEarned>(prof->getValueAsNum("RUN3_DIST")) \
            .end() \
            /* RUN4: before the corner around Gate 4 */ \
            .composite<BrainTree::ParallelSequence>(1, 2) \
                .leaf<TraceLineCam>(prof->getValueAsNum("RUN4_SPEED"), \
                                    prof->getValueAsNum("RUNx_P_CONST"), \
                                    prof->getValueAsNum("RUNx_I_CONST"), \
                                    prof->getValueAsNum("RUNx_D_CONST"), \
                                    prof->getValueAsNum("RUNx_GS_MIN"), \
                                    prof->getValueAsNum("RUNx_GS_MAX"), 0.0, TS_OPPOSITE) \
                .leaf<IsDistanceEarned>(prof->getValueAsNum("RUN4_DIST")) \
            .end() \
            /* RUN5: until passing Gate 4 */ \
            .composite<BrainTree::ParallelSequence>(1, 2) \
                .leaf<TraceLineCam>(prof->getValueAsNum("RUN5_SPEED"), \
                                    prof->getValueAsNum("RUNx_P_CONST"), \
                                    prof->getValueAsNum("RUNx_I_CONST"), \
                                    prof->getValueAsNum("RUNx_D_CONST"), \
                                    prof->getValueAsNum("RUNx_GS_MIN"), \
                                    prof->getValueAsNum("RUNx_GS_MAX"), 0.0, TS_OPPOSITE) \
                .leaf<IsDistanceEarned>(prof->getValueAsNum("RUN5_DIST")) \
            .end() \
            /* RUN6: the rest until detecting BLUE */ \
            .composite<BrainTree::ParallelSequence>(2, 2) \
                .leaf<TraceLineCam>(prof->getValueAsNum("RUN6_SPEED"), \
                                    prof->getValueAsNum("RUNx_P_CONST"), \
                                    prof->getValueAsNum("RUNx_I_CONST"), \
                                    prof->getValueAsNum("RUNx_D_CONST"), \
                                    prof->getValueAsNum("RUNx_GS_MIN"), \
                                    prof->getValueAsNum("RUNx_GS_MAX"), 0.0, TS_OPPOSITE) \
                .leaf<IsColorDetected>(CL_BLUE_CAM) \
                .leaf<SetArmPosition>(0, 40) \
            .end() \
            /* RUN7: Turn and move forward.  */ \
            .composite<BrainTree::MemSequence>() \
                .composite<BrainTree::ParallelSequence>(1, 2) \
                    .leaf<IsTimeEarned>(1400000) \
                    .leaf<RunAsInstructed>(50, -50, 0.0) \
                .end() \
                .composite<BrainTree::ParallelSequence>(1, 2) \
                    .leaf<IsColorDetected>(CL_BLACK) \
                    .leaf<RunAsInstructed>(50, -50, 0.0) \
                .end() \
                .composite<BrainTree::ParallelSequence>(1, 2) \
                    .leaf<IsTimeEarned>(200000) \
                    .leaf<RunAsInstructed>(50, -50, 0.0) \
                .end() \
            .end() \
        .end() \
    .end()