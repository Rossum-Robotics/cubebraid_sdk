#ifndef PLC_SDK_H
#define PLC_SDK_H

#include <stdint.h>
#include <stdbool.h>

// 导出/导入宏定义 (支持 Windows DLL / Linux SO)
#if defined(_WIN32) || defined(_WIN64)
    #if defined(PLC_SDK_EXPORTS)
        #define PLC_SDK_API __declspec(dllexport)
    #else
        #define PLC_SDK_API __declspec(dllimport)
    #endif
#else
    #if defined(PLC_SDK_EXPORTS)
        #define PLC_SDK_API __attribute__((visibility("default")))
    #else
        #define PLC_SDK_API
    #endif
#endif

// =================================================================
// 1. C / C++ 通用数据结构定义
// =================================================================
#pragma pack(push, 1)

// PLC 读取状态结构体
typedef struct 
{
    int16_t suction_cup_state;            // 吸盘状态
    int16_t fixture_state;                // 底托状态
    int16_t robot_pick_state;             // 机器人取料状态
    int16_t robot_leave_state;            // 允许机器人取料离开状态
    int16_t table_control_state;          // 摆台控制状态
    int16_t hydraulic_rod_rise_state;     // 液压杆上升状态
    int16_t hydraulic_rod_lower_state;    // 液压杆下降状态
    int16_t plc_init_state;               // PLC初始化状态
    int16_t table_init_control_ok_state;  // 摆台初始化完成状态
    int16_t plc_data_request_state;       // PLC数据请求状态
    int16_t plc_unload_state;             // PLC卸货允许请求状态
} PLCStatus;

// 取料参数输入结构体
typedef struct 
{
    int fetch_mode;       // 取料模式 (1, 2 为转动；3, 4 为不转动)
    int mode_switch;      // 模式切换参数
    double sku_l;         // 长 (mm)
    double sku_w;         // 宽 (mm)
    double sku_h;         // 高 (mm)
    int sku_num;          // 数量
    double sku_weight;    // 单个重量 (kg)
} PickUpData;

#pragma pack(pop)

// 四组吸盘阀的 8421 位掩码（C/C++ 共用，支持按位或组合）。
// PLC：DB22.DBW48，INT，西门子大端字节序。
typedef enum
{
    PLC_SUCTION_VALVE_1   = 0x01, // 0001：阀1
    PLC_SUCTION_VALVE_2   = 0x02, // 0010：阀2
    PLC_SUCTION_VALVE_3   = 0x04, // 0100：阀3
    PLC_SUCTION_VALVE_4   = 0x08, // 1000：阀4
    PLC_SUCTION_VALVE_ALL = 0x0F  // 1111：全部吸盘阀
} PLCSuctionValve;

// 吸盘选择命令的 SDK 写入状态，不代表气阀动作或真空建立状态。
typedef enum
{
    PLC_SUCTION_SELECTION_IDLE      = 0, // 尚未提交
    PLC_SUCTION_SELECTION_PENDING   = 1, // 等待后台线程写入
    PLC_SUCTION_SELECTION_WRITTEN   = 2, // DBW48 写入成功
    PLC_SUCTION_SELECTION_FAILED    = 3, // 写入失败，禁止使用旧选择开吸盘
    PLC_SUCTION_SELECTION_CANCELLED = 4  // 请求取消或连接已断开
} PLCSuctionSelectionState;

// 独立状态结构，不改变原 PLCStatus、PickUpData 的内存布局。
typedef struct
{
    int32_t selection_mask; // 最近受理的选择值（0表示尚未选择，不是PLC指令）
    int32_t write_state;    // PLCSuctionSelectionState 中的状态值
} PLCSuctionSelectionStatus;

// C API 句柄定义
typedef void* PLC_HANDLE;
// =================================================================
// 2. 导出 C-API 接口 (支持 C/C++ 及其他语言调用)
// =================================================================
#ifdef __cplusplus
extern "C" 
{
    #endif

    /**
    * @brief 创建 PLC 控制器实例句柄
    * @return PLC_HANDLE 成功返回非空句柄，失败返回 NULL
    */
    PLC_SDK_API PLC_HANDLE plc_create(void);

    /**
    * @brief 销毁 PLC 控制器实例句柄
    * @param handle 控制器句柄
    */
    PLC_SDK_API void plc_destroy(PLC_HANDLE handle);

    /**
    * @brief 连接 PLC
    * @param handle 控制器句柄
    * @param ip PLC IP 地址
    * @param rack 机架号
    * @param slot 插槽号
    * @return bool 是否连接成功
    */
    PLC_SDK_API bool plc_connect(PLC_HANDLE handle, const char* ip, int rack, int slot);

    /**
    * @brief 断开 PLC 连接
    * @param handle 控制器句柄
    */
    PLC_SDK_API void plc_disconnect(PLC_HANDLE handle);

    /**
    * @brief 查询 PLC 连接状态
    * @param handle 控制器句柄
    * @return bool 是否处于连接状态
    */
    PLC_SDK_API bool plc_is_connected(PLC_HANDLE handle);

    /**
    * @brief 安全写入 PLC DB 块
    * @param handle 控制器句柄
    * @param dbNumber DB 块编号
    * @param start 起始地址
    * @param size 写入字节数
    * @param buffer 数据缓冲区指针
    * @return bool 是否写入成功
    */
    PLC_SDK_API bool plc_safe_db_write(PLC_HANDLE handle, int dbNumber, int start, int size, void* buffer);

    /**
    * @brief 安全读取 PLC DB 块
    * @param handle 控制器句柄
    * @param dbNumber DB 块编号
    * @param start 起始地址
    * @param size 读取字节数
    * @param buffer 接收缓冲区指针
    * @return bool 是否读取成功
    */
    PLC_SDK_API bool plc_safe_db_read(PLC_HANDLE handle, int dbNumber, int start, int size, void* buffer);

    /**
    * @brief 发送吸盘打开信号
    * @param handle 控制器句柄
    * @param enable 是否打开
    * @return None
    */
    PLC_SDK_API void plc_set_open_suction_cup(PLC_HANDLE handle, bool enable);

    /**
    * @brief 提交吸盘选择命令（DB22.DBW48，1~15）
    * @return true 请求已受理，实际写入结果通过状态接口获取
    * @note 禁止在吸持工件、开关命令待处理或已有选择请求待处理时切换。
    *       调用方须先确认夹具处于可安全切换的状态。
    */
    PLC_SDK_API bool plc_set_suction_cup_selection(PLC_HANDLE handle, int selection);

    /** @brief 通过四个布尔参数选择吸盘阀组；全部false不发送。 */
    PLC_SDK_API bool plc_set_suction_cup_valves(PLC_HANDLE handle,
        bool valve1, bool valve2, bool valve3, bool valve4);

    /** @brief 获取最近一次受理的吸盘选择及SDK写入状态。 */
    PLC_SDK_API bool plc_get_suction_cup_selection_status(PLC_HANDLE handle,
        PLCSuctionSelectionStatus* status_out);

    /**
    * @brief 提交开吸盘命令，返回是否受理（不是动作成功反馈）。
    * @note 使用分组控制后，必须先确认选择写入成功，并由调用方确认PLC已应用选择。
    */
    PLC_SDK_API bool plc_request_open_suction_cup(PLC_HANDLE handle);

    /**
    * @brief 发送机器人吸取ok信号
    * @param handle 控制器句柄
    * @param enable 是否打开
    * @return None
    */
    PLC_SDK_API void plc_set_suction_cup_picking_ok(PLC_HANDLE handle, bool enable);

    /**
    * @brief 发送底托伸出信号
    * @param handle 控制器句柄
    * @param enable 是否打开
    * @return None
    */
    PLC_SDK_API void plc_set_fixture_roll_out(PLC_HANDLE handle, bool enable);

    /**
    * @brief 发送取料完成信号
    * @param handle 控制器句柄
    * @param enable 是否打开
    * @return None
    */
    PLC_SDK_API void plc_set_retrieving_completion_side(PLC_HANDLE handle, bool enable);

    /**
    * @brief 发送底托缩回信号
    * @param handle 控制器句柄
    * @param enable 是否打开
    * @return None
    */
    PLC_SDK_API void plc_set_fixture_initialization(PLC_HANDLE handle, bool enable);

    /**
    * @brief 发送吸盘关闭信号
    * @param handle 控制器句柄
    * @param enable 是否打开
    * @return None
    */
    PLC_SDK_API void plc_set_close_suction_cup(PLC_HANDLE handle, bool enable);

    /**
    * @brief 发送倾角仪异常信号
    * @param handle 控制器句柄
    * @param enable 是否打开
    * @return None
    */
    PLC_SDK_API void plc_set_inclinometer_error_signal(PLC_HANDLE handle, bool enable);

    /**
    * @brief 发送卸柜放置OK信号
    * @param handle 控制器句柄
    * @param enable 是否打开
    * @return None
    */
    PLC_SDK_API void plc_set_unload_ok_signal(PLC_HANDLE handle, bool enable);

    // 业务数据与状态控制接口
    /**
    * @brief 获取当前PLC状态
    * @param handle 控制器句柄
    * @param status_out 当前缓存的PLC状态结构体
    * @return bool 是否获取成功
    */
    PLC_SDK_API bool plc_get_status(PLC_HANDLE handle, PLCStatus* status_out);

    /**
    * @brief 控制摆台运动信号
    * @param handle 控制器句柄
    * @param angle_mode 角度序号
    * @return None
    */
    PLC_SDK_API void plc_control_table_angle(PLC_HANDLE handle, int angle_mode);

    /**
    * @brief 更新当前装柜状态
    * @param handle 控制器句柄
    * @param surface_num 面数
    * @param lay_num 层数
    * @param box_num 单次装柜箱子数量
    * @return None
    */
    PLC_SDK_API void plc_update_box_state(PLC_HANDLE handle, int surface_num, int lay_num, int box_num);

    /**
    * @brief 更新当前倾角仪角度
    * @param handle 控制器句柄
    * @param angle_value 倾角仪角度
    * @return None
    */
    PLC_SDK_API void plc_update_inclinometer_angle(PLC_HANDLE handle, float angle_value);

    /**
    * @brief 更新总装柜与剩余装柜箱子数量
    * @param handle 控制器句柄
    * @param totalbox_num 总装柜箱子数量
    * @param remain_num 剩余装柜箱子数量
    * @return None
    */
    PLC_SDK_API void plc_update_total_box_state(PLC_HANDLE handle, int totalbox_num, int remain_num);

    /**
    * @brief 发送取料参数
    * @param handle 控制器句柄
    * @param data PickUpData取料参数结构体
    * @return None
    */
    PLC_SDK_API bool plc_send_pickup_data(PLC_HANDLE handle, const PickUpData* data);

    // 触发初始化接口
    /**
    * @brief 触发PLC一键初始化信号
    * @param handle 控制器句柄
    * @return bool 是否初始化完成
    */
    PLC_SDK_API bool plc_trigger_plc_init(PLC_HANDLE handle);

    /**
    * @brief 触发摆台初始化信号
    * @param handle 控制器句柄
    * @return bool 是否初始化完成
    */
    PLC_SDK_API bool plc_trigger_table_init(PLC_HANDLE handle);

    #ifdef __cplusplus
}
#endif

// =================================================================
// 3. 原生 C++ 类接口 (面向 C++ 开发者直接使用)
// =================================================================
#ifdef __cplusplus

#include "CubeBraidSDK/PLC_SDK/snap7.h"
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

namespace plc_sdk 
{
    class PLC_SDK_API PLCController 
    {
    public:
        PLCController();
        ~PLCController();

        /**
        * @brief 连接 PLC
        * @param ip PLC IP 地址
        * @param rack 机架号
        * @param slot 插槽号
        * @return bool 是否连接成功
        */
        bool connect(const std::string& ip, int rack = 0, int slot = 1);

        /**
        * @brief 断开 PLC 连接
        * @param None
        */
        void disconnect();

        /**
        * @brief 查询 PLC 连接状态
        * @return bool 是否处于连接状态
        */
        bool isConnected() const;

        // 基础 DB 块读写接口
        /**
        * @brief 安全写入 PLC DB 块
        * @param dbNumber DB 块编号
        * @param start 起始地址
        * @param size 写入字节数
        * @param buffer 数据缓冲区指针
        * @return bool 是否写入成功
        */
        bool safeDBWrite(int dbNumber, int start, int size, void* buffer);

        /**
        * @brief 安全读取 PLC DB 块
        * @param dbNumber DB 块编号
        * @param start 起始地址
        * @param size 读取字节数
        * @param buffer 接收缓冲区指针
        * @return bool 是否读取成功
        */
        bool safeDBRead(int dbNumber, int start, int size, void* buffer);

        // 控制离散信号设置接口
        /**
        * @brief 发送吸盘打开信号
        * @param enable 是否打开
        * @return None
        */
        void setOpenSuctionCup(bool enable);

        // C++ 吸盘阀组编码，与 C API 常量一致。
        enum SuctionValve : uint16_t
        {
            VALVE_1   = PLC_SUCTION_VALVE_1,
            VALVE_2   = PLC_SUCTION_VALVE_2,
            VALVE_3   = PLC_SUCTION_VALVE_3,
            VALVE_4   = PLC_SUCTION_VALVE_4,
            VALVE_ALL = PLC_SUCTION_VALVE_ALL
        };

        /** @brief 异步选择吸盘阀组，合法值1~15；true仅表示请求已受理。 */
        bool setSuctionCupSelection(int selection);

        /** @brief 四组吸盘独立选择，内部转换为8421位掩码。 */
        bool setSuctionCupValves(bool valve1, bool valve2, bool valve3, bool valve4);

        /** @brief 查询选择命令的SDK写入状态（非实际阀组/真空反馈）。 */
        PLCSuctionSelectionStatus getSuctionCupSelectionStatus();

        /** @brief 申请打开吸盘；选择未完成或失败时拒绝受理。 */
        bool requestOpenSuctionCup();

        /**
        * @brief 发送机器人吸取ok信号
        * @param enable 是否打开
        * @return None
        */
        void setSuctionCupPickingOK(bool enable);

        /**
        * @brief 发送底托伸出信号
        * @param enable 是否打开
        * @return None
        */
        void setFixtureRollOut(bool enable);

        /**
        * @brief 发送取料完成信号
        * @param enable 是否打开
        * @return None
        */
        void setRetrievingCompletionSide(bool enable);

        /**
        * @brief 发送底托缩回信号
        * @param enable 是否打开
        * @return None
        */
        void setFixtureInitialization(bool enable);

        /**
        * @brief 发送吸盘关闭信号
        * @param enable 是否打开
        * @return None
        */
        void setCloseSuctionCup(bool enable);

        /**
        * @brief 发送倾角仪异常信号
        * @param enable 是否打开
        * @return None
        */
        void setInclinometerErrorSignal(bool enable);

        /**
        * @brief 发送卸柜放置OK信号
        * @param enable 是否打开
        * @return None
        */
        void setUnloadOKSignal(bool enable);

        // 业务与状态数据控制接口
        /**
        * @brief 获取当前PLC状态
        * @param status_out 当前缓存的PLC状态结构体
        * @return bool 是否获取成功
        */
        PLCStatus getStatus();

        /**
        * @brief 控制摆台运动信号
        * @param angle_mode 角度序号
        * @return None
        */
        void controlTableAngle(int angle_mode);

        /**
        * @brief 更新当前装柜状态
        * @param surface_num 面数
        * @param lay_num 层数
        * @param box_num 单次装柜箱子数量
        * @return None
        */
        void updateBoxState(int surface_num, int lay_num, int box_num);

        /**
        * @brief 更新当前倾角仪角度
        * @param angle_value 倾角仪角度
        * @return None
        */
        void updateInclinometerAngle(float angle_value);

        /**
        * @brief 更新总装柜与剩余装柜箱子数量
        * @param totalbox_num 总装柜箱子数量
        * @param remain_num 剩余装柜箱子数量
        * @return None
        */
        void updateTotalBoxState(int totalbox_num, int remain_num);
        
        /**
        * @brief 发送取料参数
        * @param data PickUpData取料参数结构体
        * @return None
        */
        bool sendPickUpData(const PickUpData& data);

        // 初始化触发接口
        /**
        * @brief 触发PLC一键初始化信号
        * @return bool 是否初始化完成
        */
        bool triggerPLCInit();

        /**
        * @brief 触发摆台初始化信号
        * @return bool 是否初始化完成
        */
        bool triggerTableInit();

    private:
        int16_t swapBigEndianToHost(const uint8_t* buffer);
        void readLoop();
        void writeLoop();
        void monitorLoop();

        TS7Client client_;
        std::string ip_;
        int rack_;
        int slot_;

        std::atomic<bool> is_connected_{false};
        std::atomic<bool> is_running_{false};

        // 离散控制信号标志位
        bool open_suction_cup_signal_{false}; // 吸盘打开信号
        bool suction_cup_picking_ok_signal_{false}; // 机器人吸取ok信号
        bool fixture_roll_out_signal_{false};   // 底托推出信号
        bool retrieving_completion_signal_side_{false};     // 取料完成信号
        bool fixture_initialization_signal_{false};     // 底托缩回信号
        bool close_suction_cup_signal_{false};          // 吸盘关闭信号
        bool inclinometer_error_signal_{false};         // 倾角仪异常信号
        bool unload_ok_signal_{false};                  // 卸柜放置OK信号

        // 吸盘分组控制状态：以下成员均由 signal_mutex_ 保护。
        // 启用分组控制后，断线重连必须重新选择，不自动沿用旧掩码。
        bool suction_selection_enabled_{false};
        PLCSuctionSelectionStatus suction_selection_status_{0, PLC_SUCTION_SELECTION_IDLE};
        // SDK侧保守互锁：发出过开吸盘命令后，须成功写入关闭命令才解除。
        // 该标志不是PLC实际吸持状态；关闭写入成功也不等于工件已释放。
        bool suction_open_may_be_active_{false};

        PLCStatus current_status_{};    // PLC当前状态

        mutable std::mutex client_mutex_;
        std::mutex signal_mutex_;
        std::mutex status_mutex_;

        std::thread read_thread_;
        std::thread write_thread_;
        std::thread monitor_thread_;
    };
}
#endif // __cplusplus

#endif // PLC_SDK_H