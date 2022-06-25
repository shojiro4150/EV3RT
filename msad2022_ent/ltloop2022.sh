#!/usr/bin/env bash

##ltloop.sh
trap 'killbackground > /dev/null 2>&1' EXIT

killbackground(){
    local jobs=`jobs -p`
    for job in $jobs; do
        killpstree $job
    done
}

killpstree(){
    local children=`pgrep -P $1`
    for child in $children; do
        killpstree $child
    done
    kill $1
}

usage_exit() {
        echo "  maxtime make開始から走行打ち切りまでの実時間" 1>&2
        echo "" 1>&2
        echo "  ログは${DSTDIR}に格納されています" 1>&2
        exit 1
}

SRCDIR=${ETROBO_HRP3_WORKSPACE}/msad2022_ent
DSTDIR=${ETROBO_ROOT}/workspace/msad2022_ent/work

MAKELOG="makelog"
LOOPLOG="lplog"
LOGEXT="txt"
CSVEXT="csv"
DT=`date "+%y%m%d%H%M%S"`

#実行ディレクトリはETROBOのroot
cd $ETROBO_ROOT

#処理打ち切り時間がパラメータにあったらセット
if [ $# -eq 0 ];then
    MAXTIME=30
else
    MAXTIME=$1
fi

# 最初に1回だけmake
make app=msad2022_ent sim 2>&1 | tee ${DSTDIR}/${MAKELOG}_${DT}.${LOGEXT}

# 処理ループ
for ((ll = 0; ll <= 1; ll++)) {
    for ((rot = 0; rot <= 1; rot++)) {
        for ((lsp = 0; lsp <= 1; lsp++)) {
# シミュレータ起動と初期位置設定
            sim ctl pos 2 0 -15.5 0
            sleep 5
# 光源値設定
#            curl -X POST -H "Content-Type: application/json" -d "{\"EnvLightIntensityLevel\":$ll,\"EnvLightRotation\":$rot,\"LSpotLight\":$lsp,\"RSpotLight\":"0"}" http://localhost:54000
            curl -X POST -H "Content-Type: application/json" -d "{\"EnvLightIntensityLevel\":"0",\"EnvLightRotation\":"0",\"LSpotLight\":"0",\"RSpotLight\":"0"}" http://localhost:54000
#            sleep 3
# アプリを実行しプロセスIDを記録
            asp msad2022_ent &
#            PID=`asp check l`
            PID=`asp check`
    echo $PID
# シミュレータ PREPAREモード
            sim ctl prepare
            sleep 3
# シミュレータ GOモード
            sim ctl go &
# 処理打ち切り時間を越えたらアプリプロセスの動作確認、プロセスが居たら殺す
            sleep $MAXTIME
            CNT=0
            CNT=`ps -ef | grep $PID | wc -l`
            if [ $CNT -ne 0 ]; then
                kill $PID
            fi
            wait $PID
# 終了処理
            echo "stop"
            sleep 2
            sim ctl end 2>&1 | tee ${DSTDIR}/lp_${ll}${rot}${lsp}0.${CSVEXT}
            asp stop
         }
     }
}
