/*
 * Subject_2.c
 *
 *  Created on: 2026年6月7日
 *      Author: Shelton Liu
 *
 *  惯导回放模式：
 *    1. 手动推车录制八字路径（Body_keep + guandao_record）
 *    2. 回放时 guandao_load() 自动更新 Taget_angle
 *    3. 转向环直接跟踪 Taget_angle（guandao_load 内部已做阶梯逼近）
 *    4. 跑完自动停车
 */

#include "zf_common_headfile.h"


void Body_ctrl_2(void)
{
    static bool once = false;     //保证只赋值一次
    if (!once)
    {
      balance_mode_parameter(2);   //分配科目2参数

      /* 回放时目标角从当前 yaw 开始，guandao_load 会逐步推向路径方向 */
      if(gd_mode == guandao_load_mode && guandao_index > 0)
      {
          Taget_angle = roll_balance_cascade.posture_value.yaw;
      }

      once = true;
    }


    if(sys_times%5==0)//角度环
    {
        pid_control(&roll_balance_cascade.angle_cycle, roll_balance_cascade.turn_cycle.out-roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);
    }

    if(sys_times%20==0)//转向环
    {
        /* Taget_angle 由 guandao_load() 在 ISR 中自动步进更新，不需要再套 StepApproach */
        pid_control(&roll_balance_cascade.turn_cycle, AngleErrorNormalize(Taget_angle - roll_balance_cascade.posture_value.yaw), 0);
    }


     Steer_set(SERVO_MOTOR_MID + roll_balance_cascade.angle_cycle.out);

     /* 路径终点检测，自动停车 */
     if(guandao_new_cnt >= guandao_index)
     {
//         CYT2_S_motor_loop_ctrl(0);
         CYT2_S_motor_ctrl(0);
     }
     else
     {
//         CYT2_S_motor_loop_ctrl(33);//闭环
         CYT2_S_motor_ctrl(750);//开环
     }

}


/*
 * LCD 调试显示 — 在 ips200 上显示惯导跟踪关键数据
 */
void Balance_2_text(void)
{
    float yaw_err = AngleErrorNormalize(Taget_angle - roll_balance_cascade.posture_value.yaw);
    float yaw_ref = 0;
    if(guandao_new_cnt < guandao_index) yaw_ref = Yaw_Record_f[guandao_new_cnt];

    ips_show_string(8*0,  16*0, "YAW:");      ips_show_float(8*10, 16*0, roll_balance_cascade.posture_value.yaw, 3, 6);
    ips_show_string(8*0,  16*1, "T_A:");       ips_show_float(8*10, 16*1, Taget_angle, 3, 6);
    ips_show_string(8*0,  16*2, "Y_ERR:");     ips_show_float(8*10, 16*2, yaw_err, 3, 6);
    ips_show_string(8*0,  16*3, "c_err:");     ips_show_float(8*10, 16*3, c_error, 3, 6);
    ips_show_string(8*0,  16*4, "Y_ref:");     ips_show_float(8*10, 16*4, yaw_ref, 3, 6);
    ips_show_string(8*0,  16*5, "idx:");       ips_show_uint(8*10, 16*5, guandao_new_cnt, 5);
    ips_show_string(8*0,  16*6, "total:");     ips_show_uint(8*10, 16*6, guandao_index, 5);
    ips_show_string(8*0,  16*7, "X:");         ips_show_float(8*10, 16*7, data_x.f, 3, 6);
    ips_show_string(8*0,  16*8, "Y:");         ips_show_float(8*10, 16*8, data_y.f, 3, 6);
    ips_show_string(8*0,  16*9, "G_out:");     ips_show_float(8*10, 16*9, roll_balance_cascade.angle_cycle.out, 3, 6);

    /* 最后显示运行状态 */
    if(guandao_new_cnt >= guandao_index && guandao_index > 0)
        ips_show_string(8*0, 16*11, " === FINISHED ===");
    else if(gd_mode == guandao_load_mode)
        ips_show_string(8*0, 16*11, " >>> TRACKING <<<");
    else
        ips_show_string(8*0, 16*11, " --- IDLE ---");
}


/*
 * 串口调试输出 — 每 100ms 打印一行惯导跟踪数据
 * 在串口助手中查看，方便录数据做图表分析
 */
void Balance_2_printf(void)
{
    static uint32 print_tick = 0;
    print_tick++;

    if(print_tick < 100) return;  // 100ms 输出一行 (sys_times 1ms)
    print_tick = 0;

    float yaw     = roll_balance_cascade.posture_value.yaw;
    float yaw_ref = 0;
    if(guandao_new_cnt < guandao_index) yaw_ref = Yaw_Record_f[guandao_new_cnt];

    printf("INS|idx=%4d/%d|yaw=%+7.2f|T_A=%+7.2f|c_err=%+6.3f|yaw_ref=%+7.2f|X=%+6.3f|Y=%+6.3f\r\n",
           guandao_new_cnt, guandao_index,
           yaw, Taget_angle, c_error, yaw_ref,
           data_x.f, data_y.f);
}
