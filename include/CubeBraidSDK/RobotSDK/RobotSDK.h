#ifndef ROBOT_SDK_H
#define ROBOT_SDK_H

#include <string>
#include <memory>
#include <Eigen/Dense>

// 动态库导出/导入宏定义
#if defined(_WIN32) || defined(_WIN64)
    #ifdef ROBOT_SDK_EXPORTS
        #define ROBOT_API __declspec(dllexport) // Windows 下导出符号
    #else
        #define ROBOT_API __declspec(dllimport) // Windows 下导入符号
    #endif
#else
    #define ROBOT_API __attribute__((visibility("default"))) // Linux/Mac 下设置默认可见性
#endif

namespace robot_sdk 
{
    #ifndef M_PI
    #define M_PI 3.14159265358979323846
    #endif

    /**
     * @brief 机器人位姿结构体 (XYZABC,位置（单位：毫米）,姿态（单位：°）)
     */
    struct Pose { double x{0.0}, y{0.0}, z{0.0}, rx{0.0}, ry{0.0}, rz{0.0}; };

    /**
     * @brief 机器人关节角度结构体 (J1-J6,单位:°)
     */
    struct Joint { double j1{0.0}, j2{0.0}, j3{0.0}, j4{0.0}, j5{0.0}, j6{0.0}; };

    /**
     * @brief 箱体/SKU 尺寸结构体 (mm)
     */
    struct BoxDimension { float length{0.0f}, width{0.0f}, height{0.0f}; };

    /**
     * @brief 运动控制精度模式
     */
    enum class ControlMode 
    { 
        Coarse = 1,    // 粗略到位/过渡点 (精细度 50mm)
        Fine = 2,      // 精准到位/目标点 (精细度 1mm)
        Transition = 3 // 远距离过渡点 (精细度 200mm)
    };

 
    /**
     * @brief 机器人 SDK 控制器主类
     */
    class ROBOT_API RobotController 
    {
    public:
        RobotController();
        ~RobotController();

        /**
         * @brief 连接机器人
         * @param ip 机器人 IP 地址
         * @param motionPort 控制运动端口 (默认 31400)
         * @param statusPort 状态监听端口 (默认 31401)
         * @return bool 连接是否成功
         */
        bool connectRobot(const std::string& ip, int motionPort = 31400, int statusPort = 31401);

        /**
         * @brief 断开机器人连接
         */
        void disconnectRobot();

        /**
         * @brief 检查当前是否已建立连接
         * @return bool 是否连接成功
         */
        bool isConnected() const;

        /**
         * @brief 基于空间位姿的笛卡尔运动控制
         * @param mode 运动控制模式 (Coarse/Fine/Transition)
         * @param pose 目标位姿 (x, y, z, rx, ry, rz)
         * @return bool 执行是否成功
         */
        bool controlPosture(ControlMode mode, const Pose& pose);

        /**
         * @brief 基于关节角度的运动控制
         * @param mode 运动控制模式 (Coarse/Fine)
         * @param joint 6 个关节的目标角度
         * @return bool 执行是否成功
         */
        bool controlJoint(ControlMode mode, const Joint& joint);

        // 获取机器人当前状态
        Pose GetCurrentPose();
        Joint GetCurrentJoint();
        double GetJoint4Angle();

        // ==========================================
        // 抓取算法与斜面角度补偿接口
        // ==========================================

        /**
         * @brief 顶吸斜面 X 方向位置补偿计算
         */
        static float top_x_value(float benchmark_x, float angleDeg, float h, float x0);

        /**
         * @brief 顶吸斜面 Z 方向位置补偿计算
         */
        static float top_z_value(float angleDeg, float h, float x0);

        /**
         * @brief 侧吸斜面 X 方向位置补偿计算
         */
        static float side_x_value(float benchmark_x, float angleDeg, float h, float x0);

        /**
         * @brief 侧吸斜面 Z 方向位置补偿计算
         */
        static float side_z_value(float angleDeg, float h, float x0);

        /**
        * @brief 顶吸偏移量xyz求解
        * @param centroid 基准点位置 (单位: 米)
        * @param box SKU 产品尺寸 (单位: 毫米)
        * @param fetchMode 抓取模式 (1,2: 沿长边抓取; 3,4: 沿短边抓取)
        * @param sku_num 并排抓取的 SKU 数量
        * @param dis_y 每层剩余缝隙
        * @param poseOffset json垛型x,y,z数据 (单位: 毫米)
        * @param ROffset 垛型最左侧标志位 (true: 最左侧, false: 右侧)
        * @param model_mod 是否为第一面(0为第一面, 1为其它面)
        * @param inclx_angle 倾角仪角度
        * @return Pose 计算得出的最终机器人工具端目标位姿 (单位: 毫米)
        */
        static Pose Top_suction_angle(
            const Pose& centroid, const BoxDimension& box, int fetchMode, 
            int sku_num, float dis_y, const Pose& poseOffset, bool ROffset, 
            int model_mod, double inclx_angle = 0.0
        );

        /**
        * @brief 顶吸偏移量xyz求解(与侧吸同层)
        * @param centroid 基准点位置 (单位: 米)
        * @param box SKU 产品尺寸 (单位: 毫米)
        * @param fetchMode 抓取模式 (1,2: 沿长边抓取; 3,4: 沿短边抓取)
        * @param fetchMode_side 左下参考基准纸箱抓取模式 (1,2: 沿长边抓取; 3,4: 沿短边抓取)
        * @param sku_num 并排抓取的 SKU 数量
        * @param dis_y 每层剩余缝隙
        * @param poseOffset json垛型x,y,z数据 (单位: 毫米)
        * @param ROffset 垛型最左侧标志位 (true: 最左侧, false: 右侧)
        * @param model_mod 是否为第一面(0为第一面, 1为其它面)
        * @param inclx_angle 倾角仪角度
        * @return Pose 计算得出的最终机器人工具端目标位姿 (单位: 毫米)
        */
        static Pose Top_suction_angle_switch(
            const Pose& centroid, const BoxDimension& box, int fetchMode, int fetchMode_side,
            int sku_num, float dis_y, const Pose& poseOffset, bool ROffset, int model_mod, double inclx_angle);

        /**
        * @brief 顶吸特殊码法偏移量xyz求解
        * @param centroid 基准点位置 (单位: 米)
        * @param box SKU 产品尺寸 (单位: 毫米)
        * @param fetchMode 抓取模式 (1,2: 沿长边抓取; 3,4: 沿短边抓取)
        * @param sku_num 并排抓取的 SKU 数量
        * @param dis_y 每层剩余缝隙
        * @param poseOffset json垛型x,y,z数据 (单位: 毫米)
        * @param ROffset 垛型最左侧标志位 (true: 最左侧, false: 右侧)
        * @param model_mod 是否为第一面(0为第一面, 1为其它面)
        * @param inclx_angle 倾角仪角度
        * @return Pose 计算得出的最终机器人工具端目标位姿 (单位: 毫米)
        */
        static Pose Top_suction_special(
            const Pose& centroid, const BoxDimension& box, int fetchMode, 
            int sku_num, float dis_y, const Pose& poseOffset, bool ROffset, 
            int model_mod, double inclx_angle = 0.0
        );

        /**
        * @brief 侧吸偏移量xyz求解
        * @param centroid 基准点位置 (单位: 米)
        * @param box SKU 产品尺寸 (单位: 毫米)
        * @param fetchMode 抓取模式 (1,2: 沿长边抓取; 3,4: 沿短边抓取)
        * @param fetchMode_side 每面垛型左下第一个纸箱抓取模式 (1,2: 沿长边抓取; 3,4: 沿短边抓取)
        * @param sku_num 并排抓取的 SKU 数量
        * @param dis_y 每层剩余缝隙
        * @param poseOffset json垛型x,y,z数据 (单位: 毫米)
        * @param ROffset 垛型最左侧标志位 (true: 最左侧, false: 右侧)
        * @param model_mod 是否为第一面(0为第一面, 1为其它面)
        * @param inclx_angle 倾角仪角度
        * @param container_h 集装箱高度(单位: 毫米)
        * @param switch_top_bottom_suction 上层/下次侧吸切换标志位 (true: 下层侧吸(酒类), false: 上层侧吸(默认))
        * @return Pose 计算得出的最终机器人工具端目标位姿 (单位: 毫米)
        */
        static Pose Side_suction_angle(
            const Pose& centroid, const BoxDimension& box, int fetchMode, int fetchMode_side,
            int sku_num, float dis_y, const Pose& poseOffset, bool ROffset, 
            int model_mod, double inclx_angle, float container_h, bool switch_top_bottom_suction = false
        );

        /**
         * @brief 计算相对旋转 (ZYZ 顺序欧拉角，输入输出单位：角度 deg)
         */
        static Eigen::Vector3d computeDeltaEulerZYZ_deg(
            const Eigen::Vector3d &euler_before_deg,
            const Eigen::Vector3d &euler_temp_deg
        );

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl; // PImpl 模式指针，用于隐藏底层通讯细节

        static Eigen::Vector3d matrixToEulerZYZ(const Eigen::Matrix3d &R);
    };
}

// ==========================================
// C 语言导出接口 (供 C# / Python / C++ 动态库调用)
// ==========================================
#ifdef __cplusplus
extern "C" {
#endif

    typedef void* RobotHandle; // 机器人实例句柄

    /** @brief 创建机器人控制器实例 */
    ROBOT_API RobotHandle Robot_Create();

    /** @brief 销毁机器人控制器实例 */
    ROBOT_API void Robot_Destroy(RobotHandle handle);

    /** @brief 连接机器人 */
    ROBOT_API int Robot_Connect(RobotHandle handle, const char* ip, int motionPort, int statusPort);

    /** @brief 断开连接 */
    ROBOT_API void Robot_Disconnect(RobotHandle handle);

    /** @brief 笛卡尔空间位姿控制 */
    ROBOT_API int Robot_ControlPosture(RobotHandle handle, int mode, double x, double y, double z, double rx, double ry, double rz);

    /** @brief 关节角度控制 */
    ROBOT_API int Robot_ControlJoint(RobotHandle handle, int mode, double j1, double j2, double j3, double j4, double j5, double j6);

    /** @brief 获取当前位姿 */
    ROBOT_API int Robot_GetCurrentPose(RobotHandle handle, double* x, double* y, double* z, double* rx, double* ry, double* rz);

    /** @brief 获取当前关节角度 */
    ROBOT_API int Robot_GetCurrentJoint(RobotHandle handle, double* j1, double* j2, double* j3, double* j4, double* j5, double* j6);

    /** @brief 获取 J4 关节角度 */
    ROBOT_API int Robot_GetJoint4Angle(RobotHandle handle, double* j4);

    /** @brief 带倾角补偿的顶吸目标位姿计算 */
    // ROBOT_API int Robot_TopSuctionAngle(RobotHandle handle, double* x, double* y, double* z);

    ROBOT_API int Robot_TopSuctionAngle(
        RobotHandle handle,
        robot_sdk::Pose centroid, robot_sdk::BoxDimension box, int fetchMode,
        int sku_num, float dis_y, robot_sdk::Pose poseOffset, bool ROffset,
        int model_mod, double inclx_angle,
        double* x, double* y, double* z
    );

    /** @brief 带倾角补偿的顶吸目标位姿计算(同一面顶/侧吸混合) */
    ROBOT_API int Robot_TopSuctionAngleSwitch(
        RobotHandle handle,
        robot_sdk::Pose centroid, robot_sdk::BoxDimension box, int fetchMode, int fetchMode_side,
        int sku_num, float dis_y, robot_sdk::Pose poseOffset, bool ROffset,
        int model_mod, double inclx_angle,
        double* x, double* y, double* z
    );

    /** @brief 带倾角补偿的顶吸目标位姿计算(特殊码法) */
    // ROBOT_API int Robot_TopSuctionSpecial(RobotHandle handle, double* x, double* y, double* z);
    // 导出函数声明
    ROBOT_API int Robot_TopSuctionSpecial(
        RobotHandle handle,
        robot_sdk::Pose centroid, robot_sdk::BoxDimension box, int fetchMode,
        int sku_num, float dis_y, robot_sdk::Pose poseOffset, bool ROffset,
        int model_mod, double inclx_angle,
        double* x, double* y, double* z
    );

    /** @brief 带倾角补偿的侧吸目标位姿计算 */
    // ROBOT_API int Robot_TopSuctionSpecial(RobotHandle handle, double* x, double* y, double* z);
    // 导出函数声明
    ROBOT_API int Robot_SideSuctionAngle(
        RobotHandle handle,
        robot_sdk::Pose centroid, robot_sdk::BoxDimension box, int fetchMode, int fetchMode_side,
        int sku_num, float dis_y, robot_sdk::Pose poseOffset, bool ROffset,
        int model_mod, double inclx_angle, float container_h, bool switch_top_bottom_suction,
        double* x, double* y, double* z
    );

    /**
     * @brief C API: 计算相对旋转 (ZYZ 顺序欧拉角)
     * @param euler_before 变换前欧拉角数组 [z1, y, z2] (单位: 度)
     * @param euler_temp 变换后欧拉角数组 [z1, y, z2] (单位: 度)
     * @param delta_out 转换结果输出数组 [z1, y, z2] (单位: 度)
     * @return int 1-成功, 0-失败
     */
    ROBOT_API int Robot_ComputeDeltaEulerZYZ(
        RobotHandle handle,
        const double euler_before[3],
        const double euler_temp[3],
        double delta_out[3]
    );

#ifdef __cplusplus
}
#endif

#endif // ROBOT_SDK_H