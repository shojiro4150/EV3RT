/* キャリブレーション */
#define TR_CALIBRATION \
    .composite<BrainTree::MemSequence>() \
        .composite<BrainTree::ParallelSequence>(1,2) \
            .leaf<IsBackOn>() \
        .end() \
        .leaf<ResetClock>() \
    .end()
/*
//  .composite<BrainTree::MemSequence>()
//  temp fix 2022/6/20 W.Taniguchi, as no touch sensor available on RasPike
//      .decorator<BrainTree::UntilSuccess>()
//      .leaf<IsBackOn>()
//            .leaf<ResetClock>()
//        .end()
//  .end()
*/