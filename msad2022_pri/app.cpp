/*
    app.cpp
    Copyright © 2022 MSAD Mode2P. All rights reserved.
*/
#include "BrainTree.h"
#include "Profile.hpp"
/*
    BrainTree.h must present before ev3api.h on RasPike environment.
    Note that ev3api.h is included by app.h.
*/
#include "app.h"
#include "appusr.hpp"
#include <iostream>
#include <list>
#include <numeric>
#include <math.h>


/* this is to avoid linker error, undefined reference to `__sync_synchronize' */
extern "C" void __sync_synchronize() {}

/* global variables */
FILE*           bt;
Profile*        prof;
Clock*          ev3clock;
TouchSensor*    touchSensor;
SonarSensor*    sonarSensor;
FilteredColorSensor*    colorSensor;
GyroSensor*     gyroSensor;
SRLF*           srlfL;
FilteredMotor*  leftMotor;
SRLF*           srlfR;
FilteredMotor*  rightMotor;
Motor*          armMotor;
Plotter*        plotter;

BrainTree::BehaviorTree* tr_calibration = nullptr;
BrainTree::BehaviorTree* tr_run         = nullptr;
BrainTree::BehaviorTree* tr_slalom_first      = nullptr;
BrainTree::BehaviorTree* tr_slalom_check      = nullptr;
BrainTree::BehaviorTree* tr_slalom_second_a      = nullptr;
BrainTree::BehaviorTree* tr_slalom_second_b      = nullptr;
BrainTree::BehaviorTree* tr_block_r     = nullptr;
BrainTree::BehaviorTree* tr_block_g     = nullptr;
BrainTree::BehaviorTree* tr_block_b     = nullptr;
BrainTree::BehaviorTree* tr_block_y     = nullptr;
BrainTree::BehaviorTree* tr_block_d     = nullptr;
BrainTree::BehaviorTree* tr_block_d2    = nullptr;
State state = ST_INITIAL;

/*
    === NODE CLASS DEFINITION STARTS HERE ===
    A Node class serves like a LEGO block while a Behavior Tree serves as a blueprint for the LEGO object built using the LEGO blocks.
*/

/*
    usage:
    ".leaf<ResetClock>()"
    is to reset the clock and indicate the robot departure by LED color.
*/
class ResetClock : public BrainTree::Node {
public:
    Status update() override {
        ev3clock->reset();
        _log("clock reset.");
        ev3_led_set_color(LED_GREEN);
        return Status::Success;
    }
};

/*
    usage:
    ".leaf<StopNow>()"
    is to stop the robot.
*/
class StopNow : public BrainTree::Node {
public:
    Status update() override {
        leftMotor->setPWM(0);
        rightMotor->setPWM(0);
        _log("robot stopped.");
        return Status::Success;
    }
};

/*
    usage:
    ".leaf<IsTouchOn>()"
    is to check if the touch sensor gets pressed.
*/
class IsTouchOn : public BrainTree::Node {
public:
    Status update() override {
        if (touchSensor->isPressed()) {
            _log("touch sensor pressed.");
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
};

/*
    usage:
    ".leaf<IsBackOn>()"
    is to check if the back button gets pressed.
*/
class IsBackOn : public BrainTree::Node {
public:
    Status update() override {
        if (ev3_button_is_pressed(BACK_BUTTON)) {
            _log("back button pressed.");
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
};

class IsCeneterOn : public BrainTree::Node {
public:
    Status update() override {
        if (ev3_button_is_pressed(ENTER_BUTTON)) {
            _log("center button pressed.");
            return Status::Success;
        } else {
            return Status::Running;
        }
    }
};

/*
    usage:
    ".leaf<IsSonarOn>(distance)"
    is to determine if the robot is closer than the spedified alert distance
    to an object in front of sonar sensor.
    dist is in millimeter.
*/
class IsSonarOn : public BrainTree::Node {
public:
    IsSonarOn(int32_t d) : alertDistance(d) {}
    Status update() override {
        int32_t distance = 10 * (sonarSensor->getDistance());
        if ((distance <= alertDistance) && (distance >= 0)) {
            _log("sonar alert at %d", distance);
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
protected:
    int32_t alertDistance;
};

/*
    usage:
    ".leaf<DetectSlalomPattern>()"
    is to determine slalom pattern from the distance between the robot and plastic bottle using ultrasonic sensor.
*/
class DetectSlalomPattern : public BrainTree::Node {
public:
    static bool isSlalomPatternA;
    static int32_t earnedDistance;
    DetectSlalomPattern() {}
    Status update() override {
        distance = 10 * (sonarSensor->getDistance());
        _log("sonar recieved distance: %d", distance);
        if (0 < distance && distance <= 250) {
            //*ptrSlalomPattern = 1;
            isSlalomPatternA = true;
            earnedDistance = distance;
            return Status::Success;
        } else if (300 < distance && distance < 400) {
            //*ptrSlalomPattern = 2;
            isSlalomPatternA = false;
            earnedDistance = distance;
            return Status::Success;
        } else {
            return Status::Running;
        }
    }
protected:
    int32_t distance;
};
bool DetectSlalomPattern::isSlalomPatternA = true;
int32_t DetectSlalomPattern::earnedDistance = 0;

/*
    usage:
    ".leaf<IsAngleLarger>(angle)"
    is to determine if the angular location of the robot measured by the gyro sensor is larger than the spedified angular value.
    angle is in degree.
*/
class IsAngleLarger : public BrainTree::Node {
public:
    IsAngleLarger(int ang) : angle(ang) {}
    Status update() override {
        int32_t curAngle = gyroSensor->getAngle(); 
        if (curAngle >= angle){
            return Status::Success;
        } else {
            return Status::Running;
        }
    }
protected:
    int32_t angle;
};

/*
    usage:
    ".leaf<IsAngleSmaller>(angle)"
    is to determine if the angular location of the robot measured by the gyro sensor is smaller than the spedified angular value.
    angle is in degree.
*/
class IsAngleSmaller : public BrainTree::Node {
public:
    IsAngleSmaller(int ang) : angle(ang) {}
    Status update() override {
        int32_t curAngle = gyroSensor->getAngle(); 
        if (curAngle <= angle){
            return Status::Success;
        } else {
            return Status::Running;
        }
    }
protected:
    int32_t angle;
};

/*
    usage:
    ".leaf<IsDistanceEarned>(dist)"
    is to determine if the robot has accumulated for the specified distance since update() was invoked for the first time.
    dist is in millimeter.
*/
class IsDistanceEarned : public BrainTree::Node {
public:
    IsDistanceEarned(int32_t d) : deltaDistTarget(d) {
        updated = false;
        earned = false;
    }
    Status update() override {
        if (!updated) {
            originalDist = plotter->getDistance();
            _log("ODO=%05d, Distance accumulation started.", originalDist);
            updated = true;
        }
        int32_t deltaDist = plotter->getDistance() - originalDist;
        
        if ((deltaDist >= deltaDistTarget) || (-deltaDist <= -deltaDistTarget)) {
            if (!earned) {
                _log("ODO=%05d, Delta distance %d is earned.", plotter->getDistance(), deltaDistTarget);
                earned = true;
            }
            return Status::Success;
        } else {
            return Status::Running;
        }
    }
protected:
    int32_t deltaDistTarget, originalDist;
    bool updated, earned;
};

/*
    usage:
    ".leaf<IsTimeEarned>(time)"
    is to determine if the robot has accumulated for the specified time since update() was invoked for the first time.
    time is in microsecond = 1/1,000,000 second.
*/
class IsTimeEarned : public BrainTree::Node {
public:
    IsTimeEarned(int32_t t) : deltaTimeTarget(t) {
        updated = false;
        earned = false;
    }
    Status update() override {
        if (!updated) {
            originalTime = ev3clock->now();
            _log("ODO=%05d, Time accumulation started.", plotter->getDistance());
             updated = true;
        }
        int32_t deltaTime = ev3clock->now() - originalTime;

        if (deltaTime >= deltaTimeTarget) {
            if (!earned) {
                 _log("ODO=%05d, Delta time %d is earned.", plotter->getDistance(), deltaTimeTarget);
                earned = true;
            }
            return Status::Success;
        } else {
            return Status::Running;
        }
    }
protected:
    int32_t deltaTimeTarget, originalTime;
    bool updated, earned;
};

/*
    usage:
    ".leaf<IsColorDetected>(color)"
    is to determine if the specified color gets detected.
    For possible color that can be specified as the argument, see enum Color in "appusr.hpp".
*/
class IsColorDetected : public BrainTree::Node {
public:
    static Color garageColor;
    IsColorDetected(Color c) : color(c) {
        updated = false;
    }
    Status update() override {
        if (!updated) {
            _log("ODO=%05d, Color detection started.", plotter->getDistance());
            updated = true;
        }
        rgb_raw_t cur_rgb;
        colorSensor->getRawColor(cur_rgb);

        switch(color){
            case CL_JETBLACK:
                if (cur_rgb.r <=35 && cur_rgb.g <=35 && cur_rgb.b <=50) { 
                    _log("ODO=%05d, CL_JETBLACK detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_JETBLACK_YMNK:
                if (cur_rgb.r <=11 && cur_rgb.g <=13 && cur_rgb.b <=15) { 
                    _log("ODO=%05d, CL_JETBLACK_YMNK detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_BLACK:
                if (cur_rgb.r <=50 && cur_rgb.g <=45 && cur_rgb.b <=60) {
                    _log("ODO=%05d, CL_BLACK detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_BLUE:
                if (cur_rgb.b - cur_rgb.r > 35 && cur_rgb.g >= 55 && cur_rgb.b <= 100 && cur_rgb.b >= 70) {
                    _log("ODO=%05d, CL_BLUE detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_BLUE_SL:
                if (cur_rgb.b - cur_rgb.r > 20 && cur_rgb.g <= 100 && cur_rgb.b <= 120) {
                    garageColor = CL_BLUE_SL;
                    _log("ODO=%05d, CL_BLUE_SL detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_BLUE2:
                if (cur_rgb.r <= 20 && cur_rgb.g <= 40 && cur_rgb.b >= 44 && cur_rgb.b - cur_rgb.r > 20) {
//                if (cur_rgb.r <= 20 && cur_rgb.g <= 55 && cur_rgb.b >= 55 && cur_rgb.b - cur_rgb.r > 20) {
                    _log("ODO=%05d, CL_BLUE2 detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_RED:
                if (cur_rgb.r - cur_rgb.b >= 30 && cur_rgb.r > 80 && cur_rgb.g < 45) {
                    _log("ODO=%05d, CL_RED detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_RED_SL:
                if (cur_rgb.r - cur_rgb.b >= 25 && cur_rgb.r > 85 && cur_rgb.g < 60) {
                    garageColor = CL_RED_SL;
                    _log("ODO=%05d, CL_RED_SL detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_YELLOW:
                if (cur_rgb.r >= 90 &&  cur_rgb.g >= 90 && cur_rgb.b <= 75) {
                    _log("ODO=%05d, CL_YELLOW detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_YELLOW_SL:
                if (cur_rgb.r >= 110 &&  cur_rgb.g >= 90 && cur_rgb.b >= 50 && cur_rgb.b <= 120 ) {
                    garageColor = CL_YELLOW_SL;
                    _log("ODO=%05d, CL_YELLOW_SL detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_GREEN:
                if (cur_rgb.g - cur_rgb.r > 20 && cur_rgb.g >= 40 && cur_rgb.r <= 100) {
                    _log("ODO=%05d, CL_GREEN detected.", plotter->getDistance());
                    return Status::Success;
                }
            case CL_GREEN_SL:
                if (cur_rgb.b - cur_rgb.r < 30 && cur_rgb.g >= 30 && cur_rgb.b <= 80) {
                    garageColor = CL_GREEN_SL;
                    _log("ODO=%05d, CL_GREEN_SL detected.", plotter->getDistance());
                    return Status::Success;
                }   
                break;
            case CL_GRAY:
                if (cur_rgb.r >= 45 && cur_rgb.g <=60 && cur_rgb.b <= 65 && cur_rgb.r <= 52 && cur_rgb.b >= 53) {
                    _log("ODO=%05d, CL_GRAY detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            case CL_WHITE:
                if (cur_rgb.r >= 100 && cur_rgb.g >= 100 && cur_rgb.b >= 100 ) {
                    _log("ODO=%05d, CL_WHITE detected.", plotter->getDistance());
                    return Status::Success;
                }
                break;
            default:
                break;
        }
        return Status::Running;
    }
protected:
    Color color;
    bool updated;
};
Color IsColorDetected::garageColor = CL_BLUE_SL;    // define default color as blue

/*
    usage:
    ".leaf<TraceLine>(speed, target, p, i, d, srew_rate, trace_side)"
    is to instruct the robot to trace the line at the given speed.
    p, i, d are constants for PID control.
    target is the brightness level for the ideal line for trace.
    srew_rate = 0.0 indidates NO tropezoidal motion.
    srew_rate = 0.5 instructs FilteredMotor to change 1 pwm every two executions of update()
    until the current speed gradually reaches the instructed target speed.
    trace_side = TS_NORMAL   when in R(L) course and tracing the right(left) side of the line.
    trace_side = TS_OPPOSITE when in R(L) course and tracing the left(right) side of the line.
*/
class TraceLine : public BrainTree::Node {
public:
    TraceLine(int s, int t, double p, double i, double d, double srew_rate, TraceSide trace_side) : speed(s),target(t),srewRate(srew_rate),side(trace_side) {
        updated = false;
        ltPid = new PIDcalculator(p, i, d, PERIOD_UPD_TSK, -speed, speed);
    }
    ~TraceLine() {
        delete ltPid;
    }
    Status update() override {
        if (!updated) {
            /* The following code chunk is to properly set prevXin in SRLF */
            srlfL->setRate(0.0);
            leftMotor->setPWM(leftMotor->getPWM());
            srlfR->setRate(0.0);
            rightMotor->setPWM(rightMotor->getPWM());
            _log("ODO=%05d, Trace run started.", plotter->getDistance());
            updated = true;
        }

        int16_t sensor;
        int8_t forward, turn, pwmL, pwmR;
        rgb_raw_t cur_rgb;

        colorSensor->getRawColor(cur_rgb);
        sensor = cur_rgb.r;
        /* compute necessary amount of steering by PID control */
        if (side == TS_NORMAL) {
            turn = (-1) * _COURSE * ltPid->compute(sensor, (int16_t)target);
        } else { /* side == TS_OPPOSITE */
            turn = _COURSE * ltPid->compute(sensor, (int16_t)target);
        }
        forward = speed;
        /* steer EV3 by setting different speed to the motors */
        pwmL = forward - turn;
        pwmR = forward + turn;
        srlfL->setRate(srewRate);
        leftMotor->setPWM(pwmL);
        srlfR->setRate(srewRate);
        rightMotor->setPWM(pwmR);
        return Status::Running;
    }
protected:
    int speed, target;
    PIDcalculator* ltPid;
    double srewRate;
    TraceSide side;
    bool updated;
};

/*
    usage:
    ".leaf<RunAsInstructed>(pwm_l, pwm_r, srew_rate)"
    is to move the robot at the instructed speed.
    srew_rate = 0.0 indidates NO tropezoidal motion.
    srew_rate = 0.5 instructs FilteredMotor to change 1 pwm every two executions of update()
    until the current speed gradually reaches the instructed target speed.
*/
class RunAsInstructed : public BrainTree::Node {
public:
    RunAsInstructed(int pwm_l, int pwm_r, double srew_rate) : pwmL(pwm_l),pwmR(pwm_r),srewRate(srew_rate) {
        updated = false;
        if (_COURSE == -1) {
            int pwm = pwmL;
            pwmL = pwmR;
            pwmR = pwm;            
        }     
    }
    Status update() override {
        if (!updated) {
            /* The following code chunk is to properly set prevXin in SRLF */
            srlfL->setRate(0.0);
            leftMotor->setPWM(leftMotor->getPWM());
            srlfR->setRate(0.0);
            rightMotor->setPWM(rightMotor->getPWM());
            _log("ODO=%05d, Instructed run started.", plotter->getDistance());
            updated = true;
        }
        srlfL->setRate(srewRate);
        leftMotor->setPWM(pwmL);
        srlfR->setRate(srewRate);
        rightMotor->setPWM(pwmR);
        return Status::Running;
    }
protected:
    int pwmL, pwmR;
    double srewRate;
    bool updated;
};

/*
    usage:
    ".leaf<RotateEV3>(30, speed, srew_rate)"
    is to rotate robot 30 degrees (=clockwise) at the specified speed.
    srew_rate = 0.0 indidates NO tropezoidal motion.
    srew_rate = 0.5 instructs FilteredMotor to change 1 pwm every two executions of update()
    until the current speed gradually reaches the instructed target speed.
*/
class RotateEV3 : public BrainTree::Node {
public:
    RotateEV3(int16_t degree, int s, double srew_rate) : deltaDegreeTarget(degree),speed(s),srewRate(srew_rate) {
        updated = false;
        assert(degree >= -180 && degree <= 180);
        if (degree > 0) {
            clockwise = 1;
        } else {
            clockwise = -1;
        }
    }
    Status update() override {
        if (!updated) {
            originalDegree = plotter->getDegree();
            srlfL->setRate(srewRate);
            srlfR->setRate(srewRate);
            /* stop the robot at start */
            leftMotor->setPWM(0);
            rightMotor->setPWM(0);
            _log("ODO=%05d, Rotation started. Current degree = %d", plotter->getDistance(), originalDegree);
            updated = true;
            return Status::Running;
        }

        int16_t deltaDegree = plotter->getDegree() - originalDegree;
        if (deltaDegree > 180) {
            deltaDegree -= 360;
        } else if (deltaDegree < -180) {
            deltaDegree += 360;
        }
        if (clockwise * deltaDegree < clockwise * deltaDegreeTarget) {
            if ((srewRate != 0.0) && (clockwise * deltaDegree >= clockwise * deltaDegreeTarget - 5)) {
                /* when comes to the half-way, start decreazing the speed by tropezoidal motion */    
                leftMotor->setPWM(clockwise * 3);
                rightMotor->setPWM(-clockwise * 3);
            } else {
                leftMotor->setPWM(clockwise * speed);
                rightMotor->setPWM((-clockwise) * speed);
            }
            return Status::Running;
        } else {
            _log("ODO=%05d, Rotation ended. Current degree = %d", plotter->getDistance(), plotter->getDegree());
            return Status::Success;
        }
    }
private:
    int16_t deltaDegreeTarget, originalDegree;
    int clockwise, speed;
    bool updated;
    double srewRate;
};

class ClimbBoard : public BrainTree::Node { 
public:
    ClimbBoard(int direction, int count) : dir(direction), cnt(count) {}
    Status update() override {
        curAngle = gyroSensor->getAngle();
            if (cnt >= 1) {
                leftMotor->setPWM(0);
                rightMotor->setPWM(0);
                armMotor->setPWM(-50);
                cnt++;
                if(cnt >= 200){
                    return Status::Success;
                }
                return Status::Running;
            } else {
                armMotor->setPWM(30);
                leftMotor->setPWM(23);
                rightMotor->setPWM(23);

                if (curAngle < -9) {
                    prevAngle = curAngle;
                }
                if (prevAngle < -9 && curAngle >= 0) {
                    ++cnt;
                    _log("ON BOARD");
                }
                return Status::Running;
            }
    }
private:
    int8_t dir;
    int cnt;
    int32_t curAngle;
    int32_t prevAngle;
};

/*
    usage:
    ".leaf<SetArmPosition>(target_degree, pwm)"
    is to shift the robot arm to the specified degree by the spefied power.
*/
class SetArmPosition : public BrainTree::Node {
public:
    SetArmPosition(int32_t target_degree, int pwm) : targetDegree(target_degree),pwmA(pwm) {
        updated = false;
    }
    Status update() override {
        int32_t currentDegree = armMotor->getCount();
        if (!updated) {
            _log("ODO=%05d, Arm position is moving from %d to %d.", plotter->getDistance(), currentDegree, targetDegree);
            if (currentDegree == targetDegree) {
                return Status::Success; /* do nothing */
            } else if (currentDegree < targetDegree) {
                clockwise = 1;
            } else {
                clockwise = -1;
            }
            armMotor->setPWM(clockwise * pwmA);
            updated = true;
            return Status::Running;
        }
        if (((clockwise ==  1) && (currentDegree >= targetDegree)) ||
            ((clockwise == -1) && (currentDegree <= targetDegree))) {
            armMotor->setPWM(0);
            _log("ODO=%05d, Arm position set to %d.", plotter->getDistance(), currentDegree);
            return Status::Success;
        } else {
            return Status::Running;
        }
    }
private:
    int32_t targetDegree;
    int pwmA, clockwise;
    bool updated;
};

/*
    === NODE CLASS DEFINITION ENDS HERE ===
*/


/* a cyclic handler to activate a task */
void task_activator(intptr_t tskid) {
    ER ercd = act_tsk(tskid);
    assert(ercd == E_OK || E_QOVR);
    if (ercd != E_OK) {
        syslog(LOG_NOTICE, "act_tsk() returned %d", ercd);
    }
}

/* The main task */
void main_task(intptr_t unused) {
    // temp fix 2022/6/20 W.Taniguchi, as Bluetooth not implemented yet
    //bt = ev3_serial_open_file(EV3_SERIAL_BT);
    //assert(bt != NULL);
    /* create and initialize EV3 objects */
    ev3clock    = new Clock();
    touchSensor = new TouchSensor(PORT_1);
    // temp fix 2022/6/20 W.Taniguchi, new SonarSensor() blocks apparently
    sonarSensor = new SonarSensor(PORT_3);
    colorSensor = new FilteredColorSensor(PORT_2);
    gyroSensor  = new GyroSensor(PORT_4);
    leftMotor   = new FilteredMotor(PORT_C);
    rightMotor  = new FilteredMotor(PORT_B);
    armMotor    = new Motor(PORT_A);
    plotter     = new Plotter(leftMotor, rightMotor, gyroSensor);
    /* read profile file and make the profile object ready */
    prof        = new Profile("msad2022_pri/profile.txt");
    /* determine the course L or R */
    if (prof->getValueAsStr("COURSE") == "R") {
      _COURSE = -1;
    } else {
      _COURSE = 1;
    }
 
    /* FIR parameters for a low-pass filter with normalized cut-off frequency of 0.2
        using a function of the Hamming Window */
    const int FIR_ORDER = 4; 
    const double hn[FIR_ORDER+1] = { 7.483914270309116e-03, 1.634745733863819e-01, 4.000000000000000e-01, 1.634745733863819e-01, 7.483914270309116e-03 };
    /* set filters to FilteredColorSensor */
    Filter *lpf_r = new FIR_Transposed(hn, FIR_ORDER);
    Filter *lpf_g = new FIR_Transposed(hn, FIR_ORDER);
    Filter *lpf_b = new FIR_Transposed(hn, FIR_ORDER);
    colorSensor->setRawColorFilters(lpf_r, lpf_g, lpf_b);

    leftMotor->reset();
    srlfL = new SRLF(0.0);
    leftMotor->setPWMFilter(srlfL);
    leftMotor->setPWM(0);
    rightMotor->reset();
    srlfR = new SRLF(0.0);
    rightMotor->setPWMFilter(srlfR);
    rightMotor->setPWM(0);
    armMotor->reset();

/*
    === BEHAVIOR TREE DEFINITION STARTS HERE ===
    A Behavior Tree serves as a blueprint for a LEGO object while a Node class serves as each Lego block used in the object.
*/

    /* robot starts when touch sensor is turned on */
    tr_calibration = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            // temp fix 2022/6/20 W.Taniguchi, as no touch sensor available on RasPike
            //.decorator<BrainTree::UntilSuccess>()
            //    .leaf<IsTouchOn>()
            //.end()
            .leaf<ResetClock>()
        .end()
    .build();

/*
    DEFINE ROBOT BEHAVIOR AFTER START
    FOR THE RIGHT AND LEFT COURSE SEPARATELY

    if (prof->getValueAsStr("COURSE") == "R") {
    } else {
    }
*/ 

    /* BEHAVIOR FOR THE RIGHT COURSE STARTS HERE */
    if (prof->getValueAsStr("COURSE") == "R") {
      tr_run = nullptr;
      tr_slalom_first = nullptr;
      tr_slalom_check = nullptr;
      tr_slalom_second_a = nullptr;
      tr_slalom_second_b = nullptr;

    tr_block_r = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<SetArmPosition>(10, 40) 
                .leaf<IsTimeEarned>(500000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_BLUE) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-30,
                                       -80,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1700000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-50,
                                       -50,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(4000000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-35,
                                       -35,
                                       0.0)        
                .leaf<IsColorDetected>(CL_BLACK)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 黒線検知後、ライントレース準備
                .leaf<RunAsInstructed>(-30,
                                       60,
                                       0.0)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsTimeEarned>(1000000) // 黒線検知後、ライントレース準備
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .composite<BrainTree::MemSequence>()
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                    .leaf<IsColorDetected>(CL_WHITE) //グレー検知までライントレース    
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                .end()
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(48,
                                       48,0.0)   //グレー検知後、丸穴あき部分があるため少し前進    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(9500000)  // 本線ラインに戻ってくる
                .leaf<RunAsInstructed>(75,
                                       40,0.0)          
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000)  // 本線ラインに戻ってくる
                .leaf<RunAsInstructed>(40,
                                       40,0.0)     
            .end()
             .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000) // 本線ラインに戻ってくる。黒ラインか青ライン検知
                .leaf<RunAsInstructed>(40,
                                       40,0.0)     
                .leaf<IsColorDetected>(CL_BLACK)  
                .leaf<IsColorDetected>(CL_BLUE2)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(800000) // 検知後、斜め右前まで回転(ブロックを離さないように)
                .leaf<RunAsInstructed>(-55,
                                       60,0.0)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(800000) 
                .leaf<TraceLine>(38,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsColorDetected>(CL_WHITE)  
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(500000) // 全身しながら大きく左に向けて旋回。黄色を目指す。
                .leaf<RunAsInstructed>(-50,
                                       50,0.0)       
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsColorDetected>(CL_YELLOW)    // 黄色検知後、方向立て直す。
                .leaf<RunAsInstructed>(45,
                                       45,0.0)    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)   
                .leaf<IsTimeEarned>(700000) 
                .leaf<RunAsInstructed>(30,
                                       80,0.0) 
                .leaf<IsColorDetected>(CL_RED)   
            .end()
            .composite<BrainTree::ParallelSequence>(1,3) 
                .leaf<IsColorDetected>(CL_RED)  //赤検知までまっすぐ進む。
                .leaf<RunAsInstructed>(40,
                                       40,0.0)      
            .end()
            .leaf<StopNow>()
            .leaf<IsTimeEarned>(30000000) // wait 3 seconds
        .end()
        .build();

    tr_block_g = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsCeneterOn>() 
                .leaf<IsTimeEarned>(10000000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<SetArmPosition>(10, 40) 
                .leaf<IsTimeEarned>(500000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_BLUE) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-30,
                                       -80,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1700000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-50,
                                       -50,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(4000000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-35,
                                       -35,
                                       0.0)        
                .leaf<IsColorDetected>(CL_BLACK)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 黒線検知後、ライントレース準備
                .leaf<RunAsInstructed>(-30,
                                       60,
                                       0.0)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsTimeEarned>(1000000) // 黒線検知後、ライントレース準備
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .composite<BrainTree::MemSequence>()
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                    .leaf<IsColorDetected>(CL_WHITE) //グレー検知までライントレース    
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                .end()
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(45,
                                       45,0.0)   //グレー検知後、丸穴あき部分があるため少し前進    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(600000) // break after 10 seconds
                .leaf<RunAsInstructed>(-50,
                                       50,0.0) //左に旋回。ライントレース準備。
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000) //少し前進。ライントレース準備。
                .leaf<RunAsInstructed>(40,
                                       42,0.0)     
                 .leaf<IsColorDetected>(CL_BLACK)
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)  
                .leaf<IsColorDetected>(CL_BLUE2)  //純粋な青検知までライントレース
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(300000) // break after 10 seconds
                .leaf<RunAsInstructed>(44,
                                       -44,0.0)   //青検知後は大きく右に旋回    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(35,
                                       55,0.0)    //前進。次の青検知を目指す。
            .end() 
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0)   
                .leaf<IsColorDetected>(CL_BLUE2)  //前進。次の青検知を目指す。
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(580000) //青検知後、大きく右旋回。向きを整える。
                .leaf<RunAsInstructed>(60,
                                       -55,0.0)         
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsColorDetected>(CL_WHITE)  
                .leaf<TraceLine>(37, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(2000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0)  //目的の色検知まで前進
            .end() 
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0) 
                .leaf<IsColorDetected>(CL_GREEN)  
            .end()
            .leaf<StopNow>()
            .leaf<IsTimeEarned>(3000000) // wait 3 seconds
            .leaf<SetArmPosition>(10, 40)
        .end()
        .build();

      tr_block_b     = nullptr;
      tr_block_y     = nullptr;
      tr_block_d     = nullptr;

    } else { /* BEHAVIOR FOR THE LEFT COURSE STARTS HERE */
      tr_run = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::ParallelSequence>(1,2)
            .leaf<IsBackOn>()
            .composite<BrainTree::MemSequence>()
    //GATE1を通過後ラインの交差地点地点直前まで
                .composite<BrainTree::ParallelSequence>(2,2)
                   .leaf<IsColorDetected>(CL_JETBLACK_YMNK)//JETBLACKを検知
                   .leaf<IsDistanceEarned>(prof->getValueAsNum("DIST1"))
                   //.leaf<IsTimeEarned>(prof->getValueAsNum("TIME1"))
                   .leaf<StopNow>()
                   //.leaf<TraceLine>(prof->getValueAsNum("SPEED1"), 
                   //prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   //prof->getValueAsNum("I_CONST1"), 
                   //prof->getValueAsNum("D_CONST1"), 
                   //prof->getValueAsNum("srewrate1"), TS_OPPOSITE)//ライントレース1,右のライン検知
                .end()
    //ラインの交差地点直前から検知するまで減速
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsColorDetected>(CL_JETBLACK_YMNK)//JETBLACKを検知
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME1"))//18秒
                   .leaf<TraceLine>(prof->getValueAsNum("SPEED1a"), 
                   prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   prof->getValueAsNum("I_CONST1"), 
                   prof->getValueAsNum("D_CONST1"), 
                   prof->getValueAsNum("srewrate1"), TS_OPPOSITE)//ライントレース1,右のライン検知
                .end()
    //交差地点後にしばらく直進
                .composite<BrainTree::ParallelSequence>(2,2)
                 //.leaf<IsTimeEarned>(18000000)//18秒
                 //.leaf<StopNow>()
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME1a"))
                   .leaf<RunAsInstructed>(prof->getValueAsNum("POWER_L1a"),
                   prof->getValueAsNum("POWER_R1a"), 0.0)
                .end()
    //ゆるやかに右カーブ
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME1aa"))
                   .leaf<RunAsInstructed>(prof->getValueAsNum("POWER_L1aa"),
                   prof->getValueAsNum("POWER_R1aa"), prof->getValueAsNum("srewrate1aa"))
                .end()
    //ライン検知するまでさらに緩やかに右カーブ
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsColorDetected>(CL_BLACK)
                   .leaf<RunAsInstructed>(65,40, 0.0)
                .end()
    //ライン検知後にトレースを補正するために2秒速度を落とす
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME2"))
                   .leaf<TraceLine>(prof->getValueAsNum("SPEED2"),  
                   prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   prof->getValueAsNum("I_CONST1"), 
                   prof->getValueAsNum("D_CONST1"), 0.0, TS_NORMAL)//ライントレース2,左のライン検知
                .end()
    //ゲート2,3通過後にラインの交差点直前まで
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsColorDetected>(CL_JETBLACK_YMNK)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME2a"))
                   .leaf<TraceLine>(prof->getValueAsNum("SPEED2a"), 
                   prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   prof->getValueAsNum("I_CONST1"), 
                   prof->getValueAsNum("D_CONST1"), 0.0, TS_NORMAL)//ライントレース2a,左のライン検知
                .end()
    //ラインの交差点検知まで
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsColorDetected>(CL_JETBLACK_YMNK)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME2a"))
                   .leaf<TraceLine>(prof->getValueAsNum("SPEED2aa"), 
                   prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   prof->getValueAsNum("I_CONST1"), 
                   prof->getValueAsNum("D_CONST1"), 0.0, TS_NORMAL)//ライントレース2aa,左のライン検知
                .end()
    //ライン交差点検知後に緩やかに左カーブ
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME3"))
                   .leaf<RunAsInstructed>(prof->getValueAsNum("POWER_L3"),
                   prof->getValueAsNum("POWER_R3"), 0.0)
                .end()
    //ライン検知するまで緩やかに右カーブ
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME3a"))
                   .leaf<IsColorDetected>(CL_BLACK)
                   .leaf<RunAsInstructed>(prof->getValueAsNum("POWER_L4"),
                   prof->getValueAsNum("POWER_R4"), 0.0)
                .end()
    //ライン検知後にトレースを補正するために1.9秒速度を落とす
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME2"))
                   .leaf<TraceLine>(prof->getValueAsNum("SPEED2"),
                   prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   prof->getValueAsNum("I_CONST1"), 
                   prof->getValueAsNum("D_CONST1"), 0.0, TS_OPPOSITE)//ライントレース2,右のライン検知
                .end()
    //2回カーブまでライントレース
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME4"))
                   .leaf<TraceLine>(prof->getValueAsNum("SPEED4"),
                   prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   prof->getValueAsNum("I_CONST1"), 
                   prof->getValueAsNum("D_CONST1"), 0.0, TS_OPPOSITE)//ライントレース4,右のライン検知
                .end()
    //最終カーブまでライントレース
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsTimeEarned>(prof->getValueAsNum("TIME5"))
                   .leaf<TraceLine>(prof->getValueAsNum("SPEED5"),
                   prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   prof->getValueAsNum("I_CONST1"), 
                   prof->getValueAsNum("D_CONST1"), 
                   prof->getValueAsNum("srewrate3"), TS_OPPOSITE)//ライントレース5,右のライン検知
                .end()
    //スラロームに引き渡すまでライントレース
                .composite<BrainTree::ParallelSequence>(1,2)
                   .composite<BrainTree::MemSequence>()
                      .leaf<IsColorDetected>(CL_BLACK)
                      .leaf<IsColorDetected>(CL_BLUE)
                   .end()
                   .leaf<TraceLine>(prof->getValueAsNum("SPEED6"),
                   prof->getValueAsNum("GS_TARGET1"), prof->getValueAsNum("P_CONST1"), 
                   prof->getValueAsNum("I_CONST1"), 
                   prof->getValueAsNum("D_CONST1"), 0.0, TS_OPPOSITE)//ファイナルライントレース,右のライン検知
                .end()
            .end()
        .end()
    .build();

    tr_slalom_first = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::ParallelSequence>(1,2)
            .leaf<IsBackOn>()
            .composite<BrainTree::MemSequence>()
                // ライントレースから引継ぎして、直前の青線まで走る
                .composite<BrainTree::ParallelSequence>(1,2)
                   .leaf<IsTimeEarned>(1000000)
                   .leaf<TraceLine>(45, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                   .composite<BrainTree::MemSequence>()
                      .leaf<IsColorDetected>(CL_BLACK)
                      .leaf<IsColorDetected>(CL_BLUE)
                   .end()
                   .leaf<TraceLine>(35, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)
                .end()
                // 台にのる　勢いが必要
                .composite<BrainTree::ParallelSequence>(1,2)
//                    .leaf<IsDistanceEarned>(150)
                    .leaf<IsTimeEarned>(150000)
                    .leaf<RunAsInstructed>(70, 70, 0.0)
                .end()
/*
                // 勢いを殺す
                .composite<BrainTree::MemSequence>()
                    .leaf<StopNow>()
                    .leaf<IsTimeEarned>(200000) //sonar 0.2 sec
                .end()
*/           
/*
                .composite<BrainTree::ParallelSequence>(1,2)
                   .composite<BrainTree::MemSequence>()
                      .leaf<ClimbBoard>()
                   .end()
                   .leaf<TraceLine>(45, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)
                .end()
*/
                .composite<BrainTree::ParallelSequence>(1,2)//初期位置調整のために、台上で短距離ライントレース
                    .leaf<IsDistanceEarned>(30)
                    .leaf<TraceLine>(30, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)
                    //.leaf<RunAsInstructed>(40, 20, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)//第一スラローム開始
                    .leaf<IsDistanceEarned>(100)
                    .leaf<RunAsInstructed>(20, 50, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsDistanceEarned>(30)
                    .leaf<RunAsInstructed>(30, 30, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)//第二スラローム開始
                    .leaf<IsDistanceEarned>(120)
                    .leaf<RunAsInstructed>(60, 15, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsDistanceEarned>(30)
                    .leaf<RunAsInstructed>(15, 40, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsDistanceEarned>(10)
                    .leaf<RunAsInstructed>(30, 30, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsDistanceEarned>(70)
                    .leaf<RunAsInstructed>(40, 15, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)//黒検知したらライントレース
                    .leaf<IsColorDetected>(CL_BLACK)
                    .leaf<RunAsInstructed>(30, 20, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsDistanceEarned>(160)
                    .leaf<TraceLine>(30, 47, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)//ライントレースおもてなし
                    .leaf<IsDistanceEarned>(20)
                    .leaf<RunAsInstructed>(15, 50, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsDistanceEarned>(20)
                    .leaf<RunAsInstructed>(50, 15, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)//第三スラローム開始 ライントレース
                    .leaf<IsDistanceEarned>(160)
                    .leaf<TraceLine>(30, 47, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)//第三スラローム開始 ライントレース
                    //.leaf<IsDistanceEarned>(50)
                    .leaf<IsSonarOn>(500)//超音波センサー＆ライトレースによるチェックポイント
                    .leaf<TraceLine>(30, 47, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)//ガレージカードスラローム開始
                    .leaf<IsDistanceEarned>(90)
                    .leaf<RunAsInstructed>(50, 15, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsDistanceEarned>(80)
                    .leaf<RunAsInstructed>(30, 30, 0.0)
                .end()
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsDistanceEarned>(60)
                    .leaf<RunAsInstructed>(15, 50, 0.0)
                .end()
                // 色検知
                .composite<BrainTree::ParallelSequence>(1,2)
                    //.leaf<IsDistanceEarned>(50)
                    .leaf<IsColorDetected>(CL_BLUE_SL)
                    .leaf<IsColorDetected>(CL_RED_SL)
                    .leaf<IsColorDetected>(CL_YELLOW_SL)
                    .leaf<IsColorDetected>(CL_GREEN_SL)
                    .leaf<RunAsInstructed>(30, 30, 0.0)
                .end()
            .end()
        .end()
    .build();
    
    //台上転回後、センサーでコースパターン判定
    tr_slalom_check = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::ParallelSequence>(1,2)
            .leaf<IsBackOn>()
            .composite<BrainTree::MemSequence>()
                // 色検知 for test
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsColorDetected>(CL_BLUE_SL)
                    .leaf<IsColorDetected>(CL_RED_SL)
                    .leaf<IsColorDetected>(CL_YELLOW_SL)
                    .leaf<IsColorDetected>(CL_GREEN_SL)
                    .leaf<IsTimeEarned>(100000)
                .end()
                //move back
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsTimeEarned>(600000) //param SJ:700000,IS:550000,600000
                    .leaf<RunAsInstructed>(-40, -40, 0.0)
                .end()
                //rotate left with left wheel
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsTimeEarned>(300000) //param SJ:500000,IS:500000,200000,350000,300000
                    .leaf<RunAsInstructed>(-40, 0, 0.0) 
                .end()
                //move foward
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsTimeEarned>(450000) //param SJ:450000,IS:350000,450000
                    .leaf<RunAsInstructed>(50, 50, 0.0)
                .end()
                //turn left with right wheel
                .composite<BrainTree::ParallelSequence>(1,2)
                    .leaf<IsTimeEarned>(760000) //param SJ：1350000,IS:820000,760000
                    .leaf<RunAsInstructed>(0, 50, 0.0)
                .end()
                //detect the distance between the robot and plastic bottle using ultrasonic sensor
                //determine the arrangement pattern of plastic bottles from the distance
                //rotate right until sensor detects the distance or 2 second pass
                .composite<BrainTree::MemSequence>()
                    .leaf<StopNow>()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(500000) //sonar 0.5 sec
                        .leaf<DetectSlalomPattern>()
                        .leaf<RunAsInstructed>(0, 35, 0.0)
                    .end()
                .end()
                //able to see detected
                .composite<BrainTree::MemSequence>()
                    .leaf<StopNow>()
                    .leaf<IsTimeEarned>(500000) //wait 0.5 sec
                .end()
            .end()
        .end()
    .build();

    tr_slalom_second_a = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,2) //後半第一スラローム開始
                .leaf<IsDistanceEarned>(30)
                .leaf<RunAsInstructed>(-40, 0, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(50)
                .leaf<RunAsInstructed>(40, 40, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(20)
                .leaf<RunAsInstructed>(-40, 0, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(50)
                .leaf<RunAsInstructed>(40, 40, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2) //後半第二スラローム開始
                .leaf<IsDistanceEarned>(200)
                .leaf<RunAsInstructed>(50, 15, 0.0)
            .end()
        .end()
    .build();

    tr_slalom_second_b = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,2) //後半第一スラローム開始
                .leaf<IsDistanceEarned>(50)
                .leaf<RunAsInstructed>(40, 40, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(110)    //param SJ:100
                .leaf<RunAsInstructed>(20, 50, 0.0) //param SJ:20,50
            .end()
            .composite<BrainTree::ParallelSequence>(1,2) //後半第二スラローム開始
                .leaf<IsDistanceEarned>(20)
                .leaf<RunAsInstructed>(40, 40, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2) //カーブを分割すると綺麗になる
                .leaf<IsDistanceEarned>(40)
                .leaf<RunAsInstructed>(20, 50, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(250)
                .leaf<RunAsInstructed>(40, 40, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(120)    //param IS:150
                .leaf<RunAsInstructed>(50, 20, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(30)
                .leaf<RunAsInstructed>(40, 40, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(100)    //param IS:150
                .leaf<RunAsInstructed>(50, 20, 0.0)
            .end()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsDistanceEarned>(100)
                .leaf<RunAsInstructed>(40, 40, 0.0)
            .end()
        .end()
    .build();

    tr_block_r = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<SetArmPosition>(10, 40) 
                .leaf<IsTimeEarned>(500000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_BLUE) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-30,
                                       -80,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1700000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-50,
                                       -50,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(4000000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-35,
                                       -35,
                                       0.0)        
                .leaf<IsColorDetected>(CL_BLACK)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 黒線検知後、ライントレース準備
                .leaf<RunAsInstructed>(-30,
                                       60,
                                       0.0)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsTimeEarned>(1000000) // 黒線検知後、ライントレース準備
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .composite<BrainTree::MemSequence>()
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                    .leaf<IsColorDetected>(CL_WHITE) //グレー検知までライントレース    
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                .end()
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(46,
                                       46,0.0)   //グレー検知後、丸穴あき部分があるため少し前進    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000)  // 本線ラインに戻ってくる
                .leaf<RunAsInstructed>(75,
                                       40,0.0)          
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000)  // 本線ラインに戻ってくる
                .leaf<RunAsInstructed>(40,
                                       40,0.0)     
            .end()
             .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000) // 本線ラインに戻ってくる。黒ラインか青ライン検知
                .leaf<RunAsInstructed>(40,
                                       40,0.0)     
                .leaf<IsColorDetected>(CL_BLACK)  
                .leaf<IsColorDetected>(CL_BLUE2)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(800000) // 検知後、斜め右前まで回転(ブロックを離さないように)
                .leaf<RunAsInstructed>(-55,
                                       60,0.0)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1500000) 
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsColorDetected>(CL_WHITE)  
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(500000) // 全身しながら大きく左に向けて旋回。黄色を目指す。
                .leaf<RunAsInstructed>(-55,
                                       55,0.0)       
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsColorDetected>(CL_YELLOW)    // 黄色検知後、方向立て直す。
                .leaf<RunAsInstructed>(45,
                                       45,0.0)    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)   
                .leaf<IsTimeEarned>(800000) 
                .leaf<RunAsInstructed>(30,
                                       80,0.0) 
                .leaf<IsColorDetected>(CL_RED)   
            .end()
            .composite<BrainTree::ParallelSequence>(1,3) 
                .leaf<IsColorDetected>(CL_RED)  //赤検知までまっすぐ進む。
                .leaf<RunAsInstructed>(40,
                                       40,0.0)      
            .end()
            .leaf<StopNow>()
            .leaf<IsTimeEarned>(30000000) // wait 3 seconds
        .end()
        .build();

    tr_block_g = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<SetArmPosition>(10, 40) 
                .leaf<IsTimeEarned>(500000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_BLUE) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-30,
                                       -80,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1700000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-50,
                                       -50,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(4000000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-35,
                                       -35,
                                       0.0)        
                .leaf<IsColorDetected>(CL_BLACK)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 黒線検知後、ライントレース準備
                .leaf<RunAsInstructed>(-30,
                                       60,
                                       0.0)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsTimeEarned>(1500000) // 黒線検知後、ライントレース準備
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .composite<BrainTree::MemSequence>()
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                    .leaf<IsColorDetected>(CL_WHITE) //グレー検知までライントレース    
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                .end()
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(45,
                                       45,0.0)   //グレー検知後、丸穴あき部分があるため少し前進    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(600000) // break after 10 seconds
                .leaf<RunAsInstructed>(-50,
                                       50,0.0) //左に旋回。ライントレース準備。
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000) //少し前進。ライントレース準備。
                .leaf<RunAsInstructed>(40,
                                       42,0.0)     
                 .leaf<IsColorDetected>(CL_BLACK)
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)  
                .leaf<IsColorDetected>(CL_BLUE2)  //純粋な青検知までライントレース
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(300000) // break after 10 seconds
                .leaf<RunAsInstructed>(44,
                                       -44,0.0)   //青検知後は大きく右に旋回    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(35,
                                       55,0.0)    //前進。次の青検知を目指す。
            .end() 
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0)   
                .leaf<IsColorDetected>(CL_BLUE2)  //前進。次の青検知を目指す。
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(800000) //青検知後、大きく右旋回。向きを整える。
                .leaf<RunAsInstructed>(60,
                                       -55,0.0)         
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsColorDetected>(CL_WHITE)  
                .leaf<TraceLine>(37, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(2000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0)  //目的の色検知まで前進
            .end() 
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0) 
                .leaf<IsColorDetected>(CL_GREEN)  
            .end()
            .leaf<StopNow>()
            .leaf<IsTimeEarned>(3000000) // wait 3 seconds
            .leaf<SetArmPosition>(10, 40)
        .end()
        .build();

    tr_block_b = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<SetArmPosition>(10, 40) 
                .leaf<IsTimeEarned>(500000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_BLUE) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-30,
                                       -80,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1700000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-50,
                                       -50,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(4000000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-35,
                                       -35,
                                       0.0)        
                .leaf<IsColorDetected>(CL_BLACK)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 黒線検知後、ライントレース準備
                .leaf<RunAsInstructed>(-30,
                                       60,
                                       0.0)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsTimeEarned>(1000000) // 黒線検知後、ライントレース準備
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .composite<BrainTree::MemSequence>()
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                    .leaf<IsColorDetected>(CL_WHITE) //グレー検知までライントレース    
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                .end()
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(45,45,0.0)  //グレー検知後、丸穴あき部分があるため少し前進    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsAngleSmaller>(-14)
                .leaf<RunAsInstructed>(-50,50,0.0) //左に旋回。ライントレース準備。
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000) //少し前進。ライントレース準備。
                .leaf<RunAsInstructed>(40,42,0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<TraceLine>(40, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)  
                .leaf<IsColorDetected>(CL_BLUE2)  //純粋な青検知までライントレース
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsAngleLarger>(1)
                .leaf<RunAsInstructed>(44,-44,0.0) //青検知後は大きく右に旋回    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000)
                .leaf<RunAsInstructed>(35,55,0.0)   //前進。次の青検知を目指す。
            .end() 
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<RunAsInstructed>(40,40,0.0)   
                .leaf<IsColorDetected>(CL_BLUE2)  //前進。次の青検知を目指す。
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsAngleLarger>(70)
                .leaf<RunAsInstructed>(60,-55,0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsColorDetected>(CL_WHITE)  
                .leaf<TraceLine>(37, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(2000000)
                .leaf<RunAsInstructed>(40,40,0.0)  //目的の色検知まで前進
            .end() 
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<RunAsInstructed>(40,40,0.0) 
                .leaf<IsColorDetected>(CL_BLUE2)  
            .end()
            .leaf<StopNow>()
            .leaf<IsTimeEarned>(30000000) // wait 3 seconds
            .leaf<SetArmPosition>(10, 40)
        .end()
    .build();

    tr_block_y = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<SetArmPosition>(10, 40) 
                .leaf<IsTimeEarned>(500000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_BLUE) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(975000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-30,
                                       -80,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1700000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-50,
                                       -50,
                                       0.0)      
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(4000000) // 後ろ向き走行。狙いは黒線。
                .leaf<RunAsInstructed>(-35,
                                       -35,
                                       0.0)        
                .leaf<IsColorDetected>(CL_BLACK)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(900000) // 黒線検知後、ライントレース準備
                .leaf<RunAsInstructed>(-30,
                                       55,
                                       0.0)     
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(35,
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsTimeEarned>(1500000) // 黒線検知後、ライントレース準備
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .composite<BrainTree::MemSequence>()
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                    .leaf<IsColorDetected>(CL_WHITE) //グレー検知までライントレース    
                    .leaf<IsColorDetected>(CL_GRAY) //グレー検知までライントレース   
                .end()
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(45,
                                       45,0.0)   //グレー検知後、丸穴あき部分があるため少し前進    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(600000) // break after 10 seconds
                .leaf<RunAsInstructed>(-50,
                                       50,0.0) //左に旋回。ライントレース準備。
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000) //少し前進。ライントレース準備。
                .leaf<RunAsInstructed>(40,
                                       42,0.0)     
                 .leaf<IsColorDetected>(CL_BLACK)
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<TraceLine>(40, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)  
                .leaf<IsColorDetected>(CL_BLUE2)  //純粋な青検知までライントレース
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(300000) // break after 10 seconds
                .leaf<RunAsInstructed>(44,
                                       -44,0.0)   //青検知後は大きく右に旋回    
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(1000000) // break after 10 seconds
                .leaf<RunAsInstructed>(35,
                                       55,0.0)    //前進。次の青検知を目指す。
            .end() 
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0)   
                .leaf<IsColorDetected>(CL_BLUE2)  //前進。次の青検知を目指す。
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(620000) //青検知後、大きく右旋回。向きを整える。
                .leaf<RunAsInstructed>(55,
                                       -50,0.0)         
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsColorDetected>(CL_WHITE)  
                .leaf<TraceLine>(37, 
                                 GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_OPPOSITE)  
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(2000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0)  //目的の色検知まで前進
            .end() 
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<IsTimeEarned>(5000000)
                .leaf<RunAsInstructed>(40,
                                       40,0.0) 
                .leaf<IsColorDetected>(CL_YELLOW)  
            .end()
            .leaf<StopNow>()
            .leaf<IsTimeEarned>(30000000) // wait 3 seconds
            .leaf<SetArmPosition>(10, 40)
        .end()
        .build();

    // テストでの値取得用
    tr_block_d = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<SetArmPosition>(10, 40) 
                .leaf<IsTimeEarned>(500000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(40, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsTimeEarned>(1000000)
            .end()
            .leaf<StopNow>()
            .leaf<IsTimeEarned>(30000000) // wait 3 seconds
        .end()
    .build();

    tr_block_d2 = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<SetArmPosition>(10, 40) 
                .leaf<IsTimeEarned>(500000) 
            .end()
            .composite<BrainTree::ParallelSequence>(1,3)
                .leaf<TraceLine>(0, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0, TS_NORMAL)  
                .leaf<IsTimeEarned>(10000000) 
            .end()
            .leaf<StopNow>()
            .leaf<IsTimeEarned>(30000000) // wait 3 seconds
        .end()
    .build();


    } /* if (prof->getValueAsStr("COURSE") == "R") */

/*
    === BEHAVIOR TREE DEFINITION ENDS HERE ===
*/

    /* register cyclic handler to EV3RT */
    sta_cyc(CYC_UPD_TSK);

    /* indicate initialization completion by LED color */
    _log("initialization completed.");
    ev3_led_set_color(LED_ORANGE);
    state = ST_CALIBRATION;

    /* the main task sleep until being waken up and let the registered cyclic handler to traverse the behavir trees */
    _log("going to sleep...");
    ER ercd = slp_tsk();
    assert(ercd == E_OK);
    if (ercd != E_OK) {
        syslog(LOG_NOTICE, "slp_tsk() returned %d", ercd);
    }

    /* deregister cyclic handler from EV3RT */
    stp_cyc(CYC_UPD_TSK);
    /* destroy behavior tree */
    delete tr_block_r;
    delete tr_block_g;
    delete tr_block_b;
    delete tr_block_y;
    delete tr_block_d;
    delete tr_run;
    delete tr_slalom_first;
    delete tr_slalom_check;
    delete tr_slalom_second_a;
    delete tr_slalom_second_b;
    delete tr_calibration;
    /* destroy profile object */
    delete prof;
    /* destroy EV3 objects */
    delete lpf_b;
    delete lpf_g;
    delete lpf_r;
    delete plotter;
    delete armMotor;
    delete rightMotor;
    delete leftMotor;
    delete gyroSensor;
    delete colorSensor;
    delete sonarSensor;
    delete touchSensor;
    delete ev3clock;
    _log("being terminated...");
    // temp fix 2022/6/20 W.Taniguchi, as Bluetooth not implemented yet
    //fclose(bt);
#if defined(MAKE_SIM)    
    ETRoboc_notifyCompletedToSimulator();
#endif
    ext_tsk();
}

/* periodic task to update the behavior tree */
void update_task(intptr_t unused) {
    BrainTree::Node::Status status;
    ER ercd;

    colorSensor->sense();
    rgb_raw_t cur_rgb;
    colorSensor->getRawColor(cur_rgb);

    // for test
    plotter->plot();

    int32_t distance = plotter->getDistance();
    int16_t azimuth = plotter->getAzimuth();
    int16_t degree = plotter->getDegree();
    int32_t locX = plotter->getLocX();
    int32_t locY = plotter->getLocY();
    int32_t ang = plotter->getAngL();
    int32_t angR = plotter->getAngR();

    int32_t sonarDistance = sonarSensor->getDistance();

    _log("r=%d g=%d b=%d",cur_rgb.r,cur_rgb.g,cur_rgb.b);

    _log("dist=%d azi=%d deg=%d locX=%d locY=%d ang=%d angR=%d",distance,azimuth,degree,locX,locY,ang,angR);
    _log("sonar=%d",sonarDistance);
    
/*
    === STATE MACHINE DEFINITION STARTS HERE ===
    The robot behavior is defined using HFSM (Hierarchical Finite State Machine) with two hierarchies as a whole where:
    - The upper layer is implemented as a state machine here.
    - The lower layer is implemented using Behavior Tree where each tree gets traversed within each corresponding state of the state machine.
*/
    switch (state) {
    case ST_CALIBRATION:
        if (tr_calibration != nullptr) {
            status = tr_calibration->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
                switch (JUMP_CALIBRATION) { /* JUMP_CALIBRATION = 1... is for testing only */
                    case 1:
                        state = ST_SLALOM_FIRST;
                        _log("State changed: ST_CALIBRATION to ST_SLALOM_FIRST");
                        break;
                    case 2:
                        state = ST_SLALOM_CHECK;
                        _log("State changed: ST_CALIBRATION to ST_SLALOM_CHECK");
                        break;
                    case 3:
                        state = ST_SLALOM_SECOND_A;
                        _log("State changed: ST_CALIBRATION to ST_SLALOM_SECOND_A");
                        break;
                    case 4:
                        state = ST_SLALOM_SECOND_B;
                        _log("State changed: ST_CALIBRATION to ST_SLALOM_SECOND_B");
                        break;
                    case 5:
                        state = ST_BLOCK_R;
                        _log("State changed: ST_CALIBRATION to ST_BLOCK_R");
                        break;
                    case 6:
                        state = ST_BLOCK_G;
                        _log("State changed: ST_CALIBRATION to ST_BLOCK_G");
                        break;
                    case 7:
                        state = ST_BLOCK_B;
                        _log("State changed: ST_CALIBRATION to ST_BLOCK_B");
                        break;
                    case 8:
                        state = ST_BLOCK_Y;
                        _log("State changed: ST_CALIBRATION to ST_BLOCK_Y");
                        break;
                    case 9:
                        state = ST_BLOCK_D;
                        _log("State changed: ST_CALIBRATION to ST_BLOCK_D");
                        break;
                    case 10:
                        state = ST_BLOCK_D2;
                        _log("State changed: ST_CALIBRATION to ST_BLOCK_D");
                        break;
                    default:
                        state = ST_RUN;
                        _log("State changed: ST_CALIBRATION to ST_RUN");
                        break;
                }
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_CALIBRATION to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_RUN:
        if (tr_run != nullptr) {
            status = tr_run->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
                state = ST_SLALOM_FIRST;
                _log("State changed: ST_RUN to ST_SLALOM_FIRST");
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_RUN to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_SLALOM_FIRST:
        if (tr_slalom_first != nullptr) {
            status = tr_slalom_first->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
                state = ST_SLALOM_CHECK;
                _log("State changed: ST_SLALOM_FIRST to ST_SLALOM_CHECK");
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_SLALOM_FIRST to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_SLALOM_CHECK:
        if (tr_slalom_check != nullptr) {
            status = tr_slalom_check->update();
            switch (status) {       
            case BrainTree::Node::Status::Success:
                if (DetectSlalomPattern::isSlalomPatternA == true) {
                    // for test
                    if (JUMP_SLALOM == true) {
                        _log("test only ST_SLALOM_CHECK.");
                        if (DetectSlalomPattern::earnedDistance == 0) {
                            _log("Failed to check slalom pattern.");
                        }
                            state = ST_ENDING;
                            _log("Distance %d is detected by sonar and chose pattern A.", DetectSlalomPattern::earnedDistance);
                            _log("State changed: ST_SLALOM_CHECK to ST_ENDING");
                    } else {
                        if (DetectSlalomPattern::earnedDistance == 0) {
                            _log("Failed to check slalom pattern.");
                        }
                        state = ST_SLALOM_SECOND_A;
                        _log("Distance %d is detected by sonar and chose pattern A.", DetectSlalomPattern::earnedDistance);
                        _log("State changed: ST_SLALOM_CHECK to ST_SLALOM_SECOND_A");
                    }
                } else {
                    // for test
                    if (JUMP_SLALOM == true) {
                        _log("test only ST_SLALOM_CHECK.");
                        state = ST_ENDING;
                        _log("Distance %d is detected by sonar and chose pattern B.", DetectSlalomPattern::earnedDistance);
                        _log("State changed: ST_SLALOM_CHECK to ST_ENDING");
                    } else {
                        state = ST_SLALOM_SECOND_B;
                        _log("Distance %d is detected by sonar and chose pattern B.", DetectSlalomPattern::earnedDistance);
                        _log("State changed: ST_SLALOM_CHECK to ST_SLALOM_SECOND_B");
                    }
                }
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_SLALOM_CHECK to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_SLALOM_SECOND_A:
        if (tr_slalom_second_a != nullptr) {
            status = tr_slalom_second_a->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
                switch (JUMP_BLOCK) { /* JUMP_BLOCK = 1... is for testing only */
                    case 1:
                        state = ST_BLOCK_R;
                        _log("State changed: ST_SLALOM_SECOND_A to ST_BLOCK_R");
                        break;
                    case 2:
                        state = ST_BLOCK_G;
                        _log("State changed: ST_SLALOM_SECOND_A to ST_BLOCK_G");
                        break;
                    case 3:
                        state = ST_BLOCK_B;
                        _log("State changed: ST_SLALOM_SECOND_A to ST_BLOCK_B");
                        break;
                    case 4:
                        state = ST_BLOCK_Y;
                        _log("State changed: ST_SLALOM_SECOND_A to ST_BLOCK_Y");
                        break;
                    case 5:
                        state = ST_ENDING;
                        _log("State changed: ST_SLALOM_SECOND_A to ST_ENDING");
                        break;
                    default:
                        state = ST_BLOCK_D;
                        _log("State changed: ST_SLALOM_SECOND_A to ST_BLOCK_D");
                        break;
                }
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_SLALOM_SECOND_A to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_SLALOM_SECOND_B:
        if (tr_slalom_second_b != nullptr) {
            status = tr_slalom_second_b->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
                switch (JUMP_BLOCK) { /* JUMP_BLOCK = 1... is for testing only */
                    case 1:
                        state = ST_BLOCK_R;
                        _log("State changed: ST_SLALOM_SECOND_B to ST_BLOCK_R");
                        break;
                    case 2:
                        state = ST_BLOCK_G;
                        _log("State changed: ST_SLALOM_SECOND_B to ST_BLOCK_G");
                        break;
                    case 3:
                        state = ST_BLOCK_B;
                        _log("State changed: ST_SLALOM_SECOND_B to ST_BLOCK_B");
                        break;
                    case 4:
                        state = ST_BLOCK_Y;
                        _log("State changed: ST_SLALOM_SECOND_B to ST_BLOCK_Y");
                        break;
                    case 5:
                        state = ST_ENDING;
                        _log("State changed: ST_SLALOM_SECOND_B to ST_ENDING");
                        break;
                    default:
                        state = ST_BLOCK_D;
                        _log("State changed: ST_SLALOM_SECOND_B to ST_BLOCK_D");
                        break;
                }
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_SLALOM_SECOND_B to ST_ENDING");
                break;
            default:
                break;
            }
        }
    case ST_BLOCK_R:
        if (tr_block_r != nullptr) {
            status = tr_block_r->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_BLOCK_R to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_BLOCK_G:
        if (tr_block_g != nullptr) {
            status = tr_block_g->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_BLOCK_G to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_BLOCK_B:
        if (tr_block_b != nullptr) {
            status = tr_block_b->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_BLOCK_B to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_BLOCK_Y:
        if (tr_block_y != nullptr) {
            status = tr_block_y->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_BLOCK_Y to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_BLOCK_D:
        if (tr_block_d != nullptr) {
            status = tr_block_d->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_BLOCK_D to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_BLOCK_D2:
        if (tr_block_d2 != nullptr) {
            status = tr_block_d2->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
            case BrainTree::Node::Status::Failure:
                state = ST_ENDING;
                _log("State changed: ST_BLOCK_D to ST_ENDING");
                break;
            default:
                break;
            }
        }
        break;
    case ST_ENDING:
        _log("waking up main...");
        /* wake up the main task */
        ercd = wup_tsk(MAIN_TASK);
        assert(ercd == E_OK);
        if (ercd != E_OK) {
            syslog(LOG_NOTICE, "wup_tsk() returned %d", ercd);
        }
        state = ST_END;
        _log("State changed: ST_ENDING to ST_END");
        break;    
    case ST_INITIAL:    /* do nothing */
    case ST_END:        /* do nothing */
    default:            /* do nothing */
        break;
    }
/*
    === STATE MACHINE DEFINITION ENDS HERE ===
*/

    rightMotor->drive();
    leftMotor->drive();

    //logger->outputLog(LOG_INTERVAL);
}