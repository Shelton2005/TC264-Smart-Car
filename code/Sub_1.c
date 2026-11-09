/*
 * Subject_1.c
 *
 *  Created on: 2026年4月1日
 *      Author: Shelton Liu
 */

#include "zf_common_headfile.h"

/* 弯道自动减速参数 */
//#define SPEED_MAX   80
//#define SPEED_MIN   45

#define SPEED_MAX   798
#define SPEED_MIN   439

uint32 Mode_change=0;

uint32 sys_times=0;
float  End_error=0;
float  Target_angle=0;



void Body_ctrl_1(void)
{
    static int32 I=0;   //系统计时 (ms)
    I++;


    static bool once = false;     //保证只赋值一次
    if (!once)
    {
      Target_angle=0;              //起始角度为0
      balance_mode_parameter(1);   //分配科目一参数
      once = true;
    }


    Imu_lowpass_filter();       //IMU数据一阶低通滤波


    /* ===== 弯道自动减速 ===== */

    float turn_err = fabsf(AngleErrorNormalize(Target_angle - roll_balance_cascade.posture_value.yaw));
    float speed_target = SPEED_MAX;
    if(turn_err > 20.0f)
    {
        speed_target = SPEED_MIN;                           // 误差>20° → 降到最低速
    }
    else if(turn_err > 0.5f)
    {
        speed_target = SPEED_MAX - (SPEED_MAX - SPEED_MIN) * (turn_err - 0.5f) / 19.5f;
    }
    // 误差<0.5° → 直道，全速
//    CYT2_S_motor_loop_ctrl(speed_target);      //闭环科目一速度
    CYT2_S_motor_ctrl(1450);

    if(I>=2000)                 //改变I的数值即可改变课目一回头时间
    {
        Target_angle=180;
//        gpio_set_level(Buzzer_pin,1);
    }


    if(sys_times%1==0)//角速度环
    {

        if(Imu_type==1)//在选择IMU型号的时候就已经确定
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu660ra_gyro_x);
        }
        else if(Imu_type==2)
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu660rb_gyro_y);
        }
        else if(Imu_type==3)
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu963ra_gyro_y);
        }

    }

    if(sys_times%5==0)//角度环
    {
          pid_control(&roll_balance_cascade.angle_cycle, roll_balance_cascade.turn_cycle.out-roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);

    }

    if(sys_times%20==0)//转向环
    {
//        pid_control(&roll_balance_cascade.turn_cycle, StepApproach(Target_angle,2), roll_balance_cascade.posture_value.yaw);

//            if(sys_times>150)
//            {
//                pid_control(&roll_balance_cascade.turn_cycle, AngleErrorNormalize(StepApproach(Target_angle,4)-roll_balance_cascade.posture_value.yaw),0);
//            }
//            else
            {
                pid_control(&roll_balance_cascade.turn_cycle, AngleErrorNormalize(StepApproach(Target_angle,2)-roll_balance_cascade.posture_value.yaw),0);
            }


    }


         Steer_set(SERVO_MOTOR_MID +roll_balance_cascade.angular_speed_cycle.out);


}


void Body_keep(void)//只依靠角速度和角度维持车头随动
{
    static int32 I=0;   //系统计时
    I++;


    static bool once = false;     //保证只赋值一次
    if (!once)
    {
      balance_mode_parameter(1);   //分配科目一参数
      once = true;
    }


    Imu_lowpass_filter();       //IMU数据一阶低通滤波



    if(sys_times%1==0)//角速度环
    {

        if(Imu_type==1)//在选择IMU型号的时候就已经确定
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu660ra_gyro_x);
        }
        else if(Imu_type==2)
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu660rb_gyro_y);
        }
        else if(Imu_type==3)
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu963ra_gyro_y);
        }

    }

    if(sys_times%5==0)//角度环
    {
            pid_control(&roll_balance_cascade.angle_cycle, 0-roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);

//          pid_control(&roll_balance_cascade.angle_cycle, roll_balance_cascade.turn_cycle.out-roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);

    }



         Steer_set(SERVO_MOTOR_MID +roll_balance_cascade.angular_speed_cycle.out);

}


void Balance_1_text(void)
{
//    ips200_draw_line(0, 16*11, 239, 16*11, RGB565_RED);
    ips_show_string(8*0, 16*12, "Yaw:");      ips_show_float(8*10,16*12, roll_balance_cascade.posture_value.yaw,3,6);
    ips_show_string(8*0, 16*13, "Roll:");     ips_show_float(8*10,16*13, roll_balance_cascade.posture_value.rol,3,6);
//    ips_show_string(8*0, 16*14, "A_OUT:");    ips_show_float(8*10,16*14, roll_balance_cascade.angle_cycle.out,3,6);
//    ips_show_string(8*0, 16*15, "G_out");     ips_show_float(8*10,16*15, roll_balance_cascade.angular_speed_cycle.out,3,6);
//    ips_show_string(8*0, 16*16, "Taget_A");   ips_show_float(8*10,16*16, Target_angle,3,6);
    ips_show_string(8*0, 16*17, "Distance");       ips_show_float(8*10,16*17, trajectory_distance,3,6);

//    ips200_draw_line(0, 16*18, 239, 16*18, RGB565_RED);

}
