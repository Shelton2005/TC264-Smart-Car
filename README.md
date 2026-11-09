# 智能车控制系统｜代码与实物演示

基于 Infineon AURIX TC264 的嵌入式智能车项目，包含姿态解算、串级 PID 控制、轨迹记录与回放，以及 Stanley 路径跟踪。

- **比赛成绩：华北赛区第十四名。**
- **本人角色：核心软件成员。** 
- **开发语言：C；平台：TC264；工程环境：AURIX Development Studio / TASKING 工具链。**

> An embedded smart car project on Infineon AURIX TC264, featuring IMU attitude estimation, cascaded PID control, trajectory recording and replay, and Stanley path tracking. Regional result: 14th in North China. My role: core software team member.

## 实物演示

已收录三段原始视频，点击链接查看或下载。视频任务名称暂按编号列出，待核对内容后补充。

| 视频 | 文件大小（约） | 链接 |
| --- | --- | --- |
| 演示 1 | 26.0 MB | [查看视频](成果演示/1281512be63a008036ed8915d2f2e0d6.mp4) |
| 演示 2 | 28.7 MB | [查看视频](成果演示/880d8c14613d9218a01897d184df3cd6.mp4) |
| 演示 3 | 20.6 MB | [查看视频](成果演示/99f8bc6c308621eaa474304324e41403.mp4) |

## 核心实现与代码入口

| 模块 | 实现内容 | 代码入口 |
| --- | --- | --- |
| 姿态解算与控制器 | 四元数姿态解算、IMU 数据处理、位置式及增量式 PID、分模式参数 | [Imu.c](code/Imu.c)、[Imu.h](code/Imu.h) |
| 路径记录与跟踪 | 结合速度反馈与 yaw 更新平面位置，轨迹 Flash 存取，Stanley 跟踪与航向目标生成 | [Ins.c](code/Ins.c)、[Ins.h](code/Ins.h) |
| 控制任务 | 不同任务的姿态、转向和电机控制逻辑 | [Sub_1.c](code/Sub_1.c)、[Sub_2.c](code/Sub_2.c)、[Sub_3.c](code/Sub_3.c) |
| 通用算法 | 一阶低通滤波、目标值渐变、角度误差归一化、角度目标渐变 | [PID.c](code/PID.c) |
| 外设接口 | 电机、舵机、按键、蜂鸣器等接口 | [Peripherals.c](code/Peripherals.c) |
| 调试交互 | 菜单与显示交互 | [GUI_method.c](code/GUI_method.c) |
| 启动与调度 | 初始化、主循环、1 ms 定时中断及控制任务调用 | [cpu0_main.c](user/cpu0_main.c) |

建议阅读顺序：`cpu0_main.c` → `Sub_1/2/3.c` → `Imu.c` / `Ins.c` → 外设与底层库。

### 控制流程

`IMU / 电机速度反馈 → 姿态与位置估计 → 轨迹跟踪 / 航向目标 → 串级 PID → 舵机与电机`

当前工程以 1 ms 定时中断组织采样和任务调用。以 `Body_ctrl_1()` 为例，角速度环、角度环和转向环分别按 1 ms、5 ms 和 20 ms 调度。

各任务采用不同的控制结构。当前部分电机调用使用开环控制，源码中也保留了闭环接口与调试分支；不能仅凭存在速度环参数就认定所有任务都启用了速度闭环。

### 可供交流的实现细节

- **跨角度边界的转向控制：** `AngleErrorNormalize()` 处理 ±180° 附近的角度误差，`StepApproachAngleMode()` 提供角度目标渐变。
- **轨迹回放：** `Ins.c` 根据记录点和横向误差生成航向目标，`Sub_2.c` 将航向目标接入转向控制，并包含终点停车逻辑。
- **滤波与响应速度：** `LowPassFilter()` 提供一阶低通滤波，系数影响平滑程度与响应速度。

以上为代码实现说明；尚未整理对比曲线、误差统计或调参前后视频，不将其作为量化性能结论。

## 工程结构

```text
.
├── README.md
├── .project / .cproject         # ADS 工程配置
├── Lcf_Tasking_Tricore_Tc.lsl   # 链接配置
├── code/                       # 业务算法与控制任务
├── user/                       # 多核入口与中断配置
├── libraries/                  # 芯片与逐飞等底层库
└── 成果演示/                    # 三段原始演示视频
```

## 开发与复现

1. 使用 AURIX Development Studio 导入本目录下的既有工程（包含 `.project` 和 `.cproject`），检查 TASKING 工具链是否可用。底层库文件头标注的参考环境为 ADS v1.10.2，实际使用版本仍需补充。
2. 检查目标板、链接配置、IMU 型号及安装方向、电机驱动串口、舵机 PWM 和引脚分配。相关入口为 `Imu.h`、`Peripherals.h`、`Peripherals.c` 和 `cpu0_main.c`。
3. 构建工程，通过配套调试器下载到目标硬件；结合屏幕菜单与串口输出进行调试。
4. 根据实车重新校准机械零点、反馈方向与控制参数，再验证对应任务。

当前源码默认 IMU 数据宏指向 IMU660RA；工程中同时存在其他 IMU 型号分支。电机初始化使用 `small_driver_uart_init()`，显示初始化使用 `IPS200_TYPE_SPI`。这些是当前源码配置，不代表完整硬件物料清单。

本次整理仅检查文档与源码引用，未进行工具链编译、烧录或实车复测。仓库保留了开发期间的注释与实验分支，复现前应核对当前启用路径。

## 项目来源与资料

`libraries/` 和部分 `user/` 文件包含逐飞科技及 Infineon 等第三方代码和原有版权说明。逐飞库的说明见 [GPL3_permission_statement.txt](libraries/doc/GPL3_permission_statement.txt)。第三方底层库不属于个人独立开发内容，原始声明应随工程保留。

比赛正式名称、参赛年份、具体个人负责模块及各视频对应任务尚待补充。
