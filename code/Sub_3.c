/*
 * Subject_3.c
 *
 *  Created on: 2026年5月30日
 *      Author: Shelton Liu
 *
 *  科目3：科目1的3环串级控制 + 惯导角度 + 弯道自动减速
 *    控制：balance_mode_parameter(3) — 有角速度环
 *    角度：来自 INS（trajectory_replay 更新 Target_angle）
 *    速度：直道 SPEED_MAX，弯道自动降到 SPEED_MIN
 */

#include "zf_common_headfile.h"

/* ===== 声明外部变量 ===== */
// 告诉编译器，forward 在 Ins.c 中定义过，请直接跨文件调用它
extern int forward;

/* ===== 开环动力与减速参数 ===== */
#define OPEN_PWM_MAX   1450.0f  // 冲坡、直道、草地的全速开环PWM
#define OPEN_PWM_MIN  1150.0f   // 掉头区急弯保底PWM (如果过弯依然外翻，可继续降低)
//#define OPEN_PWM_MID  930.0f   // 掉头区急弯保底PWM (如果过弯依然外翻，可继续降低)
#define OPEN_PWM_CONE  1300.0f   // 锥桶区速度
#define PWM_STEP       15.0f    // PWM斜坡步长（调节加减速的平滑度，防起步摔倒）

void Body_ctrl_3(void)
{
    static bool s_is_init = false;
    static float s_current_pwm = 0.0f; // 实际下发给电机的平滑PWM

    /* ==================================================================
     * 1. 发车初始化阶段 (防起步打转与暴走)
     * ================================================================== */
    if (!s_is_init)
    {
        balance_mode_parameter(3);            // 科目3参数（有角速度环）

        // 强制对齐起步姿态，消除初始手放偏差引起的猛烈打舵
        Target_angle = roll_balance_cascade.posture_value.yaw;

        s_current_pwm = 0.0f;
        s_is_init = true;
    }

    Imu_lowpass_filter();                   // IMU数据一阶低通滤波

    /* ==================================================================
     * 2. 弯道自动减速与【路程状态机动态调参】 (开环版本)
     * ================================================================== */
    float target_expected_pwm = OPEN_PWM_MAX;
    float progress_ratio = 0.0f;

    // 计算当前的赛道进度比例 (范围 0.0 ~ 1.0)
    // 增加分母为0的保护机制
    if (trajectory_index > 0)
    {
        progress_ratio = (float)trajectory_new_cnt / (float)trajectory_index;
    }

    // 【路段 A：掉头区】：只有跑过了赛道全程的 40%，才允许启用弯道减速！
    if (progress_ratio > 0.42f)
    {
        float turn_err = fabsf(AngleErrorNormalize(Target_angle - roll_balance_cascade.posture_value.yaw));

        // --- 1. 速度干预 ---
        if(turn_err > 20.0f)
        {
            target_expected_pwm = OPEN_PWM_MIN; // 急弯降速至保底PWM
        }
        else if(turn_err > 4.0f) // 【死区放大】：大于4度才开始减速，过滤草地微小抖动
        {
            // 4度~20度之间，PWM线性平滑下降
            target_expected_pwm = OPEN_PWM_MAX - (OPEN_PWM_MAX - OPEN_PWM_MIN) * (turn_err - 4.0f) / 16.0f;
        }

        // --- 2. 算法参数干预 (专治提前转弯与出弯画龙) ---
        // 注意：0-40% 期间用的是你在 menu 里设定的直道参数 (如 forward=4, step=8, kp=15)
//        forward = 1;  // 【缩短前瞻】：消除“抄近道”，让车死死咬住弯道入口再转！
        step = 6;     // 【微收手速】：允许相对快速打方向防止外抛，但比直道略收敛
        stl_kp = 15;  // 【降火脾气】：方向盘打得温柔点，专治出弯摇摆！
    }

    // 【路段 B：新增出弯全速恢复区】 55% - 65%
        // 核心思想：在此区间，不要急着减速！把速度恢复到 MAX (或者保持直道大动力)！
        // 强大的前行惯性会产生天然的陀螺仪效应，帮助车身迅速“拔直”站稳。
//        if (progress_ratio > 0.50f && progress_ratio <= 0.65f)
//        {
//            target_expected_pwm = OPEN_PWM_MID; // 保持 1100 匀速稳定出弯
//
//                    forward = 1;  // 【精妙微调】前瞻稍微放远到 2，引导车子拉出一条平滑的直线弧度
//                    step = 8;     // 【限制回正速率】手速放慢，防止出弯方向盘打得太碎扯倒车身
//                    stl_kp = 20;   // 保持温柔脾气
//        }

        // 【路段 C：锥桶区】 72% 之后
        // 此时车身已经在直道上完全站直了，再安全地切入绕锥模式
//        if (progress_ratio > 0.65f)
//        {
//            target_expected_pwm = OPEN_PWM_CONE; // 安全减速到 750
//
//            forward = 1;
//            step = 6;
//            stl_kp = 10;
//        }

    /* ==================================================================
     * 3. 三环串级控制 (姿态与寻迹)
     * ================================================================== */
    if(sys_times % 1 == 0) // 角速度环
    {
        if(Imu_type == 1)      pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu660ra_gyro_x);
        else if(Imu_type == 2) pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu660rb_gyro_y);
        else if(Imu_type == 3) pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu963ra_gyro_y);
    }

    if(sys_times % 5 == 0) // 角度环
    {
        pid_control(&roll_balance_cascade.angle_cycle, roll_balance_cascade.turn_cycle.out - roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);
    }

    if(sys_times % 20 == 0) // 转向环 (依靠惯导Target_angle寻迹)
    {
        pid_control(&roll_balance_cascade.turn_cycle, AngleErrorNormalize(Target_angle - roll_balance_cascade.posture_value.yaw), 0);
    }

    Steer_set(SERVO_MOTOR_MID + roll_balance_cascade.angular_speed_cycle.out);

    /* ==================================================================
     * 4. 绝对开环动力输出 (带 PWM 斜坡缓冲防摔)
     * ================================================================== */
    if(trajectory_new_cnt >= trajectory_index) // 终点停车
    {
        CYT2_S_motor_ctrl(0);            // 停车，PWM归0
        s_is_init = false;               // 复位状态机，为下次发车做准备
        s_current_pwm = 0.0f;
    }
    else // 行驶中
    {
        // PWM 斜坡缓冲逻辑：让实际 PWM 平滑追赶目标 PWM
        if(s_current_pwm < target_expected_pwm)
        {
            s_current_pwm += PWM_STEP;
            if(s_current_pwm > target_expected_pwm) s_current_pwm = target_expected_pwm;
        }
        else if(s_current_pwm > target_expected_pwm)
        {
            s_current_pwm -= PWM_STEP; // 入弯减速缓冲
            if(s_current_pwm < target_expected_pwm) s_current_pwm = target_expected_pwm;
        }

        // 调用开环电机函数！强转为 int 型
        CYT2_S_motor_ctrl((int)s_current_pwm);
    }
}
